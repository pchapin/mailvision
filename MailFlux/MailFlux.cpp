/*! \file    MailFlux.cpp
 *  \brief   The MailFlux main program.
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 *
 * This program is a simple SMTP server. It is intended to primarily support debugging and
 * educational goals. See the documentation folder for more information.
 */

// Standard C
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

// Standard C++
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// POSIX
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

// MailFlux
#include "config.hpp"
#include "Console.hpp"
#include "ServerConnection.hpp"
#include "Spool.hpp"

#define BUFFER_SIZE 128

using namespace std;

/*!
 * This function creates a pid file for this process.
 */
void create_pidfile( )
{
    ofstream outfile( "/var/run/MailFlux.pid" );

    if( !outfile ) return;
    outfile << getpid( ) << "\n";
}


/*!
 * This function erases the pid file. It should be called when this program terminates, but
 * currently this program never terminates (cleanly).
 */
void remove_pidfile( )
{
    remove( "/var/run/MailFlux.pid" );
}


/*!
 * This function detaches this process from its controlling terminal. See "Advanced Programming
 * in the Unix Environment" by W. Richard Stevens, Chapter 13, for more information.
 */
int daemonize( )
{
    pid_t pid;

    if(( pid = fork( )) < 0 ) return -1;
    else if( pid != 0 ) exit( 0 );         // Terminate parent process.

    setsid( );    // Become session leader.
    chdir( "/" );  // Switch to "well known" directory.
    umask( 0 );    // Plan to set all output file permissions explicitly.
    return 0;
}


/*!
 * This function prepares the listen socket and arranges to start listening on that socket. It
 * returns the listening socket handle if successful, otherwise it returns -1.
 *
 * \param port The port on which MailFlux should listen for client connections.
 */
int initialize_network( unsigned short port )
{
    int listen_handle;
    struct sockaddr_in server_address;
    ostringstream formatter;

    // Create the server socket.
    if(( listen_handle = socket( PF_INET, SOCK_STREAM, 0 )) < 0 ) {
        formatter << "Problem creating socket: " << strerror(errno);
        Console::put_exception_line( formatter.str( ).c_str( ));
        return -1;
    }

    // Prepare the server socket address structure.
    memset( &server_address, 0, sizeof( server_address ));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl( INADDR_ANY );
    server_address.sin_port = htons( port );

    // Bind the server socket.
    if( ::bind( listen_handle,
                (struct sockaddr *) &server_address, sizeof( server_address )) < 0 ) {
        formatter << "Problem binding: " << strerror(errno);
        Console::put_exception_line( formatter.str( ).c_str( ));
        close( listen_handle );
        return -1;
    }

    // Allow incoming connections.
    if( listen( listen_handle, 32 ) < 0 ) {
        formatter << "Problem listening: " << strerror(errno);
        Console::put_exception_line( formatter.str( ).c_str( ));
        close( listen_handle );
        return -1;
    }
    return listen_handle;
}


/*!
 * This function deals with a single connection. A new thread is created for each connection
 * that arrives and that thread executes this function. Thus there might be multiple activations
 * of this function occurring at the same time.
 */
void *connection_processor( void *arg )
{
    int *i_arg = static_cast<int *>(arg);
    int connection_handle = *i_arg;
    delete i_arg;

    try {
        ServerConnection client( connection_handle );
        client.doSMTP( );
    }
    catch( exception &e ) {
        Console::put_exception_line( e.what( ));
    }
    catch( ... ) {
        Console::put_exception_line( "Unknown exception in connection_processor()" );
    }

    close( connection_handle );
    return nullptr;
}


/*!
 * This function is the main server loop. It accepts connections and starts a thread to handle
 * each one. This function returns zero if it ended normally; one otherwise.
 */
void *accept_loop( void *arg )
{
    int listen_handle;        // Socket to listen on.
    int connection_handle;    // Socket handle for each connection.
    struct sockaddr_in client_address;       // Remote address.
    socklen_t client_length;        // Size of remote address.
    pthread_t connection_thread;    // ID of connection handling thread.
    char buffer[BUFFER_SIZE];  // Holds client address.
    ostringstream formatter;            // Used to format error messages.

    listen_handle = *static_cast<int *>(arg);

    while( true ) {
        memset( &client_address, 0, sizeof( client_address ));
        client_length = sizeof( client_address );

        // Block until a client comes along.
        if(( connection_handle = accept( listen_handle,
                                         (struct sockaddr *) &client_address, &client_length )) < 0 ) {
            formatter << "Problem with accept: " << strerror(errno);
            Console::put_exception_line( formatter.str( ).c_str( ));
            return nullptr;
        }

        // Display informational message.
        string client_info = "Accepted client connection from: ";
        inet_ntop( AF_INET, &client_address.sin_addr, buffer, BUFFER_SIZE );
        client_info += buffer;
        Console::put_line( client_info.c_str( ));

        int *i_arg = new int( connection_handle );
        pthread_create( &connection_thread, nullptr, connection_processor, i_arg );
        pthread_detach( connection_thread );
    }
    return nullptr;
}


//! Main Program
int main( int argc, char **argv )
{
    unsigned short port;              // Port number to listen on.
    int listen_handle;     // Handle of listening socket.
    pthread_t accept_thread;     // Accepts client connections.

    try {
        // Get the configuration early in case we want to use it below.
        Support::register_parameter( "PORT", "25", false );
        Support::register_parameter( "SPOOL", "spool", false );
        Support::read_config_files( "./MailFlux.cfg" );

        // Setup defaults.
        string *parameter = Support::lookup_parameter( "PORT" );
        if( parameter == nullptr ) port = 25;
        else {
            port = atoi( parameter->c_str( ));
            if( port == 0 ) port = 25;
        }

        // Start up the various subsystems. This needs to be done early so that email messages
        // and console messages are handled properly during the rest of the program's
        // initialization activities.
        //
        Console::initialize( );
        Spool::initialize( );

        // Set up the network handling.
        if(( listen_handle = initialize_network( port )) == -1 ) {
            Console::put_warning_line( "Network failed to initialize" );
        }
        pthread_create( &accept_thread, nullptr, accept_loop, &listen_handle );
        pthread_detach( accept_thread );

        // Interact with the user on the console.
        Console::command_loop( );
        Console::cleanup( );

        // FIXME: Should clean up the accept thread "nicely" (if it is still running).
        // FIXME: Should clean up the spool thread "nicely" (if it is still running).
        return 0;
    }
    catch( exception &e ) {
        Console::cleanup( );
        cout << "Exception in main: " << e.what( ) << "\n";
    }
    catch( ... ) {
        Console::cleanup( );
        cout << "Unknown exception in main(). Execution aborted.\n";
    }
    return 1;
}

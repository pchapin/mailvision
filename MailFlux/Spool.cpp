/*! \file    Spool.cpp
 *  \brief   Implementation of spool handling functions.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

// Standard C++
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
// #include <string>
#include <string.h>   // Clang 3.0 does not see memset and memcpy in <string>
#include <vector>

// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

// MailFlux
#include "ClientConnection.hpp"
#include "config.hpp"
#include "Console.hpp"
#include "Spool.hpp"

using namespace std;

// Anonymous namespace for module private items.
namespace {
    string spool_directory;
    pthread_mutex_t spool_lock = PTHREAD_MUTEX_INITIALIZER;

    /*!
     * Converts istrings to std::strings. It shouldn't be necessary to copy an istring just to
     * treat it like a standard string (for purposes of I/O). I'm not sure how to best handle
     * this. Do I have to make a special instantiation of the basic_ostream?
     */
    std::string to_string( const istring &is )
    {
        std::string result;

        for( char i : is ) {
            result.push_back( i );
        }
        return result;
    }


    /*!
     * Converts std::sgtrings to istrings. It shouldn't be necessary to copy a std::string just
     * to treat it like an istring (for purposes of I/O). I'm not sure how to best handle this.
     * Do I have to make a special instantiation of the basic_istream?
     */
    istring to_istring( const string &s )
    {
        istring result;

        for( char i : s ) {
            result.push_back( i );
        }
        return result;
    }


    //! Reads a single message out of the spool and prepares a Message object.
    Message read_message( const string &file_name )
    {
        Message result;
        ifstream input( file_name.c_str( ));
        string line;

        if( !input ) throw Spool::SpoolError( "Can't open message file" );

        // FIXME: This should be made more robust against file format errors.

        // Get the sender.
        getline( input, line );
        result.set_sender( to_istring( line ));
        getline( input, line );

        // Get the recipient list.
        while( getline( input, line ) && line != "=====" ) {
            result.add_recipient( to_istring( line ));
        }

        // Get the message text.
        while( getline( input, line )) {
            result.append_text( to_istring( line ));
        }
        return result;
    }


    //! Connects to the server that will deliver mail.
    int connect_server( )
    {
        int handle;

        std::string *next_server = Support::lookup_parameter( "NEXT_SERVER" );
        if( next_server == 0 )
            throw Spool::SpoolError( "No next server defined" );

        // FIXME: Change to gethostbyname to gethostbyname_r for thread safety.
        hostent *host_information = gethostbyname( next_server->c_str( ));
        if( host_information == 0 )
            throw Spool::SpoolError( "Unable to look up next server address" );

        handle = socket( PF_INET, SOCK_STREAM, 0 );
        if( handle == -1 )
            throw Spool::SpoolError( "Unable to create socket to connect with next server" );

        sockaddr_in server_address;
        memset( &server_address, 0, sizeof( server_address ));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons( 25 );
        memcpy( &server_address.sin_addr, host_information->h_addr_list[0], 4 );

        int result = connect( handle, (sockaddr *) &server_address, sizeof( server_address ));
        if( result == -1 ) {
            close( handle );
            throw Spool::SpoolError( "Unable to connect to next server" );
        }

        return handle;
    }


    /*!
     * This is the spool handling thread function. It wakes up periodically and tries to send
     * (or deliver) every message it finds in the spool.
     */
    void *spool_loop( void * )
    {
        vector<string> file_names;
        DIR *scan_state;
        dirent *directory_entry;

        for( ;; ) {
            // Catch all possible exceptions and keep going.
            try {
                sleep( 15 );
                file_names.clear( );

                // Scan the spool directory and make a list of all files.
                // FIXME: If an exception is thrown while the lock is held deadlock occurs.
                pthread_mutex_lock( &spool_lock );
                if(( scan_state = opendir( spool_directory.c_str( ))) == 0 ) {
                    Console::put_exception_line( "Can't scan spool directory" );
                }
                else {
                    while(( directory_entry = readdir( scan_state )) != 0 ) {
                        if( directory_entry->d_name[0] == '.' ) continue;
                        file_names.push_back( spool_directory + "/" + directory_entry->d_name );
                    }
                    closedir( scan_state );
                }
                pthread_mutex_unlock( &spool_lock );

                // For each message in the spool, create ClientConnection object and send it.
                vector<string>::size_type i;
                for( i = 0; i < file_names.size( ); ++i ) {
                    int socket_handle;
                    ostringstream message_formatter;

                    message_formatter << "Processing spool file '" << file_names[i] << "'";
                    Console::put_debug_line( message_formatter.str( ).c_str( ));

                    // FIXME: If there is an error sending the message, it should be requeued.
                    Message email = read_message( file_names[i] );
                    socket_handle = connect_server( );
                    ClientConnection forwarder( socket_handle, email );
                    forwarder.doSMTP( );
                    close( socket_handle );
                    unlink( file_names[i].c_str( ));
                }
            }
            catch( exception &e ) {
                Console::put_exception_line( e.what( ));
            }
            catch( ... ) {
                Console::put_exception_line( "Unexpected exception in spool thread" );
            }
        }
        return nullptr;
    }

} // End of anonymous namespace.


namespace Spool {

    //! Initialize the spool.
    /*!
     * This function initializes the in-memory data structures and the on-disk data structures
     * (if any) required by the spool. It also starts the spool monitoring thread that
     * periodically tries to send all messages that it finds in the spool. Note that this
     * function assumes that Support::read_config_files() has already been called.
     */
    void initialize( )
    {
        pthread_t spool_thread;

        // Get the spool directory name. Abort if there is no such name defined.
        string *temp = Support::lookup_parameter( "SPOOL" );
        if( temp == 0 ) throw SpoolError( "No spool directory specified" );
        spool_directory = *temp;

        ostringstream message_formatter;
        message_formatter << "Using spool directory of '" << spool_directory << "'";
        Console::put_debug_line( message_formatter.str( ).c_str( ));

        // Create the spool handling thread. The thread runs forever and is never terminated or
        // joined. This is probably not ideal.
        //
        // FIXME: Some plan for a clean shutdown of the spool thread should be devised.
        Console::put_debug_line( "Initializing spool handling thread" );
        pthread_create( &spool_thread, nullptr, spool_loop, nullptr );
        pthread_detach( spool_thread );
    }


    //! Add an email message to the spool.
    /*!
     * This function copies the given email message to non-volatile storage for later delivery.
     *
     * \param the_message The email message to add to the spool.
     */
    void add_message( const Message &the_message )
    {
        time_t raw_time;
        struct tm *cooked_time;

        // Build the file name based on the current date/time.
        raw_time = std::time( nullptr );
        cooked_time = localtime( &raw_time );

        ostringstream formatter;
        formatter << setfill( '0' );
        formatter << cooked_time->tm_year + 1900
                  << setw( 2 ) << cooked_time->tm_mon + 1
                  << setw( 2 ) << cooked_time->tm_mday;
        formatter << 'T';
        formatter << setw( 2 ) << cooked_time->tm_hour
                  << setw( 2 ) << cooked_time->tm_min
                  << setw( 2 ) << cooked_time->tm_sec;
        string file_name = spool_directory + "/" + formatter.str( ) + ".msg";

        ostringstream message_formatter;
        message_formatter << "Writing message to '" << file_name << "'";
        Console::put_debug_line( message_formatter.str( ).c_str( ));

        // Write the entire file to disk under the lock so that the spool handling thread never
        // tries to send a partially written message.
        //
        pthread_mutex_lock( &spool_lock );
        {
            ofstream output( file_name.c_str( ));

            if( !output ) {
                Console::put_exception_line( "Can't open spool file" );
            }
            else {

                // Sender
                output << to_string( the_message.get_sender( )) << "\n";
                output << "=====\n";

                // Recipients
                const vector<istring> &recipients = the_message.get_recipients( );
                vector<istring>::const_iterator p;
                for( p = recipients.begin( ); p != recipients.end( ); ++p ) {
                    output << to_string( *p ) << "\n";
                }
                output << "=====\n";

                // Message body.
                const vector<istring> &text = the_message.get_text( );
                for( p = text.begin( ); p != text.end( ); ++p ) {
                    output << to_string( *p ) << "\n";
                }
            }
        }
        pthread_mutex_unlock( &spool_lock );
    }

}


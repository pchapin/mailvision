/*! \file    ClientConnection.cpp
 *  \brief   Implementation of client-side SMTP connections.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

#include <cstring>
#include <stdexcept>

#include <unistd.h>
#include "istring.hpp"
#include "ClientConnection.hpp"
#include "Console.hpp"

using namespace std;

// ===============
// Private Methods
// ===============

//! Read a line of text from the connection.
istring ClientConnection::line_in( )
{
    istring result;

    while( true ) {
        if( buffer_index == buffer_size ) {
            buffer_size = read( socket_handle, buffer, MAX_BUFFER_SIZE );
            if( buffer_size == 0 || buffer_size == -1 ) break;
            buffer[buffer_size] = '\0';
            buffer_index = 0;
        }

        char ch = buffer[buffer_index++];
        if( ch == '\r' ) continue;
        if( ch == '\n' ) break;

        result += ch;
    }
    return result;
}


//! Write a line of text to the connection.
void ClientConnection::line_out( const char *line )
{
    if( line == nullptr )
        throw invalid_argument( "ServerConnection::line_out" );

    write( socket_handle, line, strlen( line ));
    write( socket_handle, "\r\n", 2 );
}

// ==============
// Public Methods
// ==============

//! Construct the object.
/*!
 * The constructor initializes SMTP state variables as well as the buffers used to hold the raw
 * data on the connection. This class assumes a connection with the server has been previously
 * established. \param handle The socket handle of the connection with the server.
 *
 * \param the_message A reference to the email message to send. Eventually this should be
 * generalized to support a collection of messages.
 */
ClientConnection::ClientConnection( int handle, const Message &the_message )
{
    if( handle < 0 )
        throw invalid_argument( "ClientConnection::ClientConnection" );

    buffer_size = 0;
    buffer_index = 0;
    socket_handle = handle;
}


//! Recover resources.
/*!
 * The destructor does not close the connection with the server. Since this connection was
 * established before the object was constructed, it should be closed after the object is
 * destroyed.
 */
ClientConnection::~ClientConnection( )
{
    // No actions needed.
}


//! Have an SMTP conversation with the server.
/*!
 * This method executes the full SMTP conversation with the server, sending as many mail
 * messages as necessary [currently only a single message is supported].
 */
void ClientConnection::doSMTP( )
{
    // TODO: Finish me!
}

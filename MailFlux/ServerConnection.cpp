/*! \file    ServerConnection.cpp
 *  \brief   Implementation of server-side SMTP connections.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include "istring.hpp"
#include "ServerConnection.hpp"
#include "Console.hpp"
#include "Spool.hpp"

using namespace std;

namespace {

    //! Exception for reporting problems with the client's SMTP conversation.
    class BadClientSMTP : public runtime_error {
    public:
        explicit BadClientSMTP( const string &message ) : runtime_error( message )
        { }
    };


    //! Return the email address on an SMTP line.
    /*!
     * Search the given line looking for the first '< ... >' enclosed email address. This
     * function is used to reliably extract email addresses out of MAIL and RCPT client
     * commands.
     *
     * \param from_sender A line of SMTP text from the sender (client).
     * \return A string containing just the email address (without the '<' and '>' delimiters).
     * \throw BadClientSMTP thrown if no email address found on the given line.
     */
    istring get_email_address( const istring &from_sender )
    {
        istring::size_type open_angle = from_sender.find_first_of( '<' );
        if( open_angle == istring::npos ) {
            throw BadClientSMTP( "Open angle ('<') expected but not found" );
        }
        istring::size_type close_angle = from_sender.find_last_of( '>' );
        if( close_angle == istring::npos ) {
            throw BadClientSMTP( "Close angle ('>') expected but not found" );
        }
        if( close_angle < open_angle ) {
            throw BadClientSMTP( "Malformed email address delimiters found" );
        }
        istring email_address = from_sender.substr( open_angle + 1, close_angle - open_angle - 1 );
        if( email_address.length( ) < 3 || email_address.find_first_of( '@' ) == istring::npos ) {
            throw BadClientSMTP( "Mailformed email address found" );
        }
        return email_address;
    }

}   // End of anonymous namespace.

// ===============
// Private Methods
// ===============

//! Read a line of text from the connection.
istring ServerConnection::line_in( )
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
void ServerConnection::line_out( const char *line )
{
    if( line == nullptr )
        throw invalid_argument( "ServerConnection::line_out" );

    write( socket_handle, line, strlen( line ));
    write( socket_handle, "\r\n", 2 );
}


//! Send a line of error information to the client.
/*!
 * Sends the given line to the client and also displays the line in MailFlux's asynchronous area
 * This is useful for showing the MailFlux user the SMTP error messages being produced by
 * MailFlux.
 *
 * \param line A null terminated string containing the error line to be sent to the client. This
 * string must be proper SMTP (in particular it must contain an appropriate three digit code at
 * the beginning of the line).
 */
void ServerConnection::error_out( const char *line )
{
    line_out( line );

    ostringstream formatter;
    formatter << "*** MailFlux replies: " << line;
    Console::put_line( formatter.str( ).c_str( ));
}


//! Returns an "interesting" line of SMTP text.
/*!
 * This function internally responds to all SMTP commands that can be issued at any time and
 * that don't change the state of the connection.
 */
istring ServerConnection::get_nontrivial_SMTP( )
{
    if( current_state == GETMESSAGE ) return line_in( );

    bool interesting_line = false;
    istring result;

    while( !interesting_line ) {
        result = line_in( );
        ostringstream formatter;
        formatter << "*** client: " << result.c_str( );
        Console::put_line( formatter.str( ).c_str( ));

        istring verb = result.substr( 0, 4 );
        if( verb == "NOOP" ) {
            line_out( "250 OK" );
        }
        else if( verb == "HELP" ) {
            // FIXME: Provide real help.
            line_out( "250 OK" );
        }
        else if( verb == "VRFY" || verb == "EXPN" ) {
            error_out( "502 Command not implemented" );
        }
        else {
            interesting_line = true;
        }
    }
    return result;
}


void ServerConnection::doWEHLO( const istring &from_sender )
{
    istring verb = from_sender.substr( 0, 4 );

    if( verb == "HELO" || verb == "EHLO" ) {
        line_out( "250 OK" );
        current_state = WMAIL;
    }
    else if( verb == "QUIT" ) {
        line_out( "221 MailFlux service ending" );
        current_state = DONE;
    }
    else if( verb == "RSET" ) {
        line_out( "250 OK" );
    }
    else if( verb == "MAIL" ||
             verb == "RCPT" ||
             verb == "DATA" ) {
        error_out( "503 Bad sequence of commands" );
    }
    else {
        error_out( "500 Syntax error" );
    }
}


void ServerConnection::doWMAIL( const istring &from_sender )
{
    istring verb = from_sender.substr( 0, 4 );

    if( verb == "MAIL" ) {
        try {
            email.set_sender( get_email_address( from_sender ));
            line_out( "250 OK" );
            current_state = WRCPT1;
        }
        catch( const BadClientSMTP &e ) {
            // Should a CLIENT ERROR message be issued before each syntax error?
            ostringstream formatter;
            formatter << "CLIENT ERROR: " << e.what( );
            Console::put_line( formatter.str( ).c_str( ));
            error_out( "500 Syntax error" );
        }
    }
    else if( verb == "QUIT" ) {
        line_out( "221 MailFlux service ending" );
        current_state = DONE;
    }
    else if( verb == "RSET" ) {
        email.clear( );
        line_out( "250 OK" );
    }
    else if( verb == "RCPT" || verb == "DATA" ) {
        error_out( "503 Bad sequence of commands" );
    }
    else {
        error_out( "500 Syntax error" );
    }
}


void ServerConnection::doWRCPT1( const istring &from_sender )
{
    istring verb = from_sender.substr( 0, 4 );

    if( verb == "RCPT" ) {
        try {
            email.add_recipient( get_email_address( from_sender ));
            line_out( "250 OK" );
            current_state = WRCPT2;
        }
        catch( const BadClientSMTP &e ) {
            ostringstream formatter;
            formatter << "CLIENT ERROR: " << e.what( );
            Console::put_line( formatter.str( ).c_str( ));
            error_out( "500 Syntax error" );
        }
    }
    else if( verb == "QUIT" ) {
        line_out( "221 MailFlux service ending" );
        current_state = DONE;
    }
    else if( verb == "RSET" ) {
        email.clear( );
        line_out( "250 OK" );
        current_state = WMAIL;
    }
    else if( verb == "MAIL" || verb == "DATA" ) {
        error_out( "503 Bad sequence of commands" );
    }
    else {
        error_out( "500 Syntax error" );
    }
}


void ServerConnection::doWRCPT2( const istring &from_sender )
{
    istring verb = from_sender.substr( 0, 4 );

    if( verb == "RCPT" ) {
        try {
            email.add_recipient( get_email_address( from_sender ));
            line_out( "250 OK" );
        }
        catch( const BadClientSMTP &e ) {
            ostringstream formatter;
            formatter << "CLIENT ERROR: " << e.what( );
            Console::put_line( formatter.str( ).c_str( ));
            error_out( "500 Syntax error" );
        }
    }
    else if( verb == "QUIT" ) {
        line_out( "221 MailFlux service ending" );
        current_state = DONE;
    }
    else if( verb == "RSET" ) {
        email.clear( );
        line_out( "250 OK" );
        current_state = WMAIL;
    }
    else if( verb == "DATA" ) {
        line_out( "354 Start mail input; end with <CRLF>.<CRLF>" );
        current_state = GETMESSAGE;
    }
    else if( verb == "MAIL" ) {
        error_out( "503 Bad sequence of commands" );
    }
    else {
        error_out( "500 Syntax error" );
    }
}


void ServerConnection::doGETMESSAGE( const istring &from_sender )
{
    if( from_sender == "." ) {
        Spool::add_message( email );
        line_out( "250 OK" );
        current_state = WQUIT;
    }
    else {
        email.append_text( from_sender );
    }
}


void ServerConnection::doWQUIT( const istring &from_sender )
{
    istring verb = from_sender.substr( 0, 4 );

    if( verb == "QUIT" ) {
        line_out( "221 MailFlux service ending" );
        current_state = DONE;
    }
    else if( verb == "RSET" ) {
        email.clear( );
        line_out( "250 OK" );
        current_state = WMAIL;
    }
    else if( verb == "MAIL" || verb == "RCPT" || verb == "DATA" ) {
        error_out( "503 Bad sequence of commands" );
    }
    else {
        error_out( "500 Syntax error" );
    }
}

// ==============
// Public Methods
// ==============

//! Construct the object.
/*!
 * The constructor initializes SMTP state variables as well as the buffers used to hold the raw
 * data on the connection. This class assumes a connection with the client has been previously
 * established.
 *
 * \param handle The socket handle of the connection with the client. 
 */
ServerConnection::ServerConnection( int handle )
{
    if( handle < 0 )
        throw invalid_argument( "ServerConnection::ServerConnection" );

    buffer_size = 0;
    buffer_index = 0;
    socket_handle = handle;
}


//! Recover resources.
/*!
 * The destructor does not close the connection with the client. Since this connection was
 * established before the object was constructed, it should be closed after the object is
 * destroyed.
*/
ServerConnection::~ServerConnection( )
{
    // No actions needed.
}


//! Have an SMTP conversation with the client.
/*! 
 * This method executes the full SMTP conversation with the client, accepting as many mail
 * messages as the client wants.
*/
void ServerConnection::doSMTP( )
{
    line_out( "220 MailFlux v0.0" );
    current_state = WEHLO;
    while( current_state != DONE ) {
        istring from_sender = get_nontrivial_SMTP( );
        switch( current_state ) {
            case WEHLO     :
                doWEHLO( from_sender );
                break;
            case WMAIL     :
                doWMAIL( from_sender );
                break;
            case WRCPT1    :
                doWRCPT1( from_sender );
                break;
            case WRCPT2    :
                doWRCPT2( from_sender );
                break;
            case GETMESSAGE:
                doGETMESSAGE( from_sender );
                break;
            case WQUIT     :
                doWQUIT( from_sender );
                break;
            case DONE      :
                break;
        }
    }
}

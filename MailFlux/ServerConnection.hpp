/*! \file    ServerConnection.hpp
 *  \brief   Interface to server-side SMTP connections.
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef SERVERCONNECTION_HPP
#define SERVERCONNECTION_HPP

#include "Message.hpp"
#include "istring.hpp"

//! Class to represent a server-oriented endpoint.
/*!
 * Instances of this class are execute a server side SMTP conversation with a given client.
 * \todo Provide more details.
 */
class ServerConnection {
public:
    explicit ServerConnection( int handle );

    ~ServerConnection( );

    void doSMTP( );

private:
    enum state {
        WEHLO, WMAIL, WRCPT1, WRCPT2, GETMESSAGE, WQUIT, DONE
    };

    static const int MAX_BUFFER_SIZE = 128;
    char buffer[MAX_BUFFER_SIZE + 1]; //!< Holds raw text from the client.
    int buffer_size;               //!< Amount of valid text in buffer.
    int buffer_index;              //!< "Current point" in buffer.
    int socket_handle;             //!< Client socket.
    state current_state;             //!< Current state of the SMTP transaction.
    Message email;                     //!< Accumulating email message.

    istring line_in( );

    void line_out( const char *line );

    void error_out( const char *line );

    istring get_nontrivial_SMTP( );

    // These functions handle the various SMTP states.
    void doWEHLO( const istring & );

    void doWMAIL( const istring & );

    void doWRCPT1( const istring & );

    void doWRCPT2( const istring & );

    void doGETMESSAGE( const istring & );

    void doWQUIT( const istring & );

    // Make copying illegal.
    ServerConnection( const ServerConnection & );

    ServerConnection &operator=( const ServerConnection & );
};

#endif

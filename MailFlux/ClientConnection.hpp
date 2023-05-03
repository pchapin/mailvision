/*! \file    ClientConnection.hpp
 *  \brief   Interface to the client connection class.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include "Message.hpp"
#include "istring.hpp"

//! Class to represent a client-oriented endpoint.
/*!
 * Instances of this class execute a client side SMTP conversation with a given server.
 * \todo Provide more details.
 */
class ClientConnection {
public:
    ClientConnection( int handle, const Message &the_message );

    ~ClientConnection( );

    void doSMTP( );

private:
    static const int MAX_BUFFER_SIZE = 128;
    char buffer[MAX_BUFFER_SIZE + 1]; //!< Holds raw text from the client.
    int buffer_size;               //!< Amount of valid text in buffer.
    int buffer_index;              //!< "Current point" in buffer.
    int socket_handle;             //!< Client socket.

    istring line_in( );

    void line_out( const char *line );

    // Make copying illegal.
    ClientConnection( const ClientConnection & );

    ClientConnection &operator=( const ClientConnection & );
};

#endif

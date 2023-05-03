/*! \file    Spool.hpp
 *  \brief   Interface to spool handling functions.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

#ifndef SPOOL_HPP
#define SPOOL_HPP

#include <stdexcept>
#include "Message.hpp"

//! Namespace for spool handling facilities.
/*!
 * The message spool is a storage area where email messages are placed while awaiting delivery.
 * MailFlux currently does not have any concept of local delivery and instead must forward all
 * messages to another mail server. The spool handling functions in this namespace deal with
 * this.
 */
namespace Spool {

    //! Exception for various kinds of runtime problems related to the spool.
    class SpoolError : public std::runtime_error {
    public:
        explicit SpoolError( const char *message ) : std::runtime_error( message )
        { }
    };

    void initialize( );

    void add_message( const Message &the_message );
}

#endif


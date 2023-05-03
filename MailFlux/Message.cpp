/*! \file    Message.cpp
 *  \brief   Implementation of a class that abstracts email messages.
 *  \author  Peter Chapin <chapinp@proton.me>
 */

#include "Message.hpp"


//! Erases the sender, recipient list, and message text for this Message object.
/*!
 * The object can be reused after a call to clear(). Note that the constructor of this class
 * also clears the object as if a call to this method is made.
*/
void Message::clear( )
{
    sender.clear( );
    recipients.clear( );
    text.clear( );
}


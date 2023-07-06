/*! \file    Message.hpp
 *  \brief   Interface to a class that abstracts email messages.
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include "istring.hpp"

//! Class representing a single email message.
/*!
 * This class is an abstraction of a single email message. In addition to containing the text of
 * the message, this class also maintains a record of the message sender and list of recipients.
 * Note that the structure of the email message (and even its validity) is not considered by
 * this class (at this time). In theory it should never be necessary to look at the message
 * itself because that's at a different protocol layer than SMTP. In practice it probably will
 * be necessary to examine the message at some point.
 */
class Message {
public:
    //! Set the message sender.
    /*!
     * This method allows you to specify the email address of the sender. There is only one
     * sender.
     *
     * \param the_sender The email address representing the entity originating this message.
     */
    void set_sender( const istring &the_sender )
    { sender = the_sender; }


    //! Add an additional recipient to this message's recipient list.
    /*!
     * A message can have multiple recipients. Currently this class makes no attempt to deal
     * with redundant copies of a recipient on the list.
     *
     * \param the_recipient The email address of the new recipient.
     */
    void add_recipient( const istring &the_recipient )
    { recipients.push_back( the_recipient ); }


    //! Add a line of text to the message itself.
    /*!
     * The structure and validity of the message is not checked. Note that the line added should
     * not have any line ending delimiters (CR, LF, etc). Such delimiters should be added when
     * the message is formatted for saving or transmission.
     *
     * \param line The line of text to save at the end of the current message.
     */
    void append_text( const istring &line )
    { text.push_back( line ); }

    void clear( );

    // Accessor methods just return references to internal data. This should be made better.

    //! Return the email address of the message sender.
    const istring &get_sender( ) const
    { return sender; }

    //! Return a list of recipient email addresses.
    const std::vector<istring> &get_recipients( ) const
    { return recipients; }

    //! Return the message text.
    const std::vector<istring> &get_text( ) const
    { return text; }

private:
    // The sender and recipients should be some kind of email address abstract type.
    istring sender;
    std::vector<istring> recipients;
    std::vector<istring> text;
};

#endif

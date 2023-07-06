/*! \file    Console.cpp
 *  \brief   Implementation of the console.
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 *
 * All console handling functions are in namespace Console. They make use of the curses library.
 * Before any function uses that library it first grabs a master mutex on the assumption that
 * curses is not thread-safe.
 */

#include <curses.h>
#include <pthread.h>
#include <sys/select.h>
#include "Console.hpp"

using namespace std;

// Anonymous namespace for internal items.
namespace {

    //! Curses lock to ensure mutually exclusive access to curses library.
    pthread_mutex_t curses_lock = PTHREAD_MUTEX_INITIALIZER;

    //! Class for using RAI idiom on the curses_lock.
    class CursesMutex {
    public:
        CursesMutex( )
        { pthread_mutex_lock( &curses_lock ); }

        ~CursesMutex( )
        { pthread_mutex_unlock( &curses_lock ); }
    };

    //! Set to true between initialization and clean up.
    bool is_initialized = false;

    WINDOW *asynchronous;  //!< Curses window for asynchronous messages.
    WINDOW *interaction;   //!< Curses window for user dialog.

    //! Display a banner.
    /*!
     * This function executes in the interactive thread. It outputs a welcome banner to the
     * interactive display area.
     */
    void banner( )
    {
        CursesMutex lock;

        wprintw( interaction, "MailFlux v0.0\n" );
        wprintw( interaction, "Copyright 2009 by Peter C. Chapin\n\n" );
        wrefresh( interaction );
    }

    //! Get a line of text from the interactive display area.
    /*!
     * This function executes in the interactive thread. It accepts a line of text at the
     * interactive display area prompt and returns the text of that line.
     */
    string get_line( )
    {
        char buffer[128];
        fd_set the_set;

        FD_ZERO( &the_set );
        FD_SET( 0, &the_set );  // Wait for stdin.
        select( 1, &the_set, nullptr, nullptr, nullptr );
        {
            CursesMutex lock;
            wgetstr( interaction, buffer );
        }
        return string( buffer );
    }

} // anonymous namespace


namespace Console {

    // ----------------
    // Public Functions
    // ----------------


    //! Initializes the console.
    /*!
     * This function must be called before any use is made of the console library. Multiple
     * calls of this function have no effect. It initializes curses. Note that this function
     * does not obtain the curses lock because at the time it is called no other threads should
     * be attempting to use the console library.
     */
    void initialize( )
    {
        if( is_initialized ) return;

        initscr( );

        // Set up the two windows.
        asynchronous = newwin(( LINES - 1 ) / 2, 0, 0, 0 );
        wsetscrreg( asynchronous, 0, (( LINES - 1 ) / 2 ) - 1 );
        scrollok( asynchronous, TRUE );
        idlok( asynchronous, TRUE );

        interaction = newwin(( LINES - 1 ) / 2, 0, (( LINES - 1 ) / 2 ) + 1, 0 );
        wsetscrreg( interaction, 0, (( LINES - 1 ) / 2 ) - 1 );
        scrollok( interaction, TRUE );
        idlok( interaction, TRUE );

        // Draw the separator.
        attron( A_REVERSE );
        for( int i = 0; i < COLS; ++i ) {
            mvaddch(( LINES - 1 ) / 2, i, ' ' );
        }
        attroff( A_REVERSE );
        refresh( );

        is_initialized = true;
    }


    //! Cleans up the console.
    /*!
     * This function should be called before the program exits. After it is called no other
     * calls should be made to the console library. This function shuts down curses and cleans
     * up the console display. If this function is called without first calling initialize,
     * there is no effect. Note that this function does not obtain the curses lock because at
     * the time it is called no other threads should be attempting to use the console library.
     */
    void cleanup( )
    {
        if( !is_initialized ) return;

        endwin( );
        is_initialized = false;
    }


    //! Outputs a line of text to the asynchronous display area.
    /*!
     * \param line Pointer to a null terminated string to print. This string should not contain
     * any embedded '\n' characters (or other control characaters), but no checking for this is
     * done.
     */
    void put_line( const char *line )
    {
        CursesMutex lock;
        wprintw( asynchronous, "%s\n", line );
        wrefresh( asynchronous );
    }


    //! Outputs a line of warning information to the asynchronous display area.
    /*!
     * This function prints warning messages. Such messages are not necessarily errors but do
     * indicate that MailFlux may behave badly in the future. This function is different than
     * put_line in that it may format the message in a special way to draw attention to it
     * (different color, blinking text, etc).
     *
     * \param line Pointer to a null terminated string to print. This string should not contain
     * any embedded '\n' characters (or other control characaters), but no checking for this is
     * done.
     */
    void put_warning_line( const char *line )
    {
        CursesMutex lock;
        wprintw( asynchronous, "Warning: %s\n", line );
        wrefresh( asynchronous );
    }


    //! Outputs a line of error/exception information to the asynchronous display area.
    /*!
     * This function prints messages from exception handlers or other error messages. It is
     * different than put_line in that it may format the message in a special way to draw
     * attention to it (different color, blinking text, etc).
     *
     * \param line Pointer to a null terminated string to print. This string should not contain
     * any embedded '\n' characters (or other control characaters), but no checking for this is
     * done.
     */
    void put_exception_line( const char *line )
    {
        CursesMutex lock;
        wprintw( asynchronous, "ERROR: %s\n", line );
        wrefresh( asynchronous );
    }


    //! Outputs a line of debugging information to the asynchronous display area.
    /*! 
     * This function prints debug messages and other messages about the program's internal state
     * that might not be interesting to ordinary users. Currently these messages are printed
     * unconditionally, but in the future it may be possible to enable/disable the display of
     * these messages.
     *
     * \param line Pointer to a null terminated string to print. This string should not contain
     * any embedded '\n' characters (or other control characaters), but no checking for this is
     * done.
     */
    void put_debug_line( const char *line )
    {
        CursesMutex lock;
        wprintw( asynchronous, "DEBUG: %s\n", line );
        wrefresh( asynchronous );
    }


    //! Interact with the user.
    /*!
     * This function accepts and handles console commands from the user. It executes in its own
     * thread, and so should be called only once.
     */
    void command_loop( )
    {
        try {
            banner( );

            string line;
            while( 1 ) {
                {
                    CursesMutex lock;
                    wprintw( interaction, "> " );
                    wrefresh( interaction );
                }
                line = get_line( );
                if( line == "quit" ) return;
            }
        }
        catch( exception &e ) {
            put_exception_line( e.what( ));
        }
        catch( ... ) {
            put_exception_line( "Unknown exception in Console::iteract()" );
        }
    }

} // Console

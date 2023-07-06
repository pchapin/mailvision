/*! \file    config.cpp
 *  \brief   ???
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 */

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <fstream>
#include <list>
#include "config.hpp"

using namespace std;

namespace Support {

    struct dictionary_entry {
        string name;
        string value;
        bool   personalized;

        // I don't believe this is necessary, but to be on the safe side...
        dictionary_entry( ) : personalized( false ) { }
    };

    // This is the dictionary. This should really be upgraded to a map someday. The reason it's
    // a list is because this is "legacy" C code that has been lightly gone over for use with
    // C++.
    //
    static list<dictionary_entry> the_dictionary;

    // The character used to mark the start of a comment in a
    // configuration file. Comments run to the end of the line.
    //
    const char comment_char = '#';

    // ===========================
    // Internally Linked Functions
    // ===========================


    /*!
     * Return true if the argument is a white space character. Otherwise it returns false. This
     * function defines the meaning of "white space" in a configuration file. I you want to make
     * changes to what is and is not considered white space, change only this function.
     */
    static bool is_white( char ch )
    {
        if( ch == ' ' || ch == '\t' || ch == '\f' ) return true;
        return false;
    }


    /*!
     * Return true if the given string is blank. A blank string is one that has nothing in it,
     * has only white space in it, or contains only a possibly indented comment.
    */
    static bool blank_line( const string &the_line )
    {
        const char *p = the_line.c_str( );

        while( *p ) {
            if( *p == comment_char ) return true;
            if(  !is_white( *p ) ) return false;
            p++;
        }
        return true;
    }


    /*!
     * Remove trailing white space or comments from the given string. There is no error checking
     * on the string handling.
    */
    static void trim_string( string &the_string )
    {
        const char *start = the_string.c_str( );
        const char *p;

        // First let's see if there is a comment in this string. If so, kill it.
        if( ( p = strchr( start, comment_char ) ) != 0 ) {
            the_string = the_string.substr( 0, p - start );
            start = the_string.c_str( );
            // Refresh 'start' since the trim operation invalidated it.
        }
    
        // If the string has zero size we are done.
        if( the_string.size( ) == 0 ) return;

        // Next let's jump to the end and back up over white space.
        p = strchr( start, '\0' );
        --p;
        while( is_white( *p ) && p != start ) p--;

        // If the loop above broke because of a non-white space, retract one step.
        if( !is_white( *p ) ) p++;

        // Chop, hack, and slash.
        the_string = the_string.substr( 0, p - start );
    }


    /*!
     * Note that this function does not do any error checking regarding the handling of string
     * objects or the dictionary. Note also that it allows for a null name. A line in the form:
     * "=VALUE" assigns the string "VALUE" to the name "". This might be useful for some
     * programs. This behavior is currently undocumented.
    */
    static void analyze_line( const string &the_line, bool personalized )
    {
        const char *p = the_line.c_str( );
        bool            happy = false;  // =true if this line has the right syntax.
        dictionary_entry temp;

        temp.personalized = personalized;

        if( blank_line( the_line ) ) return;
        while( *p && is_white( *p ) ) p++;

        // Copy the first word into the temporary dictionary entry.
        while( *p && *p != '=' && !is_white( *p ) ) {
            temp.name.append( 1, *p++ );
        }
        
        while( *p && is_white( *p ) ) p++;

        if( *p++ == '=' ) {
            // Ok, the syntax is good enough.
            happy = true;

            while( *p && is_white( *p ) ) p++;

            // Copy to the end of the line into the temporary dictionary entry. Note that this
            // behavior includes embeded white spaces in the value, and it also includes
            // trailing white spaces.
            // 
            while( *p ) temp.value.append( 1, *p++ );
            trim_string( temp.value );
        }

        // If the syntax looks good, then lets add this information to the dictionary. Otherwise
        // we'll just ignore this line.
        // 
        if( happy ) {
            list<dictionary_entry>::iterator stepper = the_dictionary.begin( );

            while( stepper != the_dictionary.end( ) ) {
                if( stepper->name == temp.name ) {
                    *stepper = temp;
                    break;
                }
                stepper++;
            }

            if( stepper == the_dictionary.end( ) ) {
                the_dictionary.push_back( temp );
            }
        }
    }


    /*!
     * There is no error checking on the handling of string or the reading of the file.
     */
    static void process_config_file( ifstream &the_file, bool personalized )
    {
        while( true ) {
            string line;

            getline( the_file, line );
            if( !the_file && line.size( ) == 0 ) return;
            analyze_line( line, personalized );
        }
    }

    // ===========================
    // Externally Linked Functions
    // ===========================

    //! Reads the master and personal configuration files.
    /*!
     * This function reads the given configuration file and loads a dictionary of (name, value)
     * pairs. The file consists of lines in the format NAME=VALUE. Blank lines and all material
     * after a '#' character are ignored. Leading white space, trailing white space, and spaces
     * around the '=' are ignored. Spaces are not allowed in NAME, but spaces are allowed in
     * VALUE. Lines can be arbitrary long. Lines in the wrong format are ignored.
     *
     * If the name PERSONAL_CONFIGURATION is defined after the master configuration file is
     * processed, this function then tries to open and process the personal configuration file
     * as well. Note that it is not necessary for PERSONAL_CONFIGURATION to be defined in the
     * master configuration file. It can be added ahead of time using register_parameter().
     *
     * \param path The name of the master configuration file.
     *
     * \return There is no error return. This function does not complain if it can't open the
     * configuration files.
     */
    void read_config_files( const char *path )
    {
        ifstream primary_config;
        ifstream secondary_config;

        primary_config.open( path );
        if( primary_config )
            process_config_file( primary_config, false );

        string *secondary_config_name =
            lookup_parameter( "PERSONAL_CONFIGURATION" );

        if( secondary_config_name != 0 ) {
            secondary_config.open( secondary_config_name->c_str( ) );
            if( !secondary_config ) return;
            process_config_file( secondary_config, true );
        }
    }


    //! Returns the value associated with the given name.
    /*!
     * The dictionary of (name, value) pairs is searched using a case sensitive comparision on
     * 'name'. The pointer returned may be invalidated if more items are added to the dictionary
     * by register_parameter( ).
     *
     * \param name The name to look up.
     *
     * \return A pointer to the associated value. If there is no value associated with the given
     * name, a NULL pointer is returned. The object pointed at by the return value can be
     * modified. When that object is looked up again, the new value will be returned.
     */
    string *lookup_parameter( const char *name )
    {
        list<dictionary_entry>::iterator stepper = the_dictionary.begin( );

        while( stepper != the_dictionary.end( ) ) {
            if( stepper->name == name ) return &stepper->value;
            stepper++;
        }
        return 0;
    }


    //! Install a (name, value) pair into the dictionary.
    /*!
     * This function can be used to install program default values into the (name, value)
     * dictionary before read_config_files() is called. This simplifies the program by allowing
     * places where configuration information is needed to ignore the possibility of default
     * values.
     *  
     * \param name The name to add to the dictionary. The string pointed at by this parameter is
     * copied.
     *
     * \param value The value to associate with the given name. The string pointed at by this
     * parameter is copied. If the given name is already in the dictionary, this new value
     * overwrites the old value.
     *
     * \param personalized Flags this (name, value) entry as a personal entry. The
     * write_config_file function will write this entry to the personal configuration file.
     */
    void register_parameter(
        const char *name,
        const char *value,
        bool personalized
    )
    {
        dictionary_entry temp;

        temp.name         = name;
        temp.value        = value;
        temp.personalized = personalized;

        list<dictionary_entry>::iterator stepper = the_dictionary.begin( );
        
        while( stepper != the_dictionary.end( ) ) {
            if( stepper->name == name ) {
                *stepper = temp;
                break;
            }
            stepper++;
        }

        if( stepper == the_dictionary.end( ) ) {
            the_dictionary.push_back( temp );
        }
    }


    //! Write the personal configuration file.
    /*!
     * This function writes out only items that have been personalized to the personal
     * configuration file. It looks up the name of the personal configuration file under the
     * name PERSONAL_CONFIGURATION in the dictionary. If no personal configuration file is
     * specified this function has no effect.
     */
    void write_config_file( )
    {
        string *personal_config_name =
            lookup_parameter( "PERSONAL_CONFIGURATION" );
        if( personal_config_name == 0 ) return;

        ofstream config_file( personal_config_name->c_str( ) );
        if( !config_file ) return;

        list<dictionary_entry>::iterator stepper;

        for( stepper  = the_dictionary.begin( );
             stepper != the_dictionary.end( );
             stepper++ ) {

            if( !stepper->personalized ) continue;
            config_file << stepper->name << "=" << stepper->value << endl;
        }
    }

} // End of namespace Support.

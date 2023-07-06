/*! \file    ClientConnection.cpp
 *  \brief   Case insensitive standard strings.
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef ISTRING_HPP
#define ISTRING_HPP

#include <cctype>
#include <string>

//! Character traits type with case insensitive comparisons.
/*!
 * In order to facilitate doing case insensitive comparisons in various string operations, a
 * specialization of std::string is provided using this customized character traits type. This
 * type inherits from the standard character traits and only provides new implementations for
 * the operations pertaining to character comparisons.
 */
struct ichar_traits : std::char_traits<char> {

  static bool eq( const char_type &c1, const char_type &c2 )
    { return( std::tolower( c1 ) == std::tolower( c2 ) ); }

  static bool lt( const char_type &c1, const char_type &c2 )
    { return( std::tolower( c1 ) < std::tolower( c2 ) ); }

  static int compare( const char_type *s1, const char_type *s2, size_t n )
  {
    for( size_t i = 0; i < n; ++i ) {
      if( std::tolower( *s1 ) < std::tolower( *s2 ) ) return( -1 );
      if( std::tolower( *s1 ) > std::tolower( *s2 ) ) return( +1 );
      ++s1; ++s2;
    }
    return( 0 );
  }

  static const char_type *find( const char_type *s, size_t n, const char_type &a )
  {
    const char_type *result = nullptr;
    for( size_t i = 0; i < n; ++i ) {
      if( std::tolower( *s ) == std::tolower( a ) ) {
        result = s;
        break;
      }
      ++s;
    }
    return( result );
  }

  static bool eq_int_type( const int_type &c1, const int_type &c2 )
    { return( std::tolower( c1 ) == std::tolower( c2 ) ); }

};

//! Definition of case insensitive string type.
typedef std::basic_string<char, ichar_traits> istring;

#endif

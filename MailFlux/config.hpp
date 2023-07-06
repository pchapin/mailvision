/*! \file    config.hpp
 *  \brief   ???
 *  \author  Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

/*! \file config.hpp
 *  \brief Functions for managing an application-wide configuration file.
 *
 * This module simplifies the task of fetching and using configurable parameters. It reads two
 * layer hierarchy of configuration files and makes a dictionary of (name, value) pairs
 * available to the application. The application can install default (name, value) pairs into
 * the dictionary and then allow the configuration files to override the defaults as desired.
 * This component can also rewrite the lower level (personal) configuration file with updated
 * (name, value) pairs making it easy for the application to save an updated configuration.
 */

#include <string>

namespace Support {

    void read_config_files( const char *path );

    std::string *lookup_parameter( const char *name );

    void register_parameter(
            const char *name,
            const char *value,
            bool personalized
    );

    void write_config_file( );
}

#endif


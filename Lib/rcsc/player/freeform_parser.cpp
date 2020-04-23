// -*-c++-*-

/*!
  \file freeform_parser.cpp
  \brief coach's freeform message parser Source File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "freeform_parser.h"

#include "world_model.h"

#include <rcsc/common/player_param.h>
#include <rcsc/common/logger.h>

#include <cstring>

namespace rcsc {


/*-------------------------------------------------------------------*/
/*!

*/
FreeformParser::FreeformParser( WorldModel & world )
    : M_world( world )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
int
FreeformParser::parse( const char * msg )
{
    if ( std::strncmp( msg, "(player_types ", 14 ) != 0 )
    {
        // unsupported message type
        return 0;
    }

    int total_read = 14;
    msg += 14;

    while ( *msg != '\0' )
    {
        if ( *msg == ')' )
        {
            total_read += 1;
            ++msg;
            break;
        }

        int unum = 0;
        int type = -1;
        int n_read = 0;

        if ( std::sscanf( msg, " ( %d %d ) %n ",
                          &unum, &type, &n_read ) != 2 )
        {
            std::cerr << "***ERROR*** FreeformParser::parse()"
                  << " Illegal message [" << msg << "]"
                  << std::endl;
            dlog.addText( Logger::SENSOR,
                          __FILE__" (parse) Illegal message [%s]",
                          msg );
            return total_read;
        }

        if ( unum < 1 || 11 < unum )
        {
            std::cerr << "***ERROR*** FreeformParser::parse()"
                  << " Illegal uniform number [" << msg << "]"
                  << std::endl;
            dlog.addText( Logger::SENSOR,
                          __FILE__" (parse) Illegal uniform number [%s]",
                          msg );
            return total_read;
        }

        int id = type;
        if ( id != Hetero_Unknown
             && id < 0
             && PlayerParam::i().playerTypes() <= id )
        {
            std::cerr << "***ERROR*** FreeformParser::parse()"
                  << " Illegal player type [" << msg << "]"
                  << std::endl;
            dlog.addText( Logger::SENSOR,
                          __FILE__" (parse) Illegal player type [%s]",
                          msg );
            return total_read;
        }

        total_read += n_read;
        msg += n_read;

        M_world.setTheirPlayerType( unum, id );
    }

    return total_read;
}


} // end namespace rcsc

// -*-c++-*-

/*!
  \file holder.cpp
  \brief rcg data holder Source File.
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

#include "holder.h"

#include "reader.h"
#include "parser_v4.h"
#include "parser_v5.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <iostream>

namespace rcsc {
namespace rcg {

/*-------------------------------------------------------------------*/
/*!

*/
bool
Holder::addDispInfo( const dispinfo_t & dinfo )
{
    setLogVersion( REC_VERSION_2 );

    switch ( ntohs( dinfo.mode ) ) {
    case SHOW_MODE:
        return addShowInfo( dinfo.body.show );
        break;
    case MSG_MODE:
        return addMsgInfo( dinfo.body.msg.board,
                           std::string( dinfo.body.msg.message ) );
        break;
    case DRAW_MODE:
        return addDrawInfo( dinfo.body.draw );
        break;
    default:
        std::cerr << __FILE__ << ':' << __LINE__
                  << " detect unsupported mode ["
                  << static_cast< int >( ntohs( dinfo.mode ) ) << ']'
                  << std::endl;
        break;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Holder::addDispInfo2( const dispinfo_t2 & dinfo2 )
{
    setLogVersion( REC_VERSION_3 );

    switch ( ntohs( dinfo2.mode ) ) {
    case SHOW_MODE:
        return addShowInfo2( dinfo2.body.show );
        break;
    case MSG_MODE:
        return addMsgInfo( dinfo2.body.msg.board,
                           std::string( dinfo2.body.msg.message ) );
        break;
    case PT_MODE:
        return addPlayerType( dinfo2.body.ptinfo );
        break;
    case PARAM_MODE:
        return addServerParam( dinfo2.body.sparams );
        break;
    case PPARAM_MODE:
        return addPlayerParam( dinfo2.body.pparams );
        break;
    default:
        std::cerr << __FILE__ << ':' << __LINE__
                  << " detect unsupported mode ["
                  << static_cast< int >( ntohs( dinfo2.mode ) ) << ']'
                  << std::endl;
        break;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Holder::addDisp3( const std::string & msg )
{
    setLogVersion( REC_VERSION_4 );

    Reader reader( *this );
    ParserV4 parser;

    return parser.parseLine( 0, msg, reader );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Holder::addDisp4( const std::string & msg )
{
    setLogVersion( REC_VERSION_5 );

    Reader reader( *this );
    ParserV5 parser;

    return parser.parseLine( 0, msg, reader );
}

} // end namespace
} // end namespace

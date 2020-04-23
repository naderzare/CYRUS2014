// -*-c++-*-

/*!
  \file audio_sensor.cpp
  \brief audio message analyzer Source File
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

#include "audio_sensor.h"

#include "freeform_parser.h"

#include <rcsc/common/say_message_parser.h>
#include <rcsc/common/logger.h>
#include <rcsc/math_util.h>

#include <cstdio>
#include <cstring>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

 */
AudioSensor::AudioSensor()
    : M_teammate_message_time( -1, 0 )
    , M_opponent_message_time( -1, 0 )
    , M_freeform_message_time( -1, 0 )
    , M_trainer_message_time( -1, 0 )
{
    M_freeform_message.reserve( 256 );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::addParser( boost::shared_ptr< SayMessageParser > parser )
{
    if ( ! parser )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " ***ERROR*** AudioSensor::addSayMessageParser()"
                  << " NULL parser object."
                  << std::endl;
        return;
    }

    if ( M_say_message_parsers.find( parser->header() )
         != M_say_message_parsers.end() )
    {
        std::cerr << __FILE__ << ":" << __LINE__
                  << " ***ERROR*** AudioSensor::addSayMessageParser()"
                  << " parser for [" << parser->header()
                  << "] is already registered."
                  << std::endl;
        return;
    }

    M_say_message_parsers.insert( ParserMap::value_type( parser->header(),
                                                         parser ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::removeParser( const char header )
{
    ParserMap::iterator it = M_say_message_parsers.find( header );
    if ( it == M_say_message_parsers.end() )
    {
        std::cerr << "***WARNING*** AudioSensor::removeParser()"
                  << " header [" << header
                  << "] is not registered."
                  << std::endl;
        return;
    }

    M_say_message_parsers.erase( it );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::setFreeformParser( boost::shared_ptr< FreeformParser > parser )
{
    M_freeform_parser = parser;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::parsePlayerMessage( const char * msg,
                                 const GameTime & current )
{
    /*
      players' communication audio format
      // from other
      v7-: (hear <TIME> <DIR> <MSG>)
      v7:  (hear <TIME> <DIR> "<MSG>")
      v8+: (hear <TIME> <DIR> our <UNUM> "<MSG>")
      (hear <TIME> <DIR> opp "<MSG>")
      (hear <TIME> our)
      (hear <TIME> opp)
    */

    long cycle = 0;
    double dir = 0.0;
    int unum = 0;
    char sender[8];
    int n_read = 0;

    // v8+ complete message
    if ( std::sscanf( msg, " (hear %ld %lf %7[^ ] %d %n ",
                      &cycle, &dir, sender, &unum, &n_read ) != 4 )
    {
        std::cerr << "***ERROR*** AudioSensor::parsePlayerMessage()"
                  << " heard unsupported message. [" << msg << "]"
                  << std::endl;
        return;
    }
    msg += n_read;

    while ( *msg == ' ' ) ++msg;

    char end_char = ')';
    if ( *msg == '\"' )
    {
        end_char = '\"';
        ++msg;
    }

    HearMessage message;
    message.unum_ = unum;
    message.dir_ = dir;
    message.str_ = msg;

    std::string::size_type pos = message.str_.rfind( end_char );
    if ( pos == std::string::npos )
    {
        std::cerr << "***ERROR*** AudioSensor::parsePlayerMessage."
                  << " Illegal message. [" << msg << ']'
                  << std::endl;
        return;
    }
    message.str_.erase( pos );

    if ( message.str_.empty() )
    {
        // empty message
        return;
    }

    if ( ! std::strncmp( sender, "our", 3 ) )
    {
        if ( M_teammate_message_time != current )
        {
            dlog.addText( Logger::SENSOR,
                          __FILE__" (parsePlayerMessage) clear old data" );
            M_teammate_message_time = current;
            M_teammate_messages.clear();
        }

        M_teammate_messages.push_back( message );

        parseTeammateMessage( message );
    }
    else if ( ! std::strncmp( sender, "opp", 3 ) )
    {
        if ( M_opponent_message_time != current )
        {
            M_opponent_message_time = current;
            M_opponent_messages.clear();
        }

        M_opponent_messages.push_back( message );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::parseCoachMessage( const char * msg,
                                const GameTime & current )
{
    // (hear <time> online_coach_{left,right} <msg>) : v7-
    // (hear <time> online_coach_{left,right} (freeform "<msg>")) : v7+
    // (hear <time> online_coach_{left,right} <clang>)   : v7+

    // skip "(hear "
    while ( *msg != ' ' && *msg != '\0' ) ++msg;
    while ( *msg == ' ' ) ++msg;

    // skip "<time> "
    while ( *msg != ' ' && *msg != '\0' ) ++msg;
    while ( *msg == ' ' ) ++msg;

    // skip sender
    while ( *msg != ' ' && *msg != '\0' ) ++msg;
    while ( *msg == ' ' ) ++msg;

    if ( *msg != '(' )
    {
        // not a clang message
        M_freeform_message_time = current;
        M_freeform_message.assign( msg );
        if ( ! M_freeform_message.empty()
             && *M_freeform_message.rbegin() == ')' )
        {
            M_freeform_message.erase( M_freeform_message.length() - 1 );
        }

        if ( M_freeform_parser )
        {
            M_freeform_parser->parse( M_freeform_message.c_str() );
        }

        return;
    }

    // clang message

    char msg_type[32];
    int n_read = 0;

    if ( std::sscanf( msg, "(%31[^ ] %n ",
                      msg_type, &n_read ) != 1 )
    {
        std::cerr << "***ERROR*** failed to parse clang message type. ["
                  << msg
                  << std::endl;
        return;
    }
    msg += n_read;

    if ( std::strcmp( msg_type, "freeform" ) != 0 )
    {
        // not a freeform message
        std::cerr << current << ": "
                  << "recv unsupported clang message. type = "
                  << msg_type
                  << std::endl;
        return;
    }

    while ( *msg == ' ' ) ++msg;

    bool quated = false;
    if ( *msg == '\"' )
    {
        quated = true;
        ++msg;
    }

    M_freeform_message_time = current;
    M_freeform_message = msg;

    // remove quotation or parenthesis
    if ( quated )
    {
        std::string::size_type pos = M_freeform_message.find_last_of( '\"' );
        if ( pos == std::string::npos )
        {
            std::cerr << "***ERROR*** AudioSensor::parseCoachMessage."
                  << " Illegal quated message. [" << msg << ']'
                      << std::endl;
            return;
        }
        M_freeform_message.erase( pos );
    }
    else
    {
        std::string::size_type pos = M_freeform_message.find_last_not_of( ')' );
        if ( pos == std::string::npos
             || pos == M_freeform_message.length() - 1 )
        {
            std::cerr << "***ERROR*** AudioSensor::parseCoachMessage."
                  << " Illegal quated message. [" << msg << ']'
                      << std::endl;
            return;
        }
        M_freeform_message.erase( pos + 1 );
    }


    //std::cerr << current << ": "
    //          << "recv freeform message. ["
    //          << M_freeform_message.str_ << ']'
    //          << std::endl;

    if ( M_freeform_parser )
    {
        M_freeform_parser->parse( M_freeform_message.c_str() );
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::parseTrainerMessage( const char * msg,
                                  const GameTime & current )
{

    // (hear <time> referee <msg>) : v7-
    // (hear <time> coach "<msg>") : v7+
    // (hear <time> coach <clang>) : v7+

    int n_read = 0;
    long cycle;
    char sender[32];

    if ( std::sscanf( msg, "(hear %ld %31s %n ",
                      &cycle, sender, &n_read ) != 2 )
    {
        std::cerr << "***ERRORR*** failed to parse trainer message. ["
                  << msg << ']'
                  << std::endl;
        return;
    }
    msg += n_read;

    while ( *msg == ' ' ) ++msg;

    char end_char = ')';
    if ( *msg == '\"' )
    {
        end_char = '\"';
        ++msg;
    }

    M_trainer_message_time = current;
    M_trainer_message.erase();
    M_trainer_message = msg;

    // remove quotation or parenthesis
    std::string::size_type pos = M_trainer_message.rfind( end_char );
    if ( pos == std::string::npos )
    {
        std::cerr << "***ERROR*** CoachAudioSensor::parsePlayerMessage."
                  << " Illegal quated message. [" << msg << ']'
                  << std::endl;
        return;
    }

    M_trainer_message.erase( pos );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
AudioSensor::parseTeammateMessage( const HearMessage & message )
{
    if ( message.str_.empty() )
    {
        return;
    }

    const ParserMap::iterator end = M_say_message_parsers.end();

    const char * msg = message.str_.c_str();

    while ( *msg != '\0' )
    {
        const char tag = *msg;

        int len = 0;

        ParserMap::iterator it = M_say_message_parsers.find( tag );

        if ( it == end )
        {
            dlog.addText( Logger::SENSOR,
                          __FILE__" (parseTeammateMessage) unsupported message [%s] in [%s]",
                          msg, message.str_.c_str() );
            return;
        }

        len = it->second->parse( message.unum_, message.dir_, msg,
                                 M_teammate_message_time );

        if ( len < 0 )
        {
            return;
        }

        msg += len;
    }

}

}

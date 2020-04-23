// -*-c++-*-

/*!
  \file audio_sensor.h
  \brief audio message analyzer Header File
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

#ifndef RCSC_PLAYER_AUDIO_SENSOR_H
#define RCSC_PLAYER_AUDIO_SENSOR_H

#include <rcsc/common/audio_message.h>

#include <rcsc/game_time.h>
#include <rcsc/types.h>
#include <rcsc/geom/vector_2d.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <map>
#include <list>
#include <vector>

namespace rcsc {

class SayMessageParser;
class FreeformParser;

/*-------------------------------------------------------------------*/
/*!
  \class AudioSensor
  \brief processor for players' communication
*/
class AudioSensor {
private:

    //! typedef of the say message parser container
    typedef std::map< char, boost::shared_ptr< SayMessageParser > > ParserMap;

    //! player message parsers
    ParserMap M_say_message_parsers;

    //! freeform message parsers
    boost::shared_ptr< FreeformParser > M_freeform_parser;

    //! last time that teammate message is heard
    GameTime M_teammate_message_time;

    //! last heard message data from teammate players
    std::list< HearMessage > M_teammate_messages;

    //! last time that teammate message is heard
    GameTime M_opponent_message_time;

    //! last heard message data from opponent players
    std::list< HearMessage > M_opponent_messages;

    //! last time when freeform message is heard
    GameTime M_freeform_message_time;

    //! last received freeform message from coach;
    std::string M_freeform_message;

    //! last time when freeform message form trainer is heard
    GameTime M_trainer_message_time;

    //! last received aural message from trainer
    std::string M_trainer_message;

public:
    /*!
      \brief init member variables by default value
    */
    AudioSensor();

    /*!
      \brief add new player message parer.
      \param parser shared_ptr of player message parser instance
     */
    void addParser( boost::shared_ptr< SayMessageParser > parser );

    /*!
      \brief remove registered parser object
      \param header say message header character
     */
    void removeParser( const char header );

    /*!
      \brief set new freeform message parer.
      \param parser shared_ptr of player message parser instance
     */
    void setFreeformParser( boost::shared_ptr< FreeformParser > parser );

    /*!
      \brief analyze other player's audio message
      \param msg raw server message
      \param current game time when message is received
    */
    void parsePlayerMessage( const char * msg,
                             const GameTime & current );

    /*!
      \brief analyze message from online coach
      \param msg raw server message
      \param current game time when message is received
     */
    void parseCoachMessage( const char * msg,
                            const GameTime & current );

    /*!
      \brief analyze trainer's audio message
      \param msg raw server message
      \param current game time when message is received
    */
    void parseTrainerMessage( const char * msg,
                              const GameTime & current );

    /*!
      \brief get time when teammate message is received
      \return const referncd to the game time
    */
    const
    GameTime & teammateMessageTime() const
      {
          return M_teammate_message_time;
      }

    /*!
      \brief get the last received teammate messages
      \return const reference to the message object container
     */
    const
    std::list< HearMessage > & teammateMessages() const
      {
          return M_teammate_messages;
      }

    /*!
      \brief get time when opponent message is received
      \return const referncd to the game time
    */
    const
    GameTime & opponentMessageTime() const
      {
          return M_opponent_message_time;
      }

    /*!
      \brief get the last received opponent messages
      \return const reference to the message object container
     */
    const
    std::list< HearMessage > & opponentMessages() const
      {
          return M_opponent_messages;
      }

    /*!
      \brief get the time when last freeform message is received
      \return game time variable
     */
    const
    GameTime & freeformMessageTime() const
      {
          return M_freeform_message_time;
      }

    /*!
      \brief get the last received freeform message
      \return const reference to the message object instance
     */
    const
    std::string & freeformMessage() const
      {
          return M_freeform_message;
      }

    /*!
      \brief get the time when last freeform message is received
      \return game time variable
     */
    const
    GameTime & trainerMessageTime() const
      {
          return M_trainer_message_time;
      }

    /*!
      \brief get the last received trainer message info
      \return const reference to the message object instance
     */
    const
    std::string & trainerMessage() const
      {
          return M_trainer_message;
      }

private:
    /*
      \brief analyze message from teammate
      \param message message object from teammate
    */
    void parseTeammateMessage( const HearMessage & message );

};

}

#endif

// -*-c++-*-

/*!
  \file world_model.h
  \brief world model Header File
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

#ifndef RCSC_PLAYER_WORLD_MODEL_H
#define RCSC_PLAYER_WORLD_MODEL_H

#include <rcsc/player/self_object.h>
#include <rcsc/player/ball_object.h>
#include <rcsc/player/player_object.h>
#include <rcsc/player/view_area.h>
#include <rcsc/player/view_grid_map.h>

#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_mode.h>
#include <rcsc/game_time.h>
#include <rcsc/types.h>

#include <boost/shared_ptr.hpp>

#include <string>

namespace rcsc {

class AudioMemory;
class ActionEffector;
class BodySensor;
class FullstateSensor;
class InterceptTable;
class Localization;
class PlayerPredicate;
class PlayerType;
class PenaltyKickState;
class VisualSensor;

/*!
  \class WorldModel
  \brief player's internal field status
*/
class WorldModel {
public:

    enum {
        DIR_CONF_DIVS = 72
    };

    static const std::size_t MAX_RECORD; //!< max record size
    static const double DIR_STEP; //!< the angle steps for dir confidence

private:

    Localization * M_localize; //!< localization module
    InterceptTable * M_intercept_table; //!< interception info table
    boost::shared_ptr< AudioMemory > M_audio_memory; //!< heard deqinfo memory
    PenaltyKickState * M_penalty_kick_state; //!< penalty kick mode status

    //////////////////////////////////////////////////
    std::string M_teamname; //!< our teamname
    SideID M_our_side; //!< our side ID

    std::string M_opponent_teamname; //!< opponent teamname

    //////////////////////////////////////////////////
    // timer & mode

    GameTime M_time; //!< updated time
    GameTime M_sense_body_time; //!< sense_body updated time
    GameTime M_see_time; //!< see updated time
    GameTime M_fullstate_time; //!< fullstate update time

    GameTime M_last_set_play_start_time; //!< SetPlay started time
    long M_setplay_count; //!< setplay counter

    GameMode M_game_mode; //!< playmode and scores

    GameTime M_training_time; //!< training start/end time for keepaway

    bool M_valid; //!< if this world model is initialized, true.

    //////////////////////////////////////////////////
    // field object instance
    SelfObject M_self; //!< self object
    BallObject M_ball; //!< ball object
    PlayerCont M_teammates; //!< side known teammmates
    PlayerCont M_opponents; //!< side known opponents
    PlayerCont M_unknown_players; //!< unknown players

    //////////////////////////////////////////////////
    // object reference (pointers to each object)
    // these containers are updated just before decision making
    PlayerPtrCont M_teammates_from_self; //!< teammates sorted by distance from self
    PlayerPtrCont M_opponents_from_self; //!< opponents sorted by distance from ball, include unknown players
    PlayerPtrCont M_teammates_from_ball; //!< teammates sorted by distance from self
    PlayerPtrCont M_opponents_from_ball; //!< opponents sorted by distance from ball, include unknown players

    int M_our_goalie_unum; //!< uniform number of teammate goalie
    int M_their_goalie_unum; //!< uniform number of opponent goalie

    AbstractPlayerCont M_all_players; //!< all players pointers includes self
    AbstractPlayerCont M_our_players; //!< all teammates pointers includes self
    AbstractPlayerCont M_their_players; //!< all opponents pointers includes unknown

    AbstractPlayerObject * M_known_teammates[12]; //!< unum known teammates (include self)
    AbstractPlayerObject * M_known_opponents[12]; //!< unum known opponents (exclude unknown player)

    //////////////////////////////////////////////////
    // analyzed result

    double M_offside_line_x; //!< offside line x value
    int M_offside_line_count; //!< accuracy count of the offside line

    double M_our_offense_line_x; //!< our offense line x value(consider ball x)
    double M_our_defense_line_x; //!< our defense line x value(consider ball x)(their offside line)
    double M_their_offense_line_x; //!< their offense line x value(consider ball x)
    double M_their_defense_line_x; //!< their defense line x value(consider ball x)
    int M_their_defense_line_count; //!< accuracy count of their defense line x value

    double M_our_offense_player_line_x; //!< our offense player line x value (not consider ball)
    double M_our_defense_player_line_x; //!< our defense player line x value (not consider ball)
    double M_their_offense_player_line_x; //!< their offense player line x value (not consider ball)
    double M_their_defense_player_line_x; //!< their defense player line x value (not consider ball)

    bool M_exist_kickable_teammate; //!< true if exist kickable teammate
    bool M_exist_kickable_opponent; //!< true if exist kickable opponent

    SideID M_last_kicker_side; //!< last ball kicked player's side

    //////////////////////////////////////////////////
    // player type management

    int M_teammate_types[11]; //!< teammate type reference
    int M_opponent_types[11]; //!< opponent type flag

    //////////////////////////////////////////////////
    // card information

    Card M_teammate_card[11]; //!< teammates' yellow/red card status
    Card M_opponent_card[11]; //!< teammates' yellow/red card status

    //////////////////////////////////////////////////
    // visual info

    //! array of direction confidence count
    int M_dir_count[DIR_CONF_DIVS];

    //! view area history
    ViewAreaCont M_view_area_cont;

    //! accuracy count grid map
    ViewGridMap M_view_grid_map;

    //////////////////////////////////////////////////

    //! not used
    WorldModel( const WorldModel & );
    //! not used
    WorldModel & operator=( const WorldModel & );

public:
    /*!
      \brief initialize member variables
    */
    WorldModel();

    /*!
      \brief delete dynamically allocated memory
    */
    ~WorldModel();

    /*!
      \brief get this world mode is valid or not
      \return true if this world model is valid
    */
    bool isValid() const;

    /*!
      \brief set this world mode is valid or not
      \param is_valid value to be set
    */
    void setValid( bool is_valid );

    /*!
      \brief get intercept table
      \return const pointer to the intercept table instance
    */
    const
    InterceptTable * interceptTable() const;

    /*!
      \brief get penalty kick state
      \return const pointer to the penalty kick state instance
    */
    const
    PenaltyKickState * penaltyKickState() const;

    /*!
      \brief get audio memory
      \return const reference to the audio memory instance
     */
    const
    AudioMemory & audioMemory() const
      {
          return *M_audio_memory;
      }

    /*!
      \brief init team info
      \param teamname our team name string
      \param ourside our side ID
      \param my_unum my uniform number
      \param my_goalie true if I am goalie
      \return true if successfully initialized, false otherwise

      This method is called just after receive init reply
    */
    bool initTeamInfo( const std::string & teamname,
                       const SideID ourside,
                       const int my_unum,
                       const bool my_goalie );

    /*!
      \brief set new audio memory
      \param memory pointer to the memory instance. This must be
      a dynamically allocated object.
     */
    void setAudioMemory( boost::shared_ptr< AudioMemory > memory );

    /*!
      \brief set teammate player type & reset card status
      \param unum uniform number of changed teammate
      \param id player type ID
    */
    void setOurPlayerType( const int unum,
                           const int id );

    /*!
      \brief set opponent player type & reset card status
      \param unum uniform number of changed opponent
      \param id player type ID
    */
    void setTheirPlayerType( const int unum,
                             const int id );

    /*!
      \brief set yellow card information
      \param side punished player's side
      \param unum punished player's unum
      \param card card type
     */
    void setCard( const SideID side,
                  const int unum,
                  const Card card );

    /*!
      \brief set current penalty kick taker.
      \param side kick taker's side id
      \param unum kick taker's uniform number
     */
    void setPenaltyKickTaker( const SideID side,
                              const int unum );

    // update stuff
private:
    /*!
      \brief internal update
      \param act action effector
      \param current current game time

      This method updates world status using recorded action effect only
    */
    void update( const ActionEffector & act,
                 const GameTime & current );
public:
    /*!
      \brief update by sense_body.
      \param sense_body analyzed sense_body info
      \param act action effector
      \param current current game time

      This method is called just after sense_body message receive
    */
    void updateAfterSenseBody( const BodySensor & sense_body,
                               const ActionEffector & act,
                               const GameTime & current );

    /*!
      \brief update by see info
      \param see analyzed see info
      \param sense_body analyzed sense_body info
      \param act action effector
      \param current current game time

      This method is called just after see message receive
    */
    void updateAfterSee( const VisualSensor & see,
                         const BodySensor & sense_body,
                         const ActionEffector & act,
                         const GameTime & current );

    /*!
      \brief update by fullstate info
      \param fullstate analyzed fullstate info
      \param act action effector
      \param current current game time

      This method is called just after fullstate message receive
     */
    void updateAfterFullstate( const FullstateSensor & fullstate,
                               const ActionEffector & act,
                               const GameTime & current );

    /*!
      \brief update current playmode
      \param game_mode playmode info
      \param current current game time

      This method is called after heared referee message
    */
    void updateGameMode( const GameMode & game_mode,
                         const GameTime & current );

    /*!
      \brief set training start/end time
      \param t game time
     */
    void setTrainingTime( const GameTime & t )
      {
          M_training_time = t;
      }

    /*!
      \brief update self view move
      \param w new view width
      \param q new view quality
    */
    void setViewMode( const ViewWidth & w,
                      const ViewQuality & q )
      {
          M_self.setViewMode( w, q );
      }

    /*!
      \brief internal update for action decision
      \param act action effector
      \param current current game time

      This method is called just before action decision to update and
      adjust world model.
    */
    void updateJustBeforeDecision( const ActionEffector & act,
                                   const GameTime & current );

    /*!
      \brief internal update using internal info and stored command info.
      \param act ActionEffector object

      called just before command send
    */
    void setCommandEffect( const ActionEffector & act );

private:
    /*!
      \brief self localization
      \param see analyzed see info
      \param sense_body analyzed sense_body info
      \param current current game time
      \return if failed, returns false
    */
    bool localizeSelf( const VisualSensor & see,
                       const BodySensor & sense_body,
                       const GameTime & current );

    /*!
      \brief ball localization
      \param see analyzed see info
      \param act action effector
      \param current current game time
    */
    void localizeBall( const VisualSensor & see,
                       const ActionEffector & act,
                       const GameTime & current );

    /*!
      \brief estimate ball velocity using position difference
      \param see analyzed see info
      \param act action effector
      \param rpos seen relative pos
      \param rpos_error seen relative pos error
      \param vel reference to the velocity variable
      \param vel_error reference to the velocity error variable
      \param vel_count reference to the velocity count variable
    */
    void estimateBallVelByPosDiff( const VisualSensor & see,
                                   const ActionEffector & act,
                                   const Vector2D & rpos,
                                   const Vector2D & rpos_error,
                                   Vector2D & vel,
                                   Vector2D & vel_error,
                                   int & vel_count );

    /*!
      \brief players localization
      \param see analyzed see info
      \param current current game time
    */
    void localizePlayers( const VisualSensor & see );

    /*!
      \brief check player that has team info
      \param side seen side info
      \param player localized info
      \param seen_dist see info data
      \param old_known_players old team known players
      \param old_unknown_players previous unknown players
      \param new_known_players new team known players
    */
    void checkTeamPlayer( const SideID side,
                          const Localization::PlayerT & player,
                          const double & seen_dist,
                          PlayerCont & old_known_players,
                          PlayerCont & old_unknown_players,
                          PlayerCont & new_known_players );

    /*!
      \brief check player that has no identifier. matching to unknown players
      \param player localized info
      \param seen_dist see info data
      \param old_teammates previous seen teammates
      \param old_opponents previous seen opponents
      \param old_unknown_players previous seen unknown player
      \param new_teammates current seen teammates
      \param new_opponents current seen opponents
      \param new_unknown_players current seen unknown players
    */
    void checkUnknownPlayer( const Localization::PlayerT & player,
                             const double & seen_dist,
                             PlayerCont & old_teammates,
                             PlayerCont & old_opponent,
                             PlayerCont & old_unknown_players,
                             PlayerCont & new_teammates,
                             PlayerCont & new_opponents,
                             PlayerCont & new_unknown_players );

    /*!
      \brief check collision.
    */
    void updateCollision();

    /*!
      \brief check ghost object
      \param varea current view area info
    */
    void checkGhost( const ViewArea & varea );

    /*!
      \brief update seen direction accuracy
      \param varea seen view area info
    */
    void updateDirCount( const ViewArea & varea );

    /*!
      \brief update ball by heard info
    */
    void updateBallByHear( const ActionEffector & act );

    /*!
      \brief update opponent goalie by heard info
    */
    void updateGoalieByHear();

    /*!
      \brief update opponent goalie by heard info
    */
    void updatePlayerByHear();

    /*!
      \brief update player type id of the recognized players
     */
    void updatePlayerType();

    /*!
      \brief update players' card state
     */
    void updatePlayerCard();

    /*!
      \brief estimate unknown players' uniform number
     */
    void estimateUnknownPlayerUnum();

    /*!
      \brief update player info. relation with ball and self.
     */
    void updatePlayerStateCache();

    /*!
      \brief update offside line
    */
    void updateOffsideLine();

    /*!
      \brief update our offense line
    */
    void updateOurOffenseLine();

    /*!
      \brief update our defense line
    */
    void updateOurDefenseLine();

    /*!
      \brief update their offense line
    */
    void updateTheirOffenseLine();

    /*!
      \brief update their defense line (offside line)
    */
    void updateTheirDefenseLine();

    /*!
      \brief update forward & defense player lines
     */
    void updatePlayerLines();

    /*!
      \brief estimate last kicker player
     */
    void updateLastKicker();

    /*!
      \brief update intercept table
     */
    void updateInterceptTable();
public:

    /*!
      \brief get our teamname
      \return const reference to the team name string
    */
    const std::string & teamName() const { return M_teamname; }

    /*!
      \brief get our team side Id
      \return side Id
    */
    SideID ourSide() const { return M_our_side; }

    /*!
      \brief get opponent teamname
      \return const reference to the team name string
    */
    const std::string & opponentTeamName() const { return M_opponent_teamname; }

    /*!
      \brief get opponent team side Id
      \return side Id
    */
    SideID theirSide() const
      {
          return M_our_side == LEFT ? RIGHT : LEFT;
      }

    /*!
      \brief get last updated time (== current game time)
      \return const reference to the game time object
    */
    const GameTime & time() const { return M_time; }

    /*!
      \brief get last time updated by sense_body
      \return const reference to the game time object
    */
    const GameTime & senseBodyTime() const { return M_sense_body_time; }

    /*!
      \brief get last time updated by see
      \return const reference to the game time object
    */
    const GameTime & seeTime() const { return M_see_time; }

    /*!
       \brief get last time updated by fullstate
       \return const reference to the game time object
     */
    const GameTime & fullstateTime() const { return M_fullstate_time; }

    /*!
      \brief get last time updated by fullstate
     */

    /*!
      \brief get last setplay type playmode start time
      \return const reference to the game time object
    */
    const GameTime & lastSetPlayStartTime() const { return M_last_set_play_start_time; }

    /*!
      \brief get cycle count that setplay type playmode is keeped
      \return counted long integer
    */
    const long & setplayCount() const { return M_setplay_count; }

    /*!
      \brief get current playmode info
      \return const reference to the GameMode object
    */
    const GameMode & gameMode() const { return M_game_mode; }

    /*!
      \brief get training start/end time
      \return game time object
     */
    const GameTime & trainingTime() const { return M_training_time; }

    /*!
      \brief get self info
      \return const reference to the SelfObject
    */
    const SelfObject & self() const { return M_self; }

    /*!
      \brief get ball info
      \return const reference to the BallObject
    */
    const BallObject & ball() const { return M_ball; }

    /*!
      \brief get teammate info
      \return const reference to the PlayerObject container
    */
    const PlayerCont & teammates() const { return M_teammates; }

    /*!
      \brief get opponent info (except unknown players)
      \return const reference to the PlayerObject container
    */
    const PlayerCont & opponents() const { return M_opponents; }

    /*!
      \brief get unknown player info
      \return const reference to the PlayerObject container
    */
    const PlayerCont & unknownPlayers() const { return M_unknown_players; }

    // reference to the sorted players

    /*!
      \brief get teammates sorted by distance from self
      \return const reference to the PlayerObject pointer container
    */
    const PlayerPtrCont & teammatesFromSelf() const { return M_teammates_from_self; }

    /*!
      \brief get opponents sorted by distance from self (includes unknown players)
      \return const reference to the PlayerObject pointer container
    */
    const PlayerPtrCont & opponentsFromSelf() const { return M_opponents_from_self; }

    /*!
      \brief get teammates sorted by distance from ball
      \return const reference to the PlayerObject pointer container
    */
    const PlayerPtrCont & teammatesFromBall() const { return M_teammates_from_ball; }

    /*!
      \brief get opponents sorted by distance from ball (includes unknown players)
      \return const reference to the PlayerObject pointer container
    */
    const PlayerPtrCont & opponentsFromBall() const { return M_opponents_from_ball; }

    /*!
      \brief get the uniform number of teammate goalie
      \return uniform number value or Unum_Unknown
     */
    int ourGoalieUnum() const { return M_our_goalie_unum; }

    /*!
      \brief get the uniform number of opponent goalie
      \return uniform number value or Unum_Unknown
     */
    int theirGoalieUnum() const { return M_their_goalie_unum; }

    /*!
      \brief get all players includes self.
      \return const rerefence to the AbstractPlayerObject pointer container.
     */
    const AbstractPlayerCont & allPlayers() const { return M_all_players; }

    /*!
      \brief get all teammate players includes self.
      \return const rerefence to the AbstractPlayerObject pointer container.
     */
    const AbstractPlayerCont & ourPlayers() const { return M_our_players; }

    /*!
      \brief get all opponent players (includes unknown players)
      \return const rerefence to the AbstractPlayerObject pointer container.
     */
    const AbstractPlayerCont & theirPlayers() const { return M_their_players; }

    //////////////////////////////////////////////////////////

    /*!
      \brief get a teammate (or self) specified by uniform number
      \param unum wanted player's uniform number
      \return const pointer to the AbstractPlayerObject instance or NULL
    */
    const AbstractPlayerObject * ourPlayer( const int unum ) const
      {
          if ( unum <= 0 || 11 < unum ) return M_known_teammates[0];
          return M_known_teammates[unum];
      }

    /*!
      \brief get an opponent specified by uniform number
      \param unum wanted player's uniform number
      \return const pointer to the AbstractPlayerObject instance or NULL
    */
    const AbstractPlayerObject * theirPlayer( const int unum ) const
      {
          if ( unum <= 0 || 11 < unum ) return M_known_opponents[0];
          return M_known_opponents[unum];
      }

private:

    /*!
      \brief get fist PlayerObject in [first, last] that satisfies confidence
      count threshold
      \param first first iterator of PlayerObject pointer container
      \param last last iterator of PlayerObject pointer container
      \param count_thr accuracy count threshold
      \param with_goalie if this value is false, goalie is ignored.
      \return if found, const pointer to the PlayerOjbect. else NULL
    */
    const PlayerObject * getFirstPlayer( const PlayerPtrCont & players,
                                         const int count_thr,
                                         const bool with_goalie ) const
      {
          const PlayerPtrCont::const_iterator end = players.end();
          for ( PlayerPtrCont::const_iterator p = players.begin();
                p != end;
                ++p )
          {
              if ( ! with_goalie
                   && (*p)->goalie() )
              {
                  continue;
              }

              if ( ! (*p)->isGhost()
                   && (*p)->posCount() <= count_thr )
              {
                  return *p;
              }
          }
          return static_cast< PlayerObject * >( 0 );
      }

public:

    /*!
      \brief get teammate nearest to self with confidence count check
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return if found, const pointer to the PlayerOjbect. else NULL
    */
    const PlayerObject * getTeammateNearestToSelf( const int count_thr,
                                                   const bool with_goalie = true ) const
      {
          return getFirstPlayer( teammatesFromSelf(),
                                 count_thr,
                                 with_goalie );
      }

    /*!
      \brief get opponent nearest to self with accuracy count check
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return if found, const pointer to the PlayerOjbect. else NULL
    */
    const PlayerObject * getOpponentNearestToSelf( const int count_thr,
                                                   const bool with_goalie = true ) const
      {
          return getFirstPlayer( opponentsFromSelf(),
                                 count_thr,
                                 with_goalie );
      }

    /*!
      \brief get the distance from teammate nearest to self wtth accuracy count
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistTeammateNearestToSelf( const int count_thr,
                                         const bool with_goalie = true ) const
      {
          const PlayerObject * p = getTeammateNearestToSelf( count_thr, with_goalie );
          return ( p ? p->distFromSelf() : 65535.0 );
      }

    /*!
      \brief get the distance from opponent nearest to self wtth accuracy count
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistOpponentNearestToSelf( const int count_thr,
                                         const bool with_goalie = true) const
      {
          const PlayerObject * p = getOpponentNearestToSelf( count_thr, with_goalie );
          return ( p ? p->distFromSelf() : 65535.0 );
      }

    /*!
      \brief get teammate nearest to with with confidence count check
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return if found, const pointer to the PlayerOjbect. else NULL
    */
    const PlayerObject * getTeammateNearestToBall( const int count_thr,
                                                   const bool with_goalie = true ) const
      {
          return getFirstPlayer( teammatesFromBall(),
                                 count_thr,
                                 with_goalie );
      }

    /*!
      \brief get opponent nearest to ball with confidence count check
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return if found, const pointer to the PlayerOjbect. else NULL
    */
    const PlayerObject * getOpponentNearestToBall( const int count_thr,
                                                   const bool with_goalie = true ) const
      {
          return getFirstPlayer( opponentsFromBall(),
                                 count_thr,
                                 with_goalie );
      }

    /*!
      \brief get the distance to teammate nearest to ball wtth accuracy count
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistTeammateNearestToBall( const int count_thr,
                                         const bool with_goalie ) const
      {
          const PlayerObject * p = getTeammateNearestToSelf( count_thr, with_goalie );
          return ( p ? p->distFromBall() : 65535.0 );
      }

    /*!
      \brief get the distance to opponent nearest to ball wtth accuracy count
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistOpponentNearestToBall( const int count_thr,
                                         const bool with_goalie = true ) const
      {
          const PlayerObject * p = getOpponentNearestToBall( count_thr, with_goalie );
          return ( p ? p->distFromBall() : 65535.0 );
      }

    /*!
      \brief get estimated offside line x coordinate
      \return offside line
    */
    const double & offsideLineX() const { return M_offside_line_x; }

    /*!
      \brief get the accuracy count for the offside line
      \return accuracy count
     */
    int offsideLineCount() const { return M_offside_line_count; }

    /*!
      \brief our offense line (consider ball pos)
      \return our offense line x
    */
    const double & ourOffenseLineX() const { return M_our_offense_line_x; }

    /*!
      \brief get our defense line (consider ball pos)(offside line for opponent)
      \return our defense line x
    */
    const double & ourDefenseLineX() const { return M_our_defense_line_x; }

    /*!
      \brief get their offense line (consider ball pos)
      \return their offense line x
    */
    const double & theirOffenseLineX() const { return M_their_offense_line_x; }

    /*!
      \brief get their defense line x value (consider ball pos)
      \return their offense line x
    */
    const double & theirDefenseLineX() const { return M_their_defense_line_x; }

    /*!
      \brief get our offense player line (not consder ball pos)
      \return our offense player line x
    */
    const double & ourOffensePlayerLineX() const { return M_our_offense_player_line_x; }

    /*!
      \brief get our defense player line (not consder ball pos)
      \return our defense player line x
    */
    const double & ourDefensePlayerLineX() const { return M_our_defense_player_line_x; }

    /*!
      \brief get their offense player line (not consder ball pos)
      \return our defense player line x
    */
    const double & theirOffensePlayerLineX() const { return M_their_offense_player_line_x; }

    /*!
      \brief get estimated their offense line x value
      \return their offense line x
    */
    const double & theirDefensePlayerLineX() const { return M_their_defense_player_line_x; }


    /*!
      \brief check if exist kickable teammate
      \return true if agent estimated that kickable teammate exists
    */
    bool existKickableTeammate() const { return M_exist_kickable_teammate; }

    /*!
      \brief check if exist kickable opponent
      \return true if agent estimated that kickable opponent exists
    */
    bool existKickableOpponent() const { return M_exist_kickable_opponent; }

    /*!
      \brief get the estimated last kicker's side
      \return side id
     */
    SideID lastKickerSide() const { return M_last_kicker_side; }


    /*!
      \brief get player type Id of teammate
      \param unum target teammate uniform number
      \return player type Id. if unum is illegal, Default Id is returned.
    */
    int ourPlayerTypeId( const int unum ) const
      {
          if ( unum < 1 || 11 < unum )
          {
              std::cerr << "WorldModel::teammateHeteroID. Illegal unum "
                        << unum << std::endl;
              return Hetero_Default;
          }
          return M_teammate_types[ unum - 1 ];
      }

    /*!
      \brief get player type of the specified teammate
      \param unum target teammate uniform number
      \return const pointer to the player type object instance
     */
    const PlayerType * ourPlayerType( const int unum ) const;

    /*!
      \brief get player type Id of opponent
      \param unum target teammate uniform number
      \return player type Id. if unum is illegal, Unknown is returned.
    */
    int theirPlayerTypeId( const int unum ) const
      {
          if ( unum < 1 || 11 < unum )
          {
              std::cerr << "WorldModel::opponentHeteroID. Illegal unum "
                        << unum << std::endl;
              return Hetero_Unknown;
          }
          return M_opponent_types[ unum - 1 ];
      }

    /*!
      \brief get player type of the specified opponent
      \param unum target opponent uniform number
      \return const pointer to the player type object instance
     */
    const PlayerType * theirPlayerType( const int unum ) const;

    //
    // card information
    //

    /*!
      \brief get teammate's yellow card status
      \param unum uniform number
      \return yellow card status
     */
    Card ourCard( const int unum ) const
      {
          if ( unum < 1 || 11 < unum )
          {
              std::cerr << "(WorldModel::teammateCard) Illegal unum "
                        << unum << std::endl;
              return NO_CARD;
          }
          return M_teammate_card[ unum - 1 ];
      }

    /*!
      \brief get opponent's yellow card status
      \param unum uniform number
      \return yellow card status
     */
    Card theirCard( const int unum ) const
      {
          if ( unum < 1 || 11 < unum )
          {
              std::cerr << "(WorldModel::opponentCard) Illegal unum "
                        << unum << std::endl;
              return NO_CARD;
          }
          return M_opponent_card[ unum - 1 ];
      }

    //
    // analyzed results
    //


    // visual memory info

    /*!
      \brief get direction confidence count
      \param angle target direction
      \return confidence count value
    */
    int dirCount( const AngleDeg & angle ) const
      {
          int idx = static_cast< int >( ( angle.degree() - 0.5 + 180.0 )
                                        / DIR_STEP );
          if ( idx < 0 || DIR_CONF_DIVS - 1 < idx )
          {
              std::cerr << "WorldModel::getDirConf. index over flow"
                        << std::endl;
              idx = 0;
          }
          return M_dir_count[idx];
      }

    /*!
      \brief get max count, sum of count and average count of angle range
      \param angle center of target angle range
      \param width angle range
      \param max_count pointer to variable of max accuracy count
      \param sum_count pointer to variable of sum of accuracy count
      \param ave_count pointer to variable of average accuracy count
      \return steps in the range
    */
    int dirRangeCount( const AngleDeg & angle,
                       const double & width,
                       int * max_count,
                       int * sum_count,
                       int * ave_count ) const;

    /*!
      \brief get view area history container
      \return const refrence to the view area container.
     */
    const ViewAreaCont & viewAreaCont() const { return M_view_area_cont; }

    /*!
      \brief get field grid map that holds observation accuracy count
      \return const reference to the ViewGridMap instance
     */
    const ViewGridMap & viewGridMap() const { return M_view_grid_map; }

    /*!
      \brief get the specific point accuracy count
      \param point global cooridinate value of checked point
      \param dir_thr direction threshold for view area
      \return accuracy count. if player has not seen the point, returnthe big value (e.g. 1000).
     */
    int getPointCount( const Vector2D & point,
                       const double & dir_thr ) const;

    //
    // interfaces to player objects
    //

    /*!
      \brief get the new container of AbstractPlayer matched with the predicate.
      \param predicate predicate object for the player condition matching. This have to be a dynamically allocated object.
      \return container of AbstractPlayer pointer.
     */
    AbstractPlayerCont getPlayerCont( const PlayerPredicate * predicate ) const;

    /*!
      \brief get the new container of AbstractPlayer matched with the predicate.
      \param predicate predicate object for the player condition matching.
      \return container of AbstractPlayer pointer.
     */
    AbstractPlayerCont getPlayerCont( boost::shared_ptr< const PlayerPredicate > predicate ) const;

    /*!
      \brief get the new container of AbstractPlayer matched with the predicate.
      \param cont reference to the result variable
      \param predicate predicate object for the player condition matching. This have to be a dynamically allocated object.
     */
    void getPlayerCont( AbstractPlayerCont & cont,
                        const PlayerPredicate * predicate ) const;

    /*!
      \brief get the new container of AbstractPlayer matched with the predicate.
      \param cont reference to the result variable
      \param predicate predicate object for the player condition matching.
     */
    void getPlayerCont( AbstractPlayerCont & cont,
                        boost::shared_ptr< const PlayerPredicate > predicate ) const;

    /*!
      \brief get the number of players that satisfy an input predicate.
      \param predicate predicate predicate object for the player condition matching. This have to be a dynamically allocated object.
      \return number of players.
     */
    size_t countPlayer( const PlayerPredicate * predicate ) const;

    /*!
      \brief get the number of players that satisfy an input predicate.
      \param predicate predicate predicate object for the player condition matching.
      \return number of players.
     */
    size_t countPlayer( boost::shared_ptr< const PlayerPredicate > predicate ) const;

    /*!
      \brief get a goalie teammate (include self)
      \return if found pointer to goalie object, otherwise NULL
    */
    const AbstractPlayerObject * getOurGoalie() const;

    /*!
      \brief get opponent goalie pointer
      \return if found pointer to goalie object, otherwise NULL
    */
    const PlayerObject * getOpponentGoalie() const;

private:

    /*!
      \brief get player pointer nearest to point (excludes self)
      \param point considered point
      \param players target players
      \param count_thr confidence count threshold
      \param dist_to_point variable pointer to store the distance
      from retuned player to point
      \return if found, pointer to player object, othewise NULL
     */
    const PlayerObject * getPlayerNearestTo( const Vector2D & point,
                                             const PlayerPtrCont & players,
                                             const int count_thr,
                                             double * dist_to_point ) const;

    /*!
      \brief get the distance from input point to the nearest player
      \param players target players
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistPlayerNearestTo( const Vector2D & point,
                                   const PlayerPtrCont & players,
                                   const int count_thr ) const
      {
          double d = 65535.0;
          const PlayerObject * p = getPlayerNearestTo( point, players, count_thr, &d );
          return ( p ? d : 65535.0 );
      }

public:

    /*!
      \brief get the distance from input point to the nearest teammate
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistTeammateNearestTo( const Vector2D & point,
                                     const int count_thr ) const
      {
          double d = 65535.0;
          const PlayerObject * p = getPlayerNearestTo( point, teammatesFromSelf(), count_thr, &d );
          return ( p ? d : 65535.0 );
      }

    /*!
      \brief get the distance from input point to the nearest opponent
      \param count_thr accuracy count threshold
      \param with_goalie include goalie if true
      \return distance to the matched opponent. if not found, a big value is returned.
     */
    double getDistOpponentNearestTo( const Vector2D & point,
                                     const int count_thr ) const
      {
          double d = 65535.0;
          const PlayerObject * p = getPlayerNearestTo( point, opponentsFromSelf(), count_thr, &d );
          return ( p ? d : 65535.0 );
      }

    /*!
      \brief get teammate pointer nearest to point
      \param point considered point
      \param count_thr confidence count threshold
      \param dist_to_point variable pointer to store the distance
      from retuned player to point
      \return if found, pointer to player object, othewise NULL
    */
    const PlayerObject * getTeammateNearestTo( const Vector2D & point,
                                               const int count_thr,
                                               double * dist_to_point ) const
      {
          return getPlayerNearestTo( point, teammatesFromSelf(), count_thr, dist_to_point );
      }

    /*!
      \brief get teammate pointer nearest to the specified player
      \param p pointer to the player
      \param count_thr accuracy count threshold
      \param dist_to_point variable pointer to store the distance
      from retuned player to point
      \return if found, pointer to player object, othewise NULL
     */
    const PlayerObject * getTeammateNearestTo( const PlayerObject * p,
                                               const int count_thr,
                                               double * dist_to_point ) const
      {
          if ( ! p ) return static_cast< const PlayerObject * >( 0 );
          return getTeammateNearestTo( p->pos(), count_thr, dist_to_point );
      }

    /*!
      \brief get opponent pointer nearest to point
      \param point considered point
      \param count_thr accuracy count threshold
      \param dist_to_point variable pointer to store the distance
      from retuned player to point
      \return if found pointer to player object, othewise NULL
    */
    const PlayerObject * getOpponentNearestTo( const Vector2D & point,
                                               const int count_thr,
                                               double * dist_to_point ) const
      {
          return getPlayerNearestTo( point, opponentsFromSelf(), count_thr, dist_to_point );
      }

    /*!
      \brief get teammate pointer nearest to the specified player
      \param p pointer to the player
      \param count_thr accuracy count threshold
      \param dist_to_point variable pointer to store the distance
      from retuned player to point
      \return if found, pointer to player object, othewise NULL
     */
    const PlayerObject * getOpponentNearestTo( const PlayerObject * p,
                                         const int count_thr,
                                         double * dist_to_point ) const
      {
          if ( ! p ) return static_cast< const PlayerObject * >( 0 );
          return getOpponentNearestTo( p->pos(), count_thr, dist_to_point );
      }

private:

    /*!
      \brief template utility. check if player exist in the specified region.
      \param region template parameter. region to be checked
      \param players target players
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
     */
    template < typename REGION >
    bool existPlayerIn( const REGION & region,
                        const PlayerPtrCont & players,
                        const int count_thr,
                        const bool with_goalie ) const
      {
          const PlayerPtrCont::const_iterator end = players.end();
          for ( PlayerPtrCont::const_iterator it = players.begin();
                it != end;
                ++it )
          {
              if ( (*it)->posCount() > count_thr
                   || (*it)->isGhost() )
              {
                  continue;
              }
              if ( (*it)->goalie() && ! with_goalie )
              {
                  continue;
              }
              if ( region.contains( (*it)->pos() ) )
              {
                  return true;
              }
          }
          return false;
      }

public:

    /*!
      \brief template utility. check if teammate exist in the specified region.
      \param region template parameter. region to be checked
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
     */
    template < typename REGION >
    bool existTeammateIn( const REGION & region,
                          const int count_thr,
                          const bool with_goalie ) const
      {
          return existPlayerIn( region, teammatesFromSelf(), count_thr, with_goalie );
      }

    /*!
      \brief template utility. check if opponent exist in the specified region.
      \param region template parameter. region to be checked
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
      \return true if some opponent exist
     */
    template < typename REGION >
    bool existOpponentIn( const REGION & region,
                          const int count_thr,
                          const bool with_goalie ) const
      {
          return existPlayerIn( region, opponentsFromSelf(), count_thr, with_goalie );
      }

private:

    /*!
      \brief count the number of teammate exist in the specified region
      \param region template parameter. region to be checked
      \param players target players
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
      \return total count of teammate existed in the region
     */
    template < typename REGION >
    size_t countPlayersIn( const REGION & region,
                           const PlayerPtrCont & players,
                           const int count_thr,
                           const bool with_goalie ) const
      {
          size_t count = 0;
          const PlayerPtrCont::const_iterator end = players.end();
          for ( PlayerPtrCont::const_iterator it = players.begin();
                it != end;
                ++it )
          {
              if ( (*it)->posCount() > count_thr
                   || (*it)->isGhost()
                   || ( (*it)->goalie() && ! with_goalie )
                   )
              {
                  continue;
              }
              if ( region.contains( (*it)->pos() ) )
              {
                  ++count;
              }
          }
          return count;
      }

public:

    /*!
      \brief count the number of teammate exist in the specified region
      \param region template parameter. region to be checked
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
      \return total count of teammate existed in the region
     */
    template < typename REGION >
    size_t countTeammatesIn( const REGION & region,
                             const int count_thr,
                             const bool with_goalie ) const
      {
          return countPlayersIn( region, teammatesFromSelf(), count_thr, with_goalie );
      }

    /*!
      \brief count the number of opponent exist in the specified region
      \param region template parameter. region to be checked
      \param count_thr confdence count threshold for players
      \param with_goalie if true, goalie player is cheked.
      \return total count of opponent existed in the region
     */
    template < typename REGION >
    size_t countOpponentsIn( const REGION & region,
                             const int count_thr,
                             const bool with_goalie ) const
      {
          return countPlayersIn( region, opponentsFromSelf(), count_thr, with_goalie );
      }

};

}

#endif

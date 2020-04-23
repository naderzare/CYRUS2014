// -*-c++-*-

/*!
  \file world_model.cpp
  \brief world model Source File
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

#include "world_model.h"

#include "action_effector.h"
#include "localization_default.h"
#include "body_sensor.h"
#include "visual_sensor.h"
#include "fullstate_sensor.h"
#include "debug_client.h"
#include "intercept_table.h"
#include "penalty_kick_state.h"
#include "player_command.h"
#include "player_predicate.h"

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/server_param.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

#include <set>
#include <iterator>
#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>

// #define DEBUG_PRINT

// #define DEBUG_PRINT_SELF_UPDATE
// #define DEBUG_PRINT_BALL_UPDATE
// #define DEBUG_PRINT_PLAYER_UPDATE

// #define DEBUG_PRINT_LINES


// #defin USE_VIEW_GRID_MAP

namespace rcsc {

namespace  {

/*!
  \brief create specific player reference set
  \param players player instance container
  \param players_from_self reference container
  \param players_from_ball reference container
  \param self_pos self position
  \param ball_pos ball position
*/
inline
void
create_player_set( rcsc::PlayerCont & players,
                   rcsc::PlayerPtrCont & players_from_self,
                   rcsc::PlayerPtrCont & players_from_ball,
                   const rcsc::Vector2D & self_pos,
                   const rcsc::Vector2D & ball_pos )

{
    const rcsc::PlayerCont::iterator end = players.end();
    for ( rcsc::PlayerCont::iterator it = players.begin();
          it != end;
          ++it )
    {
        it->updateSelfBallRelated( self_pos, ball_pos );
        players_from_self.push_back( &( *it ) );
        players_from_ball.push_back( &( *it ) );
    }
}

/*!
  \brief check if player is ball kickable or not
  \param first first element in player container
  \param last last element in player container
  \param ball_count ball accuracy count
  \param ball_error effor of ball position
  \param dist_error_rate observation error rate
*/
inline
bool
check_player_kickable( rcsc::PlayerPtrCont::iterator first,
                       const rcsc::PlayerPtrCont::iterator last,
                       const int ball_count,
                       const double & ball_error,
                       const double & dist_error_rate )
{
    for ( ; first != last; ++first )
    {
        if ( (*first)->isGhost()
             || (*first)->isTackling()
             || (*first)->posCount() > ball_count + 1 )
        {
            continue;
        }

        if ( (*first)->isKickable( - ( ball_error
                                       + std::min( 0.25,
                                                   (*first)->distFromSelf()
                                                   * dist_error_rate ) ) ) )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (check_player_kickable) exist %d-%d (%.1f %.1f)",
                          (*first)->side(),
                          (*first)->unum(),
                          (*first)->pos().x, (*first)->pos().y );
            return true;
        }

        return false;
    }

    return false;
}

/*!

 */
bool
is_reverse_side( const WorldModel & wm,
                 const PenaltyKickState & pen_state )
{
    if ( pen_state.onfieldSide() == LEFT )
    {
        if ( pen_state.isKickTaker( wm.ourSide(), wm.self().unum() ) )
        {
            return true;
        }
        else if ( wm.self().goalie() )
        {
            return false;
        }
    }
    else if ( pen_state.onfieldSide() == RIGHT )
    {
        if ( pen_state.isKickTaker( wm.ourSide(), wm.self().unum() ) )
        {
            return false;
        }
        else if ( wm.self().goalie() )
        {
            return true;
        }
    }

    return ( wm.ourSide() == RIGHT );
}

/*!

 */
double
get_self_face_angle( const WorldModel & wm,
                     const PenaltyKickState & pen_state,
                     const double & seen_face_angle )
{
    if ( pen_state.onfieldSide() == LEFT )
    {
        if ( pen_state.isKickTaker( wm.ourSide(), wm.self().unum() ) )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (get_self_face_angle) pen_onfield=LEFT && kicker -> reverse" );
            return AngleDeg::normalize_angle( seen_face_angle + 180.0 );
        }
        else if ( wm.self().goalie() )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (get_self_face_angle) pen_onfield=LEFT && goalie -> no reverse" );
            return seen_face_angle;
        }
    }
    else if ( pen_state.onfieldSide() == RIGHT )
    {
        if ( pen_state.isKickTaker( wm.ourSide(), wm.self().unum() ) )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (get_self_face_angle) pen_onfield=RIGHT && kicker -> no reverse" );
            return seen_face_angle;
        }
        else if ( wm.self().goalie() )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (get_self_face_angle) pen_onfield=RIGHT && goalie -> reverse" );
            return AngleDeg::normalize_angle( seen_face_angle + 180.0 );
        }
    }

    dlog.addText( Logger::WORLD,
                  __FILE__" (get_self_face_angle) normal " );

    return ( wm.ourSide() == LEFT
             ? seen_face_angle
             : AngleDeg::normalize_angle( seen_face_angle + 180.0 ) );
}

}


/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/

const std::size_t WorldModel::MAX_RECORD = 30;
const double WorldModel::DIR_STEP = 360.0 / static_cast< double >( DIR_CONF_DIVS );

/*-------------------------------------------------------------------*/
/*!

*/
WorldModel::WorldModel()
    : M_localize( new LocalizationDefault() ),
      M_intercept_table( new InterceptTable( *this ) ),
      M_audio_memory( new AudioMemory() ),
      M_penalty_kick_state( new PenaltyKickState() ),
      M_our_side( NEUTRAL ),
      M_time( -1, 0 ),
      M_sense_body_time( -1, 0 ),
      M_see_time( -1, 0 ),
      M_last_set_play_start_time( 0, 0 ),
      M_setplay_count( 0 ),
      M_game_mode(),
      M_training_time( -1, 0 ),
      M_valid( true ),
      M_self(),
      M_ball(),
      M_our_goalie_unum( Unum_Unknown ),
      M_their_goalie_unum( Unum_Unknown ),
      M_offside_line_x( 0.0 ),
      M_offside_line_count( 0 ),
      M_our_offense_line_x( 0.0 ),
      M_our_defense_line_x( 0.0 ),
      M_their_offense_line_x( 0.0 ),
      M_their_defense_line_x( 0.0 ),
      M_their_defense_line_count( 0 ),
      M_our_offense_player_line_x( 0.0 ),
      M_our_defense_player_line_x( 0.0 ),
      M_their_offense_player_line_x( 0.0 ),
      M_their_defense_player_line_x( 0.0 ),
      M_exist_kickable_teammate( false ),
      M_exist_kickable_opponent( false ),
      M_last_kicker_side( NEUTRAL ),
      M_view_area_cont( MAX_RECORD, ViewArea() )
{
    assert( M_intercept_table );
    assert( M_penalty_kick_state );

    for ( int i = 0; i < 11; ++i )
    {
        M_teammate_card[i] = NO_CARD;
        M_opponent_card[i] = NO_CARD;
    }

    for ( int i = 0; i < DIR_CONF_DIVS; i++ )
    {
        M_dir_count[i] = 1000;
    }

    for ( int i = 0; i < 12; ++i )
    {
        M_known_teammates[i] = static_cast< AbstractPlayerObject * >( 0 );
        M_known_opponents[i] = static_cast< AbstractPlayerObject * >( 0 );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
WorldModel::~WorldModel()
{
    if ( M_localize )
    {
        delete M_localize;
        M_localize = static_cast< Localization * >( 0 );
    }

    if ( M_intercept_table )
    {
        delete M_intercept_table;
        M_intercept_table = static_cast< InterceptTable * >( 0 );
    }

    if ( M_penalty_kick_state )
    {
        delete M_penalty_kick_state;
        M_penalty_kick_state = static_cast< PenaltyKickState * >( 0 );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WorldModel::isValid() const
{
    return M_valid;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setValid( bool is_valid )
{
    M_valid = is_valid;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
InterceptTable *
WorldModel::interceptTable() const
{
    //assert( M_intercept_table );
    return M_intercept_table;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
PenaltyKickState *
WorldModel::penaltyKickState() const
{
    //assert( M_penalty_kick_state );
    return M_penalty_kick_state;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
WorldModel::initTeamInfo( const std::string & teamname,
                          const SideID ourside,
                          const int my_unum,
                          const bool my_goalie )
{
    if ( ! M_localize )
    {
        std::cerr << teamname << ' '
                  << my_unum << ':'
                  << " ***ERROR*** Failed to create localization object."
                  << std::endl;
        return false;
    }

    if ( ! M_audio_memory )
    {
        std::cerr << teamname << ' '
                  << my_unum << ':'
                  << " ***ERROR*** No audio message holder."
                  << std::endl;
        return false;
    }

    M_teamname = teamname;
    M_our_side = ourside;
    M_self.init( ourside, my_unum, my_goalie );

    if ( my_goalie )
    {
        M_our_goalie_unum = my_unum;
    }

    for ( int i = 0; i < 11; i++ )
    {
        M_teammate_types[i] = Hetero_Default;
        M_opponent_types[i] = Hetero_Default;
    }

    PlayerTypeSet::instance().resetDefaultType();
    M_self.setPlayerType( Hetero_Default );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setAudioMemory( boost::shared_ptr< AudioMemory > memory )
{
    M_audio_memory = memory;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setOurPlayerType( const int unum,
                              const int id )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << " ***ERROR*** WorldModel:: setTeammatePlayerType "
                  << " Illegal uniform number" << unum
                  << std::endl;
        return;
    }

    dlog.addText( Logger::WORLD,
                  __FILE__" (setTeammatePlayerType) teammate %d to player_type %d",
                  unum, id );

    M_teammate_types[unum - 1] = id;
    M_teammate_card[unum - 1] = NO_CARD;

    if ( unum == self().unum() )
    {
        const PlayerType * tmp = PlayerTypeSet::i().get( id );
        if ( ! tmp )
        {
            std::cerr << teamName() << " : " << self().unum()
                      << "WorldModel: Illega player type id??"
                      << " player type param not found, id = "
                      << id << std::endl;
            return;
        }
        M_self.setPlayerType( id );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setTheirPlayerType( const int unum,
                                const int id )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << " ***ERROR*** WorldModel:: setOpponentPlayerType "
                  << " Illegal uniform number"
                  << unum << std::endl;
        return;
    }

    dlog.addText( Logger::WORLD,
                  __FILE__" (setOpponentPlayerType) opponent %d to player_type %d",
                  unum, id );

    if ( M_opponent_types[unum - 1] != Hetero_Unknown
         && M_opponent_types[unum - 1] != id )
    {
        M_opponent_card[unum - 1] = NO_CARD;
    }

    M_opponent_types[unum - 1] = id;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setCard( const SideID side,
                     const int unum,
                     const Card card )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << " ***ERROR*** (WorldModel::setCard) "
                  << " Illegal uniform number"
                  << unum << std::endl;
        return;
    }

    if ( side == ourSide() )
    {
        if ( self().unum() == unum )
        {
            M_self.setCard( card );
        }
        M_teammate_card[unum - 1] = card;

        dlog.addText( Logger::WORLD,
                      __FILE__" (setCard) teammate %d, card %d",
                      unum, card );
    }
    else if ( side == theirSide() )
    {
        M_opponent_card[unum - 1] = card;

        dlog.addText( Logger::WORLD,
                      __FILE__" (setCard) opponent %d, card %d",
                      unum, card );
    }
    else
    {
        std::cerr << teamName() << " : " << self().unum()
                  << " ***ERROR*** (WorldModel::setCard) "
                  << " Illegal side" << std::endl;
        return;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setPenaltyKickTaker( const SideID side,
                                 const int unum )
{
    M_penalty_kick_state->setKickTaker( side, unum );
}

/*-------------------------------------------------------------------*/
/*!

*/
const
PlayerType *
WorldModel::ourPlayerType( const int unum ) const
{
    if ( unum < 1 || 11 < unum )
    {
        return PlayerTypeSet::i().get( Hetero_Default );
    }

    const PlayerType * p = PlayerTypeSet::i().get( ourPlayerTypeId( unum ) );
    if ( ! p )
    {
        p = PlayerTypeSet::i().get( Hetero_Default );
    }
    return p;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
PlayerType *
WorldModel::theirPlayerType( const int unum ) const
{
    if ( unum < 1 || 11 < unum )
    {
        return PlayerTypeSet::i().get( Hetero_Unknown );
    }

    const PlayerType * p = PlayerTypeSet::i().get( theirPlayerTypeId( unum ) );
    if ( ! p )
    {
        p = PlayerTypeSet::i().get( Hetero_Unknown );
    }
    return p;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::update( const ActionEffector & act,
                    const GameTime & current )
{
    // this function called from updateAfterSense()
    // or, if player could not receive sense_body,
    // this function is called at the each update operations

    if ( time() == current )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << current << "internal update called twice ??"
                  << std::endl;
        return;
    }

    M_time = current;

    // playmode is updated in updateJustBeforeDecision

    M_self.update( act, current );
    M_ball.update( act, gameMode(), current );

#ifdef DEBUG_PRINT
    if ( M_ball.rposValid() )
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (update) internal update. bpos=(%.2f, %.2f)"
                      " brpos=(%.2f, %.2f) bvel=(%.2f, %.2f)",
                      M_ball.pos().x, M_ball.pos().y,
                      M_ball.rpos().x, M_ball.rpos().y,
                      M_ball.vel().x, M_ball.vel().y );
    }
    else
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (update) internal update. bpos=(%.2f, %.2f)"
                      " bvel=(%.2f, %.2f), invalid rpos",
                      M_ball.pos().x, M_ball.pos().y,
                      M_ball.vel().x, M_ball.vel().y );
    }
#endif

    // clear pointer reference container
    M_teammates_from_self.clear();
    M_opponents_from_self.clear();
    M_teammates_from_ball.clear();
    M_opponents_from_ball.clear();

    M_all_players.clear();
    M_our_players.clear();
    M_their_players.clear();

    for ( int i = 0; i < 12; ++i )
    {
        M_known_teammates[i] = static_cast< AbstractPlayerObject * >( 0 );
        M_known_opponents[i] = static_cast< AbstractPlayerObject * >( 0 );
    }

    if ( this->gameMode().type() == GameMode::BeforeKickOff
         || ( this->gameMode().type() == GameMode::AfterGoal_
              && this->time().stopped() <= 20 )
         )
    {
        M_teammates.clear();
        M_opponents.clear();
        M_unknown_players.clear();
    }

    // update teammates
    std::for_each( M_teammates.begin(),
                   M_teammates.end(),
                   PlayerObject::UpdateOp() );
    M_teammates.remove_if( PlayerObject::IsInvalidOp() );

    // update opponents
    std::for_each( M_opponents.begin(),
                   M_opponents.end(),
                   PlayerObject::UpdateOp() );
    M_opponents.remove_if( PlayerObject::IsInvalidOp() );

    // update unknown players
    std::for_each( M_unknown_players.begin(),
                   M_unknown_players.end(),
                   PlayerObject::UpdateOp() );
    M_unknown_players.remove_if( PlayerObject::IsInvalidOp() );

    // update view area

    for ( int i = 0; i < DIR_CONF_DIVS; i++ )
    {
        M_dir_count[i] = std::min( 10, M_dir_count[i] + 1);
        //dlog.addText( Logger::WORLD,
        //            "  world.dirConf: %4.0f -> %d",
        //            (double)i * 360.0 / static_cast<double>(DIR_CONF_DIVS) - 180.0,
        //            M_dir_conf[i] );
    }

    M_view_area_cont.pop_back();
    M_view_area_cont.push_front( ViewArea( current ) );
#ifdef USE_VIEW_GRID_MAP
    M_view_grid_map.incrementAll();
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateAfterSenseBody( const BodySensor & sense_body,
                                  const ActionEffector & act,
                                  const GameTime & current )
{
    // called just after sense_body

    // if I could not get sense_body & could get see before action decision,
    // this method is called before update(VisualSensor &, GameTime &)

    // if I could not get sense_body & see before action decision,
    // this method is called just before action decision.

    if ( M_sense_body_time == current )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << current
                  << " world.updateAfterSense: called twice"
                  << std::endl;
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterSense) called twide" );
        return;
    }

    M_sense_body_time = sense_body.time();

    dlog.addText( Logger::WORLD,
                  "*************** updateAfterSense ***************" );

    if ( sense_body.time() == current )
    {
#ifdef DEBUG_PRINT_SELF_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterSense) update self" );
#endif
        M_self.updateAfterSenseBody( sense_body, act, current );
        M_localize->updateBySenseBody( sense_body );
    }

    M_teammate_card[self().unum() - 1] = sense_body.card();

    if ( time() != current )
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterSense) call internal update" );
        // internal update
        update( act, current );
        // check collision
        //updateCollision();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateCollision()
{
    // called in updateJustBeforeDecision()

    if ( ! ball().posValid()
         || ! ball().velValid()
         || ! self().posValid()
         || ! self().velValid() )
    {

        return;
    }

    if ( ball().velCount() == 0 )
    {
        // already seen the ball velocity directly
        // nothing to do
        return;
    }

    bool collided_with_ball = false;

    if ( self().hasSensedCollision() )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateCollision) agent has sensed collision info" );
#endif
        collided_with_ball = self().collidesWithBall();
        if ( collided_with_ball )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateCollision) detected by sense_body" );
#endif
        }
    }
    else
    {
        // internally updated positions
        const double self_ball_dist
            = ( ball().pos() - self().pos() ).r();

        if ( ( self().collisionEstimated()
               && self_ball_dist < ( self().playerType().playerSize()
                                     + ServerParam::i().ballSize()
                                     + 0.1 )
               )
             || ( ( self().collisionEstimated()
                    || self().vel().r() < ( self().playerType().realSpeedMax()
                                            * self().playerType().playerDecay()
                                            * 0.11 ) )
                  && ( self_ball_dist < ( self().playerType().playerSize()
                                          + ServerParam::i().ballSize()
                                          - 0.2 ) ) )
             )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateCollision) detected. ball_dist= %.3f",
                          self_ball_dist );
#endif
            collided_with_ball = true;
        }
    }

    if ( collided_with_ball )
    {
        if ( ball().posCount() > 0 )
        {
            Vector2D mid = ball().pos() + self().pos();
            mid *= 0.5;

            Vector2D mid2ball = ball().pos() - mid;
            Vector2D mid2self = self().pos() - mid;
            double ave_size = ( ServerParam::i().ballSize()
                                + self().playerType().playerSize() ) * 0.5;
            mid2ball.setLength( ave_size );
            mid2self.setLength( ave_size );

            Vector2D new_ball_pos = mid + mid2ball;
            Vector2D ball_add = new_ball_pos - ball().pos();
            Vector2D new_ball_rpos = ball().rpos() + ball_add;
            Vector2D new_ball_vel = ball().vel() * -0.1;

            M_ball.updateByCollision( new_ball_pos, ball().posCount() + 1,
                                      new_ball_rpos, ball().rposCount() + 1,
                                      new_ball_vel, ball().velCount() + 1 );
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateCollision) new bpos(%.2f %.2f) rpos(%.2f %.2f)"
                          " vel(%.2f %.2f)",
                          new_ball_pos.x, new_ball_pos.y,
                          new_ball_rpos.x, new_ball_rpos.y,
                          new_ball_vel.x, new_ball_vel.y );
#endif
            if ( self().posCount() > 0 )
            {
                Vector2D new_my_pos = mid + mid2self;
                double my_add_r = ( new_my_pos - self().pos() ).r();
                Vector2D new_my_pos_error = self().posError();
                new_my_pos_error.x += my_add_r;
                new_my_pos_error.y += my_add_r;

                M_self.updateByCollision( new_my_pos, new_my_pos_error );

#ifdef DEBUG_PRINT
                dlog.addText( Logger::WORLD,
                              __FILE__" (updateCollision) new mypos(%.2f %.2f) error(%.2f %.2f)",
                              new_my_pos.x, new_my_pos.y,
                              new_my_pos_error.x, new_my_pos_error.y );
#endif
            }
        }
        else // ball().posCount() == 0
        {
            int vel_count = ( self().hasSensedCollision()
                              ? ball().velCount()
                              : ball().velCount() + 1 );

            M_ball.updateByCollision( ball().pos(), ball().posCount(),
                                      ball().rpos(), ball().rposCount(),
                                      ball().vel() * -0.1, vel_count );
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateCollision) seen ball. new_vel=(%.2f %.2f)",
                          ball().vel().x, ball().vel().y );
#endif
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateAfterSee( const VisualSensor & see,
                            const BodySensor & sense_body,
                            const ActionEffector & act,
                            const GameTime & current )
{
    //////////////////////////////////////////////////////////////////
    // check internal update time
    if ( time() != current )
    {
        update( act, current );
    }

    //////////////////////////////////////////////////////////////////
    // check last sight update time
    if ( M_see_time == current )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << current << " (updateAfterSee) : called twice "
                  << std::endl;
        return;
    }
    //////////////////////////////////////////////////////////////////
    // time update
    M_see_time = current;

    dlog.addText( Logger::WORLD,
                  "*************** updateAfterSee *****************" );

    if ( M_fullstate_time == current )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterSee) already updated by fullstate" );
#endif
        // stored info
        ViewArea varea( self().viewWidth().width(),
                        self().pos(),
                        self().face(),
                        current );
        // add to view area history
        M_view_area_cont.front() = varea;

        // check ghost object
        //checkGhost( varea );
        // update field grid map
        //M_view_grid_map.update( varea );
        // update dir accuracy
        updateDirCount( varea );
        return;
    }

    //////////////////////////////////////////////////////////////////
    // set opponent teamname
    if ( M_opponent_teamname.empty()
         && ! see.opponentTeamName().empty() )
    {
        M_opponent_teamname = see.opponentTeamName();
    }

    //////////////////////////////////////////////////////////////////
    // self localization
    localizeSelf( see, sense_body, current );

    //////////////////////////////////////////////////////////////////
    // ball localization
    localizeBall( see, act, current );

    //////////////////////////////////////////////////////////////////
    // player localization & matching
    localizePlayers( see );

    //////////////////////////////////////////////////////////////////
    // view cone & ghost check
    // my global position info is successfully updated.
    if ( self().posCount() <= 10
         && self().viewQuality() == ViewQuality::HIGH )
    {
        // stored info
        ViewArea varea( self().viewWidth().width(),
                        self().pos(),
                        self().face(),
                        current );
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterSee) view_area, origin=(%.2f, %.2f) angle=%.1f, width=%.1f vwidth=%d,%.2f",
                      varea.origin().x, varea.origin().y,
                      varea.angle().degree(), varea.viewWidth(),
                      self().viewWidth().type(),
                      self().viewWidth().width() );
#endif
        // add to view area history
        M_view_area_cont.front() = varea;

        // check ghost object
        checkGhost( varea );
        // update field grid map
#ifdef USE_VIEW_GRID_MAP
        M_view_grid_map.update( current, varea );
#endif
        // update dir accuracy
        updateDirCount( varea );
    }

    //////////////////////////////////////////////////////////////////
    // debug output
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  "<--- mypos=(%.2f, %.2f) err=(%.3f, %.3f) vel=(%.2f, %.2f)",
                  self().pos().x, self().pos().y,
                  self().posError().x, self().posError().y,
                  self().vel().x, self().vel().y );
    dlog.addText( Logger::WORLD,
                  "<--- seen players t=%d: ut=%d: o=%d: uo=%d: u=%d",
                  see.teammates().size(),
                  see.unknownTeammates().size(),
                  see.opponents().size(),
                  see.unknownOpponents().size(),
                  see.unknownPlayers().size() );
    dlog.addText( Logger::WORLD,
                  "<--- internal players t=%d: o=%d: u=%d",
                  M_teammates.size(),
                  M_opponents.size(),
                  M_unknown_players.size() );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateAfterFullstate( const FullstateSensor & fullstate,
                                  const ActionEffector & act,
                                  const GameTime & current )
{
    // internal update
    if ( time() != current )
    {
        update( act, current );
    }

    if ( M_fullstate_time == current )
    {
        std::cerr << teamName() << " : " << self().unum()
                  << current << " (updateAfterFullstate) called twice "
                  << std::endl;
        return;
    }

    M_fullstate_time = current;

    dlog.addText( Logger::WORLD,
                  "*************** updateAfterFullstate ***************" );

    // update players
    const FullstateSensor::PlayerCont & fullstate_teammates
        = ( ourSide() == LEFT
            ? fullstate.leftTeam()
            : fullstate.rightTeam() );
    const FullstateSensor::PlayerCont & fullstate_opponents
        = ( ourSide() == LEFT
            ? fullstate.rightTeam()
            : fullstate.leftTeam() );

    // clean unkown players
    M_unknown_players.clear();

    // update teammates
    for ( FullstateSensor::PlayerCont::const_iterator fp = fullstate_teammates.begin();
          fp != fullstate_teammates.end();
          ++fp )
    {
        if ( fp->unum_ < 1 || 11 < fp->unum_ )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateAfterFullstate) illegal teammate unum %d",
                          fp->unum_ );
            std::cerr << " (updateAfterFullstate) illegal teammate unum. " << fp->unum_
                      << std::endl;
            continue;
        }

#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterFullstate) teammate %d type=%d card=%s",
                      fp->unum_, fp->type_,
                      fp->card_ == YELLOW ? "yellow" : fp->card_ == RED ? "red" : "no" );
#endif

        M_teammate_types[fp->unum_ - 1] = fp->type_;
        M_teammate_card[fp->unum_ - 1] = fp->card_;

        // update self
        if ( fp->unum_ == self().unum() )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateAfterFullstate) update self" );
#endif
            M_self.updateAfterFullstate( *fp, act, current );
            continue;
        }

        // update teammate
        PlayerObject * player = static_cast< PlayerObject * >( 0 );
        for ( PlayerCont::iterator p = M_teammates.begin();
              p != M_teammates.end();
              ++p )
        {
            if ( p->unum() == fp->unum_ )
            {
                player = &(*p);
                break;
            }
        }

        if ( ! player )
        {
            // create new player object
            M_teammates.push_back( PlayerObject() );
            player = &(M_teammates.back());
        }
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterFullstate) updated teammate %d",
                      fp->unum_ );
#endif
        player->updateByFullstate( *fp, self().pos(), fullstate.ball().pos_ );
    }

    // update opponents
    for ( FullstateSensor::PlayerCont::const_iterator fp = fullstate_opponents.begin();
          fp != fullstate_opponents.end();
          ++fp )
    {
        if ( fp->unum_ < 1 || 11 < fp->unum_ )
        {
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateAfterFullstate) illegal opponent unum %d",
                          fp->unum_ );
            std::cerr << " (updateAfterFullstate) illegal opponent unum. " << fp->unum_
                      << std::endl;
            continue;
        }

#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterFullstate) teammate %d type=%d card=%s",
                      fp->unum_, fp->type_,
                      fp->card_ == YELLOW ? "yellow" : fp->card_ == RED ? "red" : "no" );
#endif

        M_opponent_types[fp->unum_ - 1] = fp->type_;
        M_opponent_card[fp->unum_ - 1] = fp->card_;

        PlayerObject * player = static_cast< PlayerObject * >( 0 );
        for ( PlayerCont::iterator p = M_opponents.begin();
              p != M_opponents.end();
              ++p )
        {
            if ( p->unum() == fp->unum_ )
            {
                player = &(*p);
                break;
            }
        }

        if ( ! player )
        {
            M_opponents.push_back( PlayerObject() );
            player = &(M_opponents.back());
        }

#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateAfterFullstate) updated opponent %d",
                      fp->unum_ );
#endif
        player->updateByFullstate( *fp, self().pos(), fullstate.ball().pos_ );
    }

    // update ball
    M_ball.updateByFullstate( fullstate.ball().pos_,
                              fullstate.ball().vel_,
                              self().pos() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateGameMode( const GameMode & game_mode,
                            const GameTime & current )
{
    bool pk_mode = game_mode.isPenaltyKickMode();

    if ( ! pk_mode
         && game_mode.type() != GameMode::PlayOn ) // not play_on
    {
        // playmode is changed
        if ( gameMode().type() != game_mode.type() )
        {
            if ( game_mode.type() == GameMode::FreeKick_
                 && ( gameMode().type() == GameMode::OffSide_
                      || gameMode().type() == GameMode::FoulCharge_
                      || gameMode().type() == GameMode::FoulPush_
                      || gameMode().type() == GameMode::BackPass_
                      || gameMode().type() == GameMode::FreeKickFault_
                      || gameMode().type() == GameMode::CatchFault_
                      || gameMode().type() == GameMode::IndFreeKick_
                      )
                 )
            {
                // nothing to do
            }
            else
            {
                M_last_set_play_start_time = current;
                M_setplay_count = 0;
            }
        }

        // check human referee's interaction
        if ( gameMode().type() == game_mode.type()
             && game_mode.type() == GameMode::FreeKick_ )
        {
            M_last_set_play_start_time = current;
            M_setplay_count = 0;
        }
    }

    M_game_mode = game_mode;

    //
    // update penalty kick status
    //
    if ( pk_mode )
    {
        M_penalty_kick_state->update( game_mode, ourSide(), current );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateBallByHear( const ActionEffector & act )
{
    if ( M_fullstate_time == this->time() )
    {
        return;
    }

    if ( M_audio_memory->ballTime() != this->time()
         || M_audio_memory->ball().empty() )
    {
        return;
    }

    // calculate average positin using all heard info

    Vector2D heard_pos = Vector2D::INVALIDATED;
    Vector2D heard_vel = Vector2D::INVALIDATED;


    double min_dist2 = 1000000.0;
    for ( std::vector< AudioMemory::Ball >::const_iterator b = M_audio_memory->ball().begin();
          b != M_audio_memory->ball().end();
          ++b )
    {
        const PlayerObject * sender = static_cast< const PlayerObject * >( 0 );
        for ( PlayerCont::const_iterator t = M_teammates.begin();
              t != M_teammates.end();
              ++t )
        {
            if ( t->unum() == b->sender_ )
            {
                sender = &(*t);
                break;
            }
        }

        if ( sender )
        {
#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateBallByHear) sender=%d exists in memory",
                          b->sender_ );
#endif
            double d2 = sender->pos().dist2( ball().pos() );
            if ( d2 < min_dist2 )
            {
                min_dist2 = d2;
                heard_pos = b->pos_;
                if ( b->vel_.isValid() )
                {
                    heard_vel = b->vel_;
                }
            }
        }
        else if ( min_dist2 > 100000.0 )
        {
#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateBallByHear) sender=%d, unknown",
                          b->sender_ );
#endif
            min_dist2 = 100000.0;
            heard_pos = b->pos_;
            if ( b->vel_.isValid() )
            {
                heard_vel = b->vel_;
            }
        }

        //heard_pos += b->pos_;
        //heard_vel += b->vel_;
    }

    //heard_pos /= static_cast< double >( M_audio_memory->ball().size() );
    //heard_vel /= static_cast< double >( M_audio_memory->ball().size() );

    if ( heard_pos.isValid() )
    {
        M_ball.updateByHear( act, std::sqrt( min_dist2 ), heard_pos, heard_vel );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateGoalieByHear()
{
    if ( M_fullstate_time == this->time() )
    {
        return;
    }

    if ( M_audio_memory->goalieTime() != this->time()
         || M_audio_memory->goalie().empty() )
    {
        return;
    }

    if ( theirGoalieUnum() == Unum_Unknown )
    {
        return;
    }

    PlayerObject * goalie = static_cast< PlayerObject * >( 0 );

    PlayerCont::iterator end = M_opponents.end();
    for( PlayerCont::iterator it = M_opponents.begin();
         it != end;
         ++it )
    {
        if ( it->goalie() )
        {
            goalie = &(*it);
            break;
        }
    }

    if ( goalie
         && goalie->posCount() == 0
         && goalie->bodyCount() == 0 )
    {
        // goalie is seen at the current time.
#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateGoalieByHear) but already seen" );
#endif
        return;
    }

    Vector2D heard_pos( 0.0, 0.0 );
    double heard_body = 0.0;

    for ( std::vector< AudioMemory::Goalie >::const_iterator it = M_audio_memory->goalie().begin();
          it != M_audio_memory->goalie().end();
          ++it )
    {
        heard_pos += it->pos_;
        heard_body += it->body_.degree();
    }

    heard_pos /= static_cast< double >( M_audio_memory->goalie().size() );
    heard_body /= static_cast< double >( M_audio_memory->goalie().size() );

#ifdef DEBUG_PRINT_PLAYER_UPDATE
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateGoalieByHear) pos=(%.1f %.1f) body=%.1f",
                  heard_pos.x, heard_pos.y,
                  heard_body );
#endif

    if ( goalie )
    {
        goalie->updateByHear( theirSide(),
                              theirGoalieUnum(),
                              true,
                              heard_pos,
                              heard_body,
                              -1.0 ); // unknown stamina
        return;
    }

    // goalie not found

    // search the nearest unknown player

    const double goalie_speed_max = ServerParam::i().defaultPlayerSpeedMax();

    double min_dist = 1000.0;

    for( PlayerCont::iterator it = M_opponents.begin();
         it != end;
         ++it )
    {
        if ( it->unum() != Unum_Unknown ) continue;

        if ( it->pos().x < ServerParam::i().theirPenaltyAreaLineX()
             || it->pos().absY() > ServerParam::i().penaltyAreaHalfWidth() )
        {
            // out of penalty area
            continue;
        }

        double d = it->pos().dist( heard_pos );
        if ( d < min_dist
             && d < it->posCount() * goalie_speed_max + it->distFromSelf() * 0.06 )
        {
            min_dist = d;
            goalie = &(*it);
        }
    }

    const PlayerCont::iterator u_end = M_unknown_players.begin();
    for ( PlayerCont::iterator it = M_unknown_players.begin();
          it != u_end;
          ++it )
    {
        if ( it->pos().x < ServerParam::i().theirPenaltyAreaLineX()
             || it->pos().absY() > ServerParam::i().penaltyAreaHalfWidth() )
        {
            // out of penalty area
            continue;
        }

        double d = it->pos().dist( heard_pos );
        if ( d < min_dist
             && d < it->posCount() * goalie_speed_max + it->distFromSelf() * 0.06 )
        {
            min_dist = d;
            goalie = &(*it);
        }
    }


    if ( goalie )
    {
        // found a candidate unknown player
#ifdef DEBUG_PRINT_PLAYER_UPDATE
       dlog.addText( Logger::WORLD,
                     __FILE__" (updateGoalieByHear) found."
                     " heard_pos=(%.1f %.1f)",
                     heard_pos.x, heard_pos.y );
#endif
       goalie->updateByHear( theirSide(),
                             theirGoalieUnum(),
                             true,
                             heard_pos,
                             heard_body,
                             -1.0 ); // unknown stamina
    }
    else
    {
        // register new object
#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateGoalieByHear) not found."
                      " add new goalie. heard_pos=(%.1f %.1f)",
                      heard_pos.x, heard_pos.y );
#endif
        M_opponents.push_back( PlayerObject() );
        goalie = &(M_opponents.back());
        goalie->updateByHear( theirSide(),
                              theirGoalieUnum(),
                              true,
                              heard_pos,
                              heard_body,
                              -1.0 ); // unknown stamina
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updatePlayerByHear()
{
    if ( M_fullstate_time == this->time() )
    {
        return;
    }

    if ( M_audio_memory->playerTime() != this->time()
         || M_audio_memory->player().empty() )
    {
        return;
    }

    // TODO: consider duplicated player

    const std::vector< AudioMemory::Player >::const_iterator heard_end = M_audio_memory->player().end();
    for ( std::vector< AudioMemory::Player >::const_iterator heard_player = M_audio_memory->player().begin();
          heard_player != heard_end;
          ++heard_player )
    {
        if ( heard_player->unum_ == Unum_Unknown )
        {
            continue;
        }

        const SideID side = ( heard_player->unum_ <= 11
                              ? ourSide()
                              : theirSide() );
        const int unum = ( heard_player->unum_ <= 11
                           ? heard_player->unum_
                           : heard_player->unum_ - 11 );

        if ( unum < 1 || 11 < unum )
        {
            std::cerr << __FILE__ << ':' << __LINE__
                      << ": ***ERROR*** (updatePlayerByHear) Illega unum "
                      << unum
                      << " heard_unum=" << heard_player->unum_
                      << " pos=" << heard_player->pos_
                      << std::endl;
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerByHear). Illega unum %d"
                          " pos=(%.1f %.1f)",
                          unum, heard_player->pos_.x, heard_player->pos_.y );
            continue;
        }

        if ( side == ourSide()
             && unum == self().unum() )
        {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerByHear) heard myself. skip" );
#endif
            continue;
        }

        PlayerObject * player = static_cast< PlayerObject * >( 0 );

        PlayerCont & players = ( side == ourSide()
                                 ? M_teammates
                                 : M_opponents );
        const PlayerCont::iterator end = players.end();
        for ( PlayerCont::iterator p = players.begin();
              p != end;
              ++p )
        {
            if ( p->unum() == unum )
            {
                player = &(*p);
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (updatePlayerByHear) found."
                              " side %d, unum %d",
                              side, unum );
#endif
                break;
            }
        }

        PlayerCont::iterator unknown = M_unknown_players.end();
        double min_dist = 0.0;
        if ( ! player )
        {
            min_dist = 1000.0;
            for  ( PlayerCont::iterator p = players.begin();
                   p != end;
                   ++p )
            {
                double d = p->pos().dist( heard_player->pos_ );
                if ( d < min_dist
                     && d < p->posCount() * 1.2 + p->distFromSelf() * 0.06 )
                {
                    min_dist = d;
                    player = &(*p);
                }
            }

            const PlayerCont::iterator u_end = M_unknown_players.end();
            for ( PlayerCont::iterator p = M_unknown_players.begin();
                  p != u_end;
                  ++p )
            {
                double d = p->pos().dist( heard_player->pos_ );
                if ( d < min_dist
                     && d < p->posCount() * 1.2 + p->distFromSelf() * 0.06 )
                {
                    min_dist = d;
                    player = &(*p);
                    unknown = p;
                }
            }
        }

        if ( player )
        {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerByHear) exist candidate."
                          " heard_pos(%.1f %.1f) body=%.1f stamina=%.1f,  memory pos(%.1f %.1f) count %d  dist=%.2f",
                          heard_player->pos_.x,
                          heard_player->pos_.y,
                          heard_player->body_,
                          heard_player->stamina_,
                          player->pos().x, player->pos().y,
                          player->posCount(),
                          min_dist );
#endif
            player->updateByHear( side,
                                  unum,
                                  false,
                                  heard_player->pos_,
                                  heard_player->body_,
                                  heard_player->stamina_ );

            if ( unknown != M_unknown_players.end() )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (updatePlayerByHear) splice unknown player to known player list" );
#endif
                players.splice( players.end(),
                                M_unknown_players,
                                unknown );
            }
        }
        else
        {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerByHear) not found."
                          " add new player heard_pos(%.1f %.1f) body=%.1f stamina=%.1f",
                          heard_player->pos_.x,
                          heard_player->pos_.y,
                          heard_player->body_,
                          heard_player->stamina_ );
#endif
            if ( side == ourSide() )
            {
                M_teammates.push_back( PlayerObject() );
                player = &( M_teammates.back() );
            }
            else
            {
                M_opponents.push_back( PlayerObject() );
                player = &( M_opponents.back() );
            }

            player->updateByHear( side,
                                  unum,
                                  false,
                                      heard_player->pos_,
                                  heard_player->body_,
                                  heard_player->stamina_ );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateJustBeforeDecision( const ActionEffector & act,
                                      const GameTime & current )
{
    if ( time() != current )
    {
        update( act, current );
    }

    if ( M_audio_memory->waitRequestTime() == current )
    {
        M_setplay_count = 0;
    }
    else
    {
        // always increment
        ++M_setplay_count;
    }

    updateBallByHear( act );
    updateGoalieByHear();
    updatePlayerByHear();

    updateCollision();

    estimateUnknownPlayerUnum();
    updatePlayerCard();
    updatePlayerType();

    M_ball.updateSelfRelated( self() );
    M_self.updateBallInfo( ball() );

    updatePlayerStateCache();

#if 1
    // 2008-04-18: akiyama
    // set the effect of opponent kickable state to the ball velocity
    if ( M_exist_kickable_opponent
         && ! self().isKickable() )
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateJustBeforeDecision) : exist kickable opp. ball vel is set to 0." );

        M_ball.setOpponentControlEffect();
    }
#endif

    updateOurOffenseLine();
    updateOurDefenseLine();
    updateTheirOffenseLine();
    updateTheirDefenseLine();
    updateOffsideLine();

    updatePlayerLines();

    updateLastKicker();

    updateInterceptTable();

    M_ball.updateRecord();

    M_self.updateKickableState( M_ball,
                                M_intercept_table->selfReachCycle(),
                                M_intercept_table->teammateReachCycle(),
                                M_intercept_table->opponentReachCycle() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::setCommandEffect( const ActionEffector & act )
{
    if ( act.changeViewCommand() )
    {
        M_self.setViewMode( act.changeViewCommand()->width(),
                            act.changeViewCommand()->quality() );
    }

    if ( act.pointtoCommand() )
    {
        M_self.setPointto( act.getPointtoPos(),
                           time() );
    }

    const PlayerAttentiontoCommand * attentionto = act.attentiontoCommand();
    if ( attentionto )
    {
        if ( attentionto->isOn() )
        {
            if ( attentionto->side() == PlayerAttentiontoCommand::OUR )
            {
                M_self.setAttentionto( ourSide(),
                                       attentionto->number() );
            }
            else
            {
                SideID opp_side = ( ourSide() == LEFT
                                    ? RIGHT
                                    : LEFT );
                M_self.setAttentionto( opp_side,
                                       attentionto->number() );
            }
        }
        else
        {
            // off
            M_self.setAttentionto( NEUTRAL, 0 );
        }
    }

}


/*-------------------------------------------------------------------*/
/*!

*/
bool
WorldModel::localizeSelf( const VisualSensor & see,
                          const BodySensor & sense_body,
                          const GameTime & current )
{
    const bool reverse_side = is_reverse_side( *this, *M_penalty_kick_state );

    double angle_face = -360.0;
    double angle_face_error = 0.0;
    Vector2D my_pos( Vector2D::INVALIDATED );
    Vector2D my_pos_error( 0.0, 0.0 );

    // estimate self face angle
    if ( ! M_localize->estimateSelfFace( see, &angle_face, &angle_face_error ) )
    {
        return false;
    }

    //
    // set face angle to ignore coordinate system
    //
    double team_angle_face = ( reverse_side
                               ? AngleDeg::normalize_angle( angle_face + 180.0 )
                               : angle_face );

    //
    // set face dir to self
    //
    M_self.updateAngleBySee( team_angle_face, std::min( angle_face_error, 180.0 ),
                             current );

    // correct vel dir using seen self angle & sense_body's speed magnitude
    M_self.updateVelDirAfterSee( sense_body, current );


    // estimate self position
    if ( ! M_localize->localizeSelf( see,
                                     angle_face, angle_face_error,
                                     &my_pos, &my_pos_error ) )
    {
        return false;
    }

    if ( reverse_side )
    {
        my_pos *= -1.0;
    }

#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (localizeSelf) reverse=%s face:(seen==%.1f use=%.1f) pos=(%f %f)",
                  ( reverse_side ? "on" : "off" ),
                  angle_face,
                  team_angle_face,
                  my_pos.x, my_pos.y );
#endif
#if 0
    Vector2D my_pos_new = Vector2D::INVALIDATED;
    Vector2D my_pos_error_new( 0.0, 0.0 );

    M_localize->updateParticles( self().lastMove(), current );
    M_localize->localizeSelf2( see,
                               angle_face, angle_face_error,
                               &my_pos_new, &my_pos_error_new,
                               current );
#endif

    if ( my_pos.isValid() )
    {
#ifdef DEBUG_PRINT_SELF_UPDATE
        dlog.addRect( Logger::WORLD,
                      my_pos.x - my_pos_error.x, my_pos.y - my_pos_error.y,
                      my_pos_error.x * 2.0, my_pos_error.y * 2.0,
                      "#ff0000" );
#endif
        M_self.updatePosBySee( my_pos, my_pos_error,
                               team_angle_face, std::min( angle_face_error, 180.0 ),
                               current );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::localizeBall( const VisualSensor & see,
                          const ActionEffector & act,
                          const GameTime & /*current*/ )
{
    if ( ! self().faceValid() )
    {
        //std::cerr << "localizeBall : my face invalid conf= "
        //          << self().faceCount() << std::endl;
        return;
    }

    //////////////////////////////////////////////////////////////////
    // calc relative info

    Vector2D rpos( Vector2D::INVALIDATED );
    Vector2D rpos_error( 0.0, 0.0 );
    Vector2D rvel( Vector2D::INVALIDATED );
    Vector2D vel_error( 0.0, 0.0 );

    if ( ! M_localize->localizeBallRelative( see,
                                             self().face().degree(), self().faceError(),
                                             &rpos, &rpos_error,
                                             &rvel, &vel_error )  )
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizeBall) localization failed" );
#endif
        return;
    }

    if ( ! rpos.isValid() )
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizeBall) invalid rpos. cannot calc current seen pos" );
#endif
        return;
    }

    //////////////////////////////////////////////////////////////////
    // Case: invalid self localization
    // to estimate ball global position, self localization is required.
    // in this case, we can estimate only relative info
    if ( ! self().posValid() )
    {
        if ( ball().rposCount() == 1
             && ( see.balls().front().dist_
                  > self().playerType().playerSize() + ServerParam::i().ballSize() + 0.1 )
             && self().lastMove().isValid() )
        {
            Vector2D tvel = ( rpos - ball().rposPrev() ) + self().lastMove();
            Vector2D tvel_err = rpos_error + self().velError();
            // set only vel
            tvel *= ServerParam::i().ballDecay();
            tvel_err *= ServerParam::i().ballDecay();
            M_ball.updateOnlyVel( tvel, tvel_err, 1 );

#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizeBall) only vel (%.3f %.3f)",
                          tvel.x, tvel.y );
#endif
        }

        // set relative pos
        M_ball.updateOnlyRelativePos( rpos, rpos_error );

#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizeBall) only relative pos (%.3f %.3f)",
                      rpos.x, rpos.y );
#endif
        return;
    }

    //////////////////////////////////////////////////////////////////
    // calc global pos & vel using visual

    Vector2D pos = self().pos() + rpos;
    Vector2D pos_error = self().posError() + rpos_error;
    Vector2D gvel( Vector2D::INVALIDATED );
    int vel_count = 1000;


#ifdef DEBUG_PRINT_BALL_UPDATE
    dlog.addRect( Logger::WORLD,
                  pos.x - rpos_error.x, pos.y - rpos_error.y,
                  rpos_error.x * 2.0, rpos_error.y * 2.0,
                  "#ff0000" );
#endif

    if ( rvel.isValid()
         && self().velValid() )
    {
        gvel = self().vel() + rvel;
        vel_error += self().velError();
        vel_count = 0;
    }

    //////////////////////////////////////////////////////////////////
    // set static values according to the current playmode

    if ( gameMode().type() == GameMode::PlayOn
         || gameMode().type() == GameMode::GoalKick_
         || gameMode().type() == GameMode::PenaltyTaken_ )
    {
        // do nothing
    }
    else if ( gameMode().type() == GameMode::KickIn_ )
    {
        if ( pos.y > 0.0 ) pos.y = + ServerParam::i().pitchHalfWidth();
        if ( pos.y < 0.0 ) pos.y = - ServerParam::i().pitchHalfWidth();

        gvel.assign( 0.0, 0.0 );
        vel_count = 0;
    }
    else if ( gameMode().type() == GameMode::GoalKick_ )
    {
        if ( pos.y > 0.0 ) pos.y = + ServerParam::i().pitchHalfWidth() - ServerParam::i().cornerKickMargin();
        if ( pos.y < 0.0 ) pos.y = - ServerParam::i().pitchHalfWidth() + ServerParam::i().cornerKickMargin();

        if ( pos.x > 0.0 ) pos.x = + ServerParam::i().pitchHalfLength() - ServerParam::i().cornerKickMargin();
        if ( pos.x < 0.0 ) pos.x = - ServerParam::i().pitchHalfLength() + ServerParam::i().cornerKickMargin();

        gvel.assign( 0.0, 0.0 );
        vel_count = 0;
    }
    else if ( gameMode().type() == GameMode::KickOff_ )
    {
        pos.assign( 0.0, 0.0 );
        gvel.assign( 0.0, 0.0 );
        vel_count = 0;
    }
    else
    {
        gvel.assign( 0.0, 0.0 );
        vel_count = 0;
    }


    //////////////////////////////////////////////////////////////////
    // calc global velocity using rpos diff (if ball is out of view cone and within vis_dist)

    if ( ! gvel.isValid() )
    {
        estimateBallVelByPosDiff( see, act, rpos, rpos_error,
                                  gvel, vel_error, vel_count );
    }

    //////////////////////////////////////////////////////////////////
    // set data

    if ( gvel.isValid() )
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizeBall) updateAll. p(%.3f %.3f) rel(%.3f %.3f) v(%.3f %.3f)",
                      pos.x, pos.y, rpos.x, rpos.y, gvel.x, gvel.y );
#endif
        M_ball.updateAll( pos, pos_error, self().posCount(),
                          rpos, rpos_error,
                          gvel, vel_error, vel_count );
    }
    else
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizeBall) updatePos. p(%.3f %.3f) rel(%.3f %.3f)",
                      pos.x, pos.y, rpos.x, rpos.y );
#endif
        M_ball.updatePos( pos, pos_error, self().posCount(),
                          rpos, rpos_error );
    }

#ifdef DEBUG_PRINT_BALL_UPDATE
    dlog.addText( Logger::WORLD,
                  "<--- ball pos=(%.2f, %.2f) err=(%.3f, %.3f)"
                  " rpos=(%.2f, %.2f) rpos_err=(%.3f, %.3f)",
                  ball().pos().x, ball().pos().y,
                  ball().posError().x, ball().posError().y,
                  ball().rpos().x, ball().rpos().y,
                  ball().rposError().x, ball().rposError().y );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::estimateBallVelByPosDiff( const VisualSensor & see,
                                      const ActionEffector & act,
                                      const Vector2D & rpos,
                                      const Vector2D & rpos_error,
                                      Vector2D & vel,
                                      Vector2D & vel_error,
                                      int & vel_count )
{
    if ( self().hasSensedCollision() )
    {
        if ( self().collidesWithPlayer()
             || self().collidesWithPost() )
        {
#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (estimateBallVelByPosDiff) canceled by collision.." );
#endif
            return;
        }
    }

    if ( ball().rposCount() == 1 ) // player saw the ball at prev cycle, too.
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (estimateBallVelByPosDiff) update by rpos diff(1)." );
#endif

        if ( see.balls().front().dist_ < 3.15 // ServerParam::i().visibleDistance()
             && ball().rposPrev().isValid()
             && self().velValid()
             && self().lastMove().isValid() )
        {
            Vector2D rpos_diff = rpos - ball().rposPrev();
            Vector2D tmp_vel = rpos_diff + self().lastMove();
            Vector2D tmp_vel_error = rpos_error + self().velError();
            tmp_vel *= ServerParam::i().ballDecay();
            tmp_vel_error *= ServerParam::i().ballDecay();

#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          "________ rpos(%.3f %.3f) prev_rpos(%.3f %.3f)",
                          rpos.x, rpos.y,
                          ball().rposPrev().x, ball().rposPrev().y );
            dlog.addText( Logger::WORLD,
                          "________ diff(%.3f %.3f) my_move(%.3f %.3f) -> vel(%.2f, %2f)",
                          rpos_diff.x, rpos_diff.y,
                          self().lastMove().x, self().lastMove().y,
                          tmp_vel.x, tmp_vel.y );

            dlog.addText( Logger::WORLD,
                          "________ internal ball_vel(%.3f %.3f) polar(%.5f %.2f) ",
                          ball().vel().x, ball().vel().y,
                          ball().vel().r(), ball().vel().th().degree() );
            dlog.addText( Logger::WORLD,
                          "________ estimated ball_vel(%.3f %.3f) vel_error(%.5f %.2f) ",
                          tmp_vel.x, tmp_vel.y,
                          tmp_vel_error.x, tmp_vel_error.y );

#endif
            if ( ball().seenVelCount() <= 2
                 && ball().rposPrev().r() > 1.5
                 && see.balls().front().dist_ > 1.5
                 // && ! M_exist_kickable_teammate
                 // && ! M_exist_kickable_opponent
                 // && ( tmp_vel.th() - ball().vel().th() ).abs() < 5.0
                 && std::fabs( tmp_vel.x - ball().vel().x ) < 0.1
                 && std::fabs( tmp_vel.y - ball().vel().y ) < 0.1
                 )
            {
#ifdef DEBUG_PRINT_BALL_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (estimateBallVelByPosDiff) cancel" );
#endif
                return;
            }

#ifdef DEBUG_PRINT_BALL_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (estimateBallVelByPosDiff) update" );
#endif
            vel = tmp_vel;
            vel_error = tmp_vel_error;
            vel_count = 1;
        }
    }
    else if ( ball().rposCount() == 2 )
    {
#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (estimateBallVelByPosDiff) update by rpos diff(2)." );
#endif

        if ( see.balls().front().dist_ < 3.15
             && act.lastBodyCommandType() != PlayerCommand::KICK
             && ball().seenRPos().isValid()
             && ball().seenRPos().r() < 3.15
             && self().velValid()
             && self().lastMove( 0 ).isValid()
             && self().lastMove( 1 ).isValid() )
        {
            Vector2D ball_move = rpos - ball().seenRPos();
            ball_move += self().lastMove( 0 );
            ball_move += self().lastMove( 1 );
            vel = ball_move * ( square( ServerParam::i().ballDecay() )
                                / ( 1.0 + ServerParam::i().ballDecay() ) );

            double vel_r = vel.r();
            double estimate_speed = ball().vel().r();

#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (estimateBallVelByPosDiff)"
                          " diff_vel=(%.2f %.2f)%.3f   estimate_vel=(%.2f %.2f)%.3f",
                          vel.x, vel.y, vel_r,
                          ball().vel().x, ball().vel().y, estimate_speed );
#endif

            if ( vel_r > estimate_speed + 0.1
                 || vel_r < estimate_speed * ( 1.0 - ServerParam::i().ballRand() * 2.0 ) - 0.1
                 || ( vel - ball().vel() ).r() > estimate_speed * ServerParam::i().ballRand() * 2.0 + 0.1 )
            {
#ifdef DEBUG_PRINT_BALL_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (estimateBallVelByPosDiff)"
                              " failed to update ball vel using pos diff(2) " );
#endif
                vel.invalidate();
            }
            else
            {
                vel_error = ( rpos_error * 2.0 ) + self().velError();
                vel_error *= ServerParam::i().ballDecay();
                vel_count = 2;

#ifdef DEBUG_PRINT_BALL_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (estimateBallVelByPosDiff)"
                              " cur_rpos(%.2f %.2f) prev_rpos(%.2f %.2f)",
                              rpos.x, rpos.y,
                              ball().seenRPos().x, ball().seenRPos().y );
                dlog.addText( Logger::WORLD,
                              "____ ball_move(%.2f %.2f) my_move0(%.2f %.2f) my_move1(%.2f %.2f)",
                              ball_move.x, ball_move.y,
                              self().lastMove( 0 ).x, self().lastMove( 0 ).y,
                              self().lastMove( 1 ).x, self().lastMove( 1 ).y );
                dlog.addText( Logger::WORLD,
                              "---> vel(%.2f, %2f)",
                              vel.x, vel.y );
#endif
            }

        }
    }
#if 0
    else if ( ball().rposCount() == 3 )
    {
        dlog.addText( Logger::WORLD
                      __FILE__" (estimateBallVelByPosDiff) vel update by rpos diff(3) " );

        if ( see.balls().front().dist_ < 3.15
             && act.lastBodyCommandType( 0 ) != PlayerCommand::KICK
             && act.lastBodyCommandType( 1 ) != PlayerCommand::KICK
             && ball().seenRPos().isValid()
             && ball().seenRPos().r() < 3.15
             && self().velValid()
             && self().lastMove( 0 ).isValid()
             && self().lastMove( 1 ).isValid()
             && self().lastMove( 2 ).isValid() )
        {
            Vector2D ball_move = rpos - ball().seenRPos();
            ball_move += self().lastMove( 0 );
            ball_move += self().lastMove( 1 );
            ball_move += self().lastMove( 2 );

            vel = ball_move * ( std::pow( ServerParam::i().ballDecay(), 3 )
                                / ( 1.0 + ServerParam::i().ballDecay() + square( ServerParam::i().ballDecay() ) ) );

            double vel_r = vel.r();
            double estimate_speed = ball().vel().r();

#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (estimateBallVelByPosDiff)"
                          " diff_vel=(%.2f %.2f)%.3f   estimate_vel=(%.2f %.2f)%.3f",
                          vel.x, vel.y, vel_r,
                          ball().vel().x, ball().vel().y, estimate_speed );
#endif

            if ( vel_r > estimate_speed + 0.1
                 || vel_r < estimate_speed * ( 1.0 - ServerParam::i().ballRand() * 3.0 ) - 0.1
                 || ( vel - ball().vel() ).r() > estimate_speed * ServerParam::i().ballRand() * 3.0 + 0.1 )
            {
                dlog.addText( Logger::WORLD,
                              "world.localizeBall: .failed to update ball vel using pos diff(2) " );
                vel.invalidate();
            }
            else
            {
                vel_error = ( rpos_error * 3.0 ) + self().velError();
                vel_error *= ServerParam::i().ballDecay();
                vel_count = 2;

#ifdef DEBUG_PRINT_BALL_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (estimateBallVelByPosDiff)"
                              " cur_rpos(%.2f %.2f) prev_rpos(%.2f %.2f)"
                              " ball_move(%.2f %.2f)"
                              " my_move0(%.2f %.2f) my_move1(%.2f %.2f) my_move2(%.2f %.2f)"
                              " -> vel(%.2f, %2f)",
                              rpos.x, rpos.y,
                              ball().seenRPos().x, ball().seenRPos().y,
                              ball_move.x, ball_move.y,
                              self().lastMove( 0 ).x, self().lastMove( 0 ).y,
                              self().lastMove( 1 ).x, self().lastMove( 1 ).y,
                              self().lastMove( 2 ).x, self().lastMove( 2 ).y,
                              vel.x, vel.y );
#endif
            }
        }
    }
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::localizePlayers( const VisualSensor & see )
{
    if ( ! self().faceValid()
         || ! self().posValid() )
    {
        return;
    }

    ////////////////////////////////////////////////////////////////
    // update policy
    //   for each (seen player objects)
    //       if exist matched player in memory list
    //           -> splice from memory to temporary
    //       else
    //           -> assign new data to temporary list
    //   after loop, copy from temporary to memory again

    // temporary data list
    PlayerCont new_teammates;
    PlayerCont new_opponents;
    PlayerCont new_unknown_players;

    const Vector2D MYPOS = self().pos();
    const Vector2D MYVEL = self().vel();
    const double MY_FACE = self().face().degree();
    const double MY_FACE_ERR = self().faceError();

    //////////////////////////////////////////////////////////////////
    // search order is very important !!
    //   If we replace the unknown player to unknown teammate,
    //   it may cause a mistake for pass target selection.

    // current version search order is
    //   [unum opp -> side opp -> unum mate -> side mate -> unknown]

    // if matched, that player is removed from memory list
    // and copy to temporary

    //////////////////////////////////////////////////////////////////
    // localize, matching and splice from memory list to temporary list

    // unum seen opp
    {
        const VisualSensor::PlayerCont::const_iterator o_end = see.opponents().end();
        for ( VisualSensor::PlayerCont::const_iterator it = see.opponents().begin();
              it != o_end;
              ++it )
        {
            Localization::PlayerT player;
            // localize
            if ( ! M_localize->localizePlayer( *it,
                                               MY_FACE, MY_FACE_ERR, MYPOS, MYVEL,
                                               &player ) )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (localizePlayers) failed opponent %d",
                              player.unum_ );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizePlayers)"
                          " - localized opponent %d pos=(%.2f, %.2f) vel=(%.2f, %.2f)",
                          player.unum_,
                          player.pos_.x, player.pos_.y,
                          player.vel_.x, player.vel_.y );
#endif
            // matching, splice or create
            checkTeamPlayer( theirSide(),
                             player, it->dist_,
                             M_opponents,
                             M_unknown_players,
                             new_opponents );
        }
    }
    // side seen opp
    {
        const VisualSensor::PlayerCont::const_iterator uo_end = see.unknownOpponents().end();
        for ( VisualSensor::PlayerCont::const_iterator it = see.unknownOpponents().begin();
              it != uo_end;
              ++it )
        {
            Localization::PlayerT player;
            // localize
            if ( ! M_localize->localizePlayer( *it,
                                               MY_FACE, MY_FACE_ERR, MYPOS, MYVEL,
                                               &player ) )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (localizePlayers) failed u-opponent" );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizePlayers)"
                          " - localized u-opponent pos=(%.2f, %.2f)",
                          player.pos_.x, player.pos_.y );
#endif
            // matching, splice or create
            checkTeamPlayer( theirSide(),
                             player, it->dist_,
                             M_opponents,
                             M_unknown_players,
                             new_opponents );
        }
    }
    // unum seen mate
    {
        const VisualSensor::PlayerCont::const_iterator t_end = see.teammates().end();
        for ( VisualSensor::PlayerCont::const_iterator it = see.teammates().begin();
              it != t_end;
              ++it )
        {
            Localization::PlayerT player;
            // localize
            if ( ! M_localize->localizePlayer( *it,
                                               MY_FACE, MY_FACE_ERR, MYPOS, MYVEL,
                                               &player ) )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (localizePlayers) failed teammate %d",
                              player.unum_ );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizePlayers)"
                          " - localized teammate %d pos=(%.2f, %.2f) vel=(%.2f, %.2f)",
                          player.unum_,
                          player.pos_.x, player.pos_.y,
                          player.vel_.x, player.vel_.y );
#endif
            // matching, splice or create
            checkTeamPlayer( ourSide(),
                             player, it->dist_,
                             M_teammates,
                             M_unknown_players,
                             new_teammates );
        }
    }
    // side seen mate
    {
        const VisualSensor::PlayerCont::const_iterator ut_end = see.unknownTeammates().end();
        for ( VisualSensor::PlayerCont::const_iterator it = see.unknownTeammates().begin();
              it != ut_end;
              ++it )
        {
            Localization::PlayerT player;
            // localize
            if ( ! M_localize->localizePlayer( *it,
                                               MY_FACE, MY_FACE_ERR, MYPOS, MYVEL,
                                               &player ) )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (localizePlayers) failed u-teammate" );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizePlayers)"
                          " - localized u-teammate pos=(%.2f, %.2f)",
                          player.pos_.x, player.pos_.y );
#endif
            // matching, splice or create
            checkTeamPlayer( ourSide(),
                             player, it->dist_,
                             M_teammates,
                             M_unknown_players,
                             new_teammates );
        }
    }
    // unknown player
    {
        const VisualSensor::PlayerCont::const_iterator u_end = see.unknownPlayers().end();
        for ( VisualSensor::PlayerCont::const_iterator it = see.unknownPlayers().begin();
              it != u_end;
              ++it )
        {
            Localization::PlayerT player;
            // localize
            if ( ! M_localize->localizePlayer( *it,
                                               MY_FACE, MY_FACE_ERR, MYPOS, MYVEL,
                                               &player ) )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (localizePlayers) failed unknown" );
#endif
                continue;
            }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (localizePlayers)"
                          " - localized unknown: pos=(%.2f, %.2f)",
                          player.pos_.x, player.pos_.y );
#endif
            // matching, splice or create
            checkUnknownPlayer( player,
                                it->dist_,
                                M_teammates,
                                M_opponents,
                                M_unknown_players,
                                new_teammates,
                                new_opponents,
                                new_unknown_players );
        }
    }

    //////////////////////////////////////////////////////////////////
    // splice temporary seen players to memory list
    // temporary lists are cleared
    M_teammates.splice( M_teammates.end(),
                        new_teammates );
    M_opponents.splice( M_opponents.end(),
                        new_opponents );
    M_unknown_players.splice( M_unknown_players.end(),
                              new_unknown_players );

    //////////////////////////////////////////////////////////////////
    // create team member pointer vector for sort

    PlayerPtrCont all_teammates_ptr;
    PlayerPtrCont all_opponents_ptr;

    {
        const PlayerCont::iterator end = M_teammates.end();
        for ( PlayerCont::iterator it = M_teammates.begin();
              it != end;
              ++it )
        {
            all_teammates_ptr.push_back( &( *it ) );
        }
    }
    {
        const PlayerCont::iterator end = M_opponents.end();
        for ( PlayerCont::iterator it = M_opponents.begin();
              it != end;
              ++it )
        {
            all_opponents_ptr.push_back( &( *it ) );
        }
    }


    /////////////////////////////////////////////////////////////////
    // sort by accuracy count
    std::sort( all_teammates_ptr.begin(),
               all_teammates_ptr.end(),
               PlayerObject::PtrCountCmp() );
    std::sort( all_opponents_ptr.begin(),
               all_opponents_ptr.end(),
               PlayerObject::PtrCountCmp() );
    M_unknown_players.sort( PlayerObject::CountCmp() );


    //////////////////////////////////////////////////////////////////
    // check the number of players
    // if overflow is detected, player is removed based on confidence value

    // remove from teammates
    PlayerPtrCont::size_type mate_count = all_teammates_ptr.size();
    while ( mate_count > 11 - 1 )
    {
        // reset least confidence value player
#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizePlayers)"
                      " erase overflow teammate, pos=(%.2f, %.2f)",
                      all_teammates_ptr.back()->pos().x,
                      all_teammates_ptr.back()->pos().y );
#endif
        all_teammates_ptr.back()->forget();
        all_teammates_ptr.pop_back();
        --mate_count;
    }

    // remove from not-teammates
    PlayerPtrCont::size_type opp_count = all_opponents_ptr.size();
    while ( opp_count > 15 ) // 11 )
    {
        // reset least confidence value player
#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizePlayers)"
                      " erase overflow opponent, pos=(%.2f, %.2f)",
                      all_opponents_ptr.back()->pos().x,
                      all_opponents_ptr.back()->pos().y );
#endif
        all_opponents_ptr.back()->forget();
        all_opponents_ptr.pop_back();
        --opp_count;
    }

    // remove from unknown players
    PlayerCont::size_type n_size_unknown = M_unknown_players.size();
    size_t n_size_total
        = static_cast< size_t >( n_size_unknown )
        + static_cast< size_t >( mate_count )
        + static_cast< size_t >( opp_count );
    while ( n_size_unknown > 0
            && n_size_total > 11 + 15 - 1 ) //11 * 2 - 1 )
    {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (localizePlayers)"
                      " erase over flow unknown player, pos=(%.2f, %.2f)",
                      M_unknown_players.back().pos().x,
                      M_unknown_players.back().pos().y );
#endif
        if ( M_unknown_players.back().posCount() == 0 )
        {
            // not remove !!!
            break;
        }
        // remove least confidence value player
        M_unknown_players.pop_back();
        --n_size_unknown;
        --n_size_total;
    }


    //////////////////////////////////////////////////////////////////
    // if overflow is detected, instance player must be forget.
    // that player must be removed from memory list.

    // check invalid player
    // if exist, that player is removed from instance list
    M_teammates.remove_if( PlayerObject::IsInvalidOp() );
    M_opponents.remove_if( PlayerObject::IsInvalidOp() );

    //////////////////////////////////////////////////////////////////
    // it is not necessary to check the all unknown list
    // because invalid unknown player is already removed.


    //////////////////////////////////////////////////////////////////
    // ghost check is done in checkGhost()
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::checkTeamPlayer( const SideID side,
                             const Localization::PlayerT & player,
                             const double & seen_dist,
                             PlayerCont & old_known_players,
                             PlayerCont & old_unknown_players,
                             PlayerCont & new_known_players )
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
    //  if matched player is found, that player is removed from old list
    //  and updated data is splice to new container
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

    static const
        double player_speed_max
        = ServerParam::i().defaultPlayerSpeedMax() * 1.1;

    const double quantize_buf
        = unquantize_error( seen_dist, ServerParam::i().distQuantizeStep() );


    //////////////////////////////////////////////////////////////////
    // pre check
    // unum is seen -> search the player that has the same uniform number
    if ( player.unum_ != Unum_Unknown )
    {
        // search from old unum known players
        const PlayerCont::iterator end = old_known_players.end();
        for ( PlayerCont::iterator it = old_known_players.begin();
              it != end;
              ++it )
        {
            if ( it->unum() == player.unum_ )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              " -- matched!"
                              " unum = %d pos =(%.1f %.1f)",
                              player.unum_, player.pos_.x, player.pos_.y );
#endif
                it->updateBySee( side, player );
                new_known_players.splice( new_known_players.end(),
                                          old_known_players,
                                          it );
                return; // success!!
            }
        }
    }

    //////////////////////////////////////////////////////////////////
    // find nearest player

    double min_team_dist = 10.0 * 10.0;
    double min_unknown_dist = 10.0 * 10.0;

    PlayerCont::iterator candidate_team = old_known_players.end();
    PlayerCont::iterator candidate_unknown = old_unknown_players.end();

    //////////////////////////////////////////////////////////////////
    {
        // search from old same team players
        const PlayerCont::iterator end = old_known_players.end();
        for ( PlayerCont::iterator it = old_known_players.begin();
              it != end;
              ++it )
        {
            if ( player.unum_ != Unum_Unknown
                 && it->unum() != Unum_Unknown
                 && it->unum() != player.unum_ )
            {
                // unum is seen
                // and it does not match with old player's unum.
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              "______ known player: unum is not match."
                              " seen unum = %d, old_unum = %d",
                              player.unum_, it->unum() );
#endif
                continue;
            }

            double d = ( player.pos_ - it->pos() ).r();

            if ( d > ( player_speed_max * it->posCount() + quantize_buf * 2.0
                       + 2.0 )
                 )
            {
                // TODO: inertia movement should be considered.
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              "______ known player: dist over."
                              " dist=%.2f > buf=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              ( player_speed_max * it->posCount()
                                + quantize_buf * 2.0
                                + 2.0 ),
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                continue;
            }

            if ( d < min_team_dist )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              "______ known player: update."
                              " dist=%.2f < min_team_dist=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              min_team_dist,
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                min_team_dist = d;
                candidate_team = it;
            }
        }
    }

    //////////////////////////////////////////////////////////////////
    // search from unknown players
    {
        const PlayerCont::iterator end = old_unknown_players.end();
        for ( PlayerCont::iterator it = old_unknown_players.begin();
              it != end;
              ++it )
        {
            double d = ( player.pos_ - it->pos() ).r();

            if ( d > ( player_speed_max * it->posCount() + quantize_buf * 2.0
                       + 2.0 )
                 )
            {
                // TODO: inertia movement should be considered.
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              "______ unknown player: dist over. "
                              "dist=%.2f > buf=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              ( player_speed_max * it->posCount()
                                + quantize_buf * 2.0
                                + 2.0 ),
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                continue;
            }

            if ( d < min_unknown_dist )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkTeamPlayer)"
                              "______ unknown player: update. "
                              " dist=%.2f < min_unknown_dist=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              min_unknown_dist,
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                min_unknown_dist = d;
                candidate_unknown = it;
            }
        }
    }


    PlayerCont::iterator candidate = old_unknown_players.end();
    double min_dist = 1000.0;
    PlayerCont * target_list = static_cast< PlayerCont * >( 0 );

    if ( candidate_team != old_known_players.end()
         && min_team_dist < min_unknown_dist )
    {
        candidate = candidate_team;
        min_dist = min_team_dist;
        target_list = &old_known_players;

#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkTeamPlayer)"
                      "--- %d (%.1f %.1f)"
                      " -> team player %d (%.2f, %.2f) dist=%.2f",
                      player.unum_,
                      player.pos_.x, player.pos_.y,
                      candidate->unum(),
                      candidate->pos().x, candidate->pos().y,
                      min_dist );
#endif
    }

    if ( candidate_unknown != old_unknown_players.end()
         && min_unknown_dist < min_team_dist )
    {
        candidate = candidate_unknown;
        min_dist = min_unknown_dist;
        target_list = &old_unknown_players;

#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkTeamPlayer)"
                      "--- %d (%.1f %.1f)"
                      " -> unknown player (%.2f, %.2f) dist=%.2f",
                      player.unum_,
                      player.pos_.x, player.pos_.y,
                      candidate->pos().x, candidate->pos().y,
                      min_dist );
#endif
    }


    //////////////////////////////////////////////////////////////////
    // check player movable radius,
    if ( candidate != old_unknown_players.end()
         && target_list )
    {
        // update & splice to new list
        candidate->updateBySee( side, player );

        new_known_players.splice( new_known_players.end(),
                                  *target_list,
                                  candidate );
        return;
    }

    //////////////////////////////////////////////////////////////////
    // generate new player

#ifdef DEBUG_PRINT_PLAYER_UPDATE
    dlog.addText( Logger::WORLD,
                  __FILE__" (checkTeamPlayer)"
                  " XXXXX unmatch. min_dist= %.2f"
                  " generate new known player pos=(%.2f, %.2f)",
                  min_dist,
                  player.pos_.x, player.pos_.y );
#endif

    new_known_players.push_back( PlayerObject( side, player ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::checkUnknownPlayer( const Localization::PlayerT & player,
                                const double & seen_dist,
                                PlayerCont & old_teammates,
                                PlayerCont & old_opponents,
                                PlayerCont & old_unknown_players,
                                PlayerCont & new_teammates,
                                PlayerCont & new_opponents,
                                PlayerCont & new_unknown_players )
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
    //  if matched player is found, that player is removed from old list
    //  and updated data is splice to new container
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

    //////////////////////////////////////////////////////////////////
#if 0
    // if seen unknown player is within visible distance(=behind of agents)
    // it is very risky to match the exsiting player
    if ( seen_dist < ServerParam::i().visibleDistance() + 0.2 )
    {
        // generate new player
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkUnknownPlayer) behind. "
                      "  generate new unknown player. pos=(%.2f, %.2f)",
                      player.pos_.x, player.pos_.y );

        new_unknown_players.push_back( PlayerObject( player ) );

        return;
    }
#endif

    static const
        double player_speed_max
        = ServerParam::i().defaultPlayerSpeedMax() * 1.1;

    const double quantize_buf
        = unquantize_error( seen_dist, ServerParam::i().distQuantizeStep() );

    // matching start
    // search the nearest player

    double min_opponent_dist = 10.0 * 10.0;
    double min_teammate_dist = 10.0 * 10.0;
    double min_unknown_dist = 10.0 * 10.0;

    PlayerCont::iterator candidate_opponent = old_opponents.end();
    PlayerCont::iterator candidate_teammate = old_teammates.end();
    PlayerCont::iterator candidate_unknown = old_unknown_players.end();

    //////////////////////////////////////////////////////////////////
    // search from old opponents
    {
        const PlayerCont::iterator end = old_opponents.end();
        for ( PlayerCont::iterator it = old_opponents.begin();
              it != end;
              ++it )
        {
            double d = ( player.pos_ - it->pos() ).r();
            double buf = ( seen_dist < 3.2
                           ? 0.2
                           : 2.0 );

            if ( d > ( player_speed_max * it->posCount()
                       + quantize_buf * 2.0
                       + buf )
                 )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ opp player: dist over."
                              " dist=%.2f > buf=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              ( player_speed_max * it->posCount()
                                + quantize_buf * 2.0
                                + ( it->posCount() * 1.2 )
                                + buf ),
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                continue;
            }

            if ( d < min_opponent_dist )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ opp player: update."
                              " dist=%.2f < min_opp_dist=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              min_opponent_dist,
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                min_opponent_dist = d;
                candidate_opponent = it;
            }
        }
    }

    //////////////////////////////////////////////////////////////////
    // search from old teammates
    {
        const PlayerCont::iterator end = old_teammates.end();
        for ( PlayerCont::iterator it = old_teammates.begin();
              it != end;
              ++it )
        {
            double d = ( player.pos_ - it->pos() ).r();
            double buf = ( seen_dist < 3.2
                           ? 0.2
                           : 2.0 );

            if ( d > ( player_speed_max * it->posCount()
                       + quantize_buf * 2.0
                       + buf )
                 )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ our player: dist over."
                              " dist=%.2f > buf=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              ( player_speed_max * it->posCount()
                                + quantize_buf * 2.0
                                + buf ),
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                continue;
            }

            if ( d < min_teammate_dist )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ our player: update."
                              " dist=%.2f < min_our_dist=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              min_teammate_dist,
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                min_teammate_dist = d;
                candidate_teammate = it;
            }
        }
    }

    //////////////////////////////////////////////////////////////////
    // search from old unknown players
    {
        const PlayerCont::iterator end = old_unknown_players.end();
        for ( PlayerCont::iterator it = old_unknown_players.begin();
              it != end;
              ++it )
        {
            double d = ( player.pos_ - it->pos() ).r();
            double buf = ( seen_dist < 3.2
                           ? 0.2
                           : 2.0 );

            if ( d > ( player_speed_max * it->posCount()
                       + quantize_buf * 2.0
                       + buf )
                 )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ unknown player: dist over."
                              " dist=%.2f > buf=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              ( player_speed_max * it->posCount()
                                + quantize_buf * 2.0
                                + buf ),
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                continue;
            }

            if ( d < min_unknown_dist )
            {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkUnknownPlayer)"
                              "______ unknown player: update."
                              " dist=%.2f < min_unknown_dist=%.2f"
                              " seen_pos(%.1f %.1f) old_pos(%.1f %.1f)",
                              d,
                              min_unknown_dist,
                              player.pos_.x, player.pos_.y,
                              it->pos().x, it->pos().y );
#endif
                min_unknown_dist = d;
                candidate_unknown = it;
            }
        }
    }

    PlayerCont::iterator candidate = old_unknown_players.end();;
    double min_dist = 1000.0;
    PlayerCont * new_list = static_cast< PlayerCont * >( 0 );
    PlayerCont * old_list = static_cast< PlayerCont * >( 0 );
    SideID side = NEUTRAL;

    if ( candidate_teammate != old_teammates.end()
         && min_teammate_dist < min_opponent_dist
         && min_teammate_dist < min_unknown_dist )
    {
        candidate = candidate_teammate;
        min_dist = min_teammate_dist;
        new_list = &new_teammates;
        old_list = &old_teammates;
        side = ourSide();

#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkUnknownPlayer)"
                      "--- (%.1f %.1f) -> teammate %d (%.1f %.1f) dist=%.2f",
                      player.pos_.x, player.pos_.y,
                      candidate->unum(),
                      candidate->pos().x, candidate->pos().y,
                      min_dist );
#endif
    }

    if ( candidate_opponent != old_opponents.end()
         && min_opponent_dist * 0.5 - 3.0 < min_teammate_dist
         && min_opponent_dist < min_unknown_dist )
    {
        candidate = candidate_opponent;
        min_dist = min_opponent_dist;
        new_list = &new_opponents;
        old_list = &old_opponents;
        side = theirSide();

#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkUnknownPlayer)"
                      "--- (%.1f %.1f) -> opponent %d (%.1f %.1f) dist=%.2f",
                      player.pos_.x, player.pos_.y,
                      candidate->unum(),
                      candidate->pos().x, candidate->pos().y,
                      min_dist );
#endif
    }

    if ( candidate_unknown != old_unknown_players.end()
         && min_unknown_dist * 0.5 - 3.0 < min_teammate_dist
         && min_unknown_dist < min_opponent_dist )
    {
        candidate = candidate_unknown;
        min_dist = min_unknown_dist;
        new_list = &new_unknown_players;
        old_list = &old_unknown_players;
        side = NEUTRAL;

#ifdef DEBUG_PRINT_PLAYER_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkUnknownPlayer)"
                      "--- (%.1f %.1f) -> unknown (%.1f %.1f) dist=%.2f",
                      player.pos_.x, player.pos_.y,
                      candidate->pos().x, candidate->pos().y,
                      min_dist );
#endif
    }


    //////////////////////////////////////////////////////////////////
    // check player movable radius
    if ( candidate != old_unknown_players.end()
         && new_list
         && old_list )
    {
        // update & splice to new list
        candidate->updateBySee( side, player );
        new_list->splice( new_list->end(),
                          *old_list,
                          candidate );
        return;
    }

    //////////////////////////////////////////////////////////////////
    // generate new player
#ifdef DEBUG_PRINT_PLAYER_UPDATE
    dlog.addText( Logger::WORLD,
                  __FILE__" (checkUnknownPlayer)"
                  " XXXXX unmatch. quant_buf= %.2f"
                  " generate new unknown player. pos=(%.2f, %.2f)",
                  quantize_buf,
                  player.pos_.x, player.pos_.y );
#endif

    new_unknown_players.push_back( PlayerObject( NEUTRAL, player ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updatePlayerType()
{
    for ( PlayerCont::iterator it = M_teammates.begin(),
              end = M_teammates.end();
          it != end;
          ++it )
    {
        int n = it->unum() - 1;
        if ( 0 <= n && n < 11 )
        {
            it->setPlayerType( M_teammate_types[n] );
        }
        else
        {
            it->setPlayerType( Hetero_Default );
        }
    }

    for ( PlayerCont::iterator it = M_opponents.begin(),
              end = M_opponents.end();
          it != end;
          ++it )
    {
        int n = it->unum() - 1;
        if ( 0 <= n && n < 11 )
        {
            it->setPlayerType( M_opponent_types[n] );
        }
        else
        {
            it->setPlayerType( Hetero_Unknown );
        }
    }

    for ( PlayerCont::iterator it = M_unknown_players.begin(),
              end = M_unknown_players.end();
          it != end;
          ++it )
    {
        it->setPlayerType( Hetero_Unknown );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updatePlayerCard()
{
    for ( PlayerCont::iterator it = M_teammates.begin(),
              end = M_teammates.end();
          it != end;
          ++it )
    {
        int n = it->unum() - 1;
        if ( 0 <= n && n < 11 )
        {
            it->setCard( M_teammate_card[n] );
        }
    }

    for ( PlayerCont::iterator it = M_opponents.begin(),
              end = M_opponents.end();
          it != end;
          ++it )
    {
        int n = it->unum() - 1;
        if ( 0 <= n && n < 11 )
        {
            it->setCard( M_opponent_card[n] );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::estimateUnknownPlayerUnum()
{
    // 2008-07-03 akiyama
    // estimate unknown player's uniform number
    if ( M_teammates_from_self.size() == 10 )
    {
        std::set< int > unum_set;
        for ( int i = 1; i < MAX_PLAYER; ++i ) unum_set.insert( i );
        unum_set.erase( self().unum() );

        PlayerObject * unknown_teammate = static_cast< PlayerObject * >( 0 );
        const PlayerPtrCont::iterator t_end = M_teammates_from_self.end();
        for ( PlayerPtrCont::iterator t = M_teammates_from_self.begin();
              t != t_end;
              ++t )
        {
            if ( (*t)->unum() != Unum_Unknown )
            {
                unum_set.erase( (*t)->unum() );
            }
            else
            {
                unknown_teammate = *t;
            }
        }

        if ( unum_set.size() == 1
             && unknown_teammate )
        {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerStateCache)"
                          " set teammate unum %d (%.1f %.1f)",
                          *unum_set.begin(),
                          unknown_teammate->pos().x, unknown_teammate->pos().y );
//             std::cerr << self().unum() << ": " << this->time()
//                       << " updatePlayerStateCache  set teammate unum "
//                       << *unum_set.begin() << ' '
//                       << unknown_teammate->pos() << std::endl;
#endif
            int unum = *unum_set.begin();
            unknown_teammate->setTeam( ourSide(),
                                       unum,
                                       unum == M_our_goalie_unum );
        }
    }

    if ( M_teammates_from_self.size() == 10
         && M_opponents_from_self.size() == 11 )
    {
        std::set< int > unum_set;
        for ( int i = 1; i < MAX_PLAYER; ++i ) unum_set.insert( i );

        PlayerObject * unknown_opponent = static_cast< PlayerObject * >( 0 );
        const PlayerPtrCont::iterator o_end = M_opponents_from_self.end();
        for ( PlayerPtrCont::iterator o = M_opponents_from_self.begin();
              o != o_end;
              ++o )
        {
            if ( (*o)->unum() != Unum_Unknown )
            {
                unum_set.erase( (*o)->unum() );
            }
            else
            {
                unknown_opponent = *o;
            }
        }

        if ( unum_set.size() == 1
             && unknown_opponent )
        {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (updatePlayerStateCache)"
                          " set opponent unum %d (%.1f %.1f)",
                          *unum_set.begin(),
                          unknown_opponent->pos().x, unknown_opponent->pos().y );
//             std::cerr << self().unum() << ": " << this->time()
//                       << " updatePlayerStateCache  set opponent unum "
//                       << *unum_set.begin() << ' '
//                       << unknown_opponent->pos() << std::endl;
#endif
            if ( unknown_opponent->side() != theirSide() )
            {
                PlayerCont::iterator u = M_unknown_players.end();
                for ( PlayerCont::iterator p = M_unknown_players.begin();
                      p != M_unknown_players.end();
                      ++p )
                {
                    if ( &(*p) == unknown_opponent )
                    {
                        u = p;
                        break;
                    }
                }

                if ( u != M_unknown_players.end() )
                {
                    M_opponents.splice( M_opponents.end(),
                                        M_unknown_players,
                                        u );
                    int unum = *unum_set.begin();
                    unknown_opponent->setTeam( theirSide(),
                                               unum,
                                               unum == M_their_goalie_unum );
                }
            }
            else
            {
                int unum = *unum_set.begin();
                unknown_opponent->setTeam( theirSide(),
                                           unum,
                                           unum == M_their_goalie_unum );
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updatePlayerStateCache()
{
    //M_teammates_from_self.clear();
    //M_opponents_from_self.clear();
    //M_teammates_from_ball.clear();
    //M_opponents_from_ball.clear();

    if ( ! self().posValid()
         || ! ball().posValid() )
    {
        return;
    }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
    dlog.addText( Logger::WORLD,
                  __FILE__" (updatePlayerStateCache" );
#endif

    //
    // create player reference container
    //
    create_player_set( M_teammates,
                       M_teammates_from_self,
                       M_teammates_from_ball,
                       self().pos(),
                       ball().pos() );
    create_player_set( M_opponents,
                       M_opponents_from_self,
                       M_opponents_from_ball,
                       self().pos(),
                       ball().pos() );
    create_player_set( M_unknown_players,
                       M_opponents_from_self,
                       M_opponents_from_ball,
                       self().pos(),
                       ball().pos() );

    //
    // sort by distance from self or ball
    //
    std::sort( M_teammates_from_self.begin(),
               M_teammates_from_self.end(),
               PlayerObject::PtrSelfDistCmp() );
    std::sort( M_opponents_from_self.begin(),
               M_opponents_from_self.end(),
               PlayerObject::PtrSelfDistCmp() );

    std::sort( M_teammates_from_ball.begin(),
               M_teammates_from_ball.end(),
               PlayerObject::PtrBallDistCmp() );
    std::sort( M_opponents_from_ball.begin(),
               M_opponents_from_ball.end(),
               PlayerObject::PtrBallDistCmp() );

    //
    // update teammate goalie's unum
    //
    if ( M_our_goalie_unum == Unum_Unknown )
    {
        const AbstractPlayerObject * p = getOurGoalie();
        if ( p )
        {
            M_our_goalie_unum = p->unum();
        }
    }

    //
    // update opponent goalie's unim
    //
    if ( M_their_goalie_unum == Unum_Unknown )
    {
        const PlayerObject * p = getOpponentGoalie();
        if ( p )
        {
            M_their_goalie_unum = p->unum();
        }
    }

    //
    // create known players array
    //
    {
        M_all_players.push_back( &M_self );
        M_our_players.push_back( &M_self );
        M_known_teammates[self().unum()] = &M_self;

        const PlayerPtrCont::iterator t_end = M_teammates_from_ball.end();
        for ( PlayerPtrCont::iterator t = M_teammates_from_ball.begin();
              t != t_end;
              ++t )
        {
            M_all_players.push_back( *t );
            M_our_players.push_back( *t );

            if ( (*t)->unum() != Unum_Unknown )
            {
                M_known_teammates[(*t)->unum()] = *t;
            }
        }

        const PlayerPtrCont::iterator o_end = M_opponents_from_ball.end();
        for ( PlayerPtrCont::iterator o = M_opponents_from_ball.begin();
              o != o_end;
              ++o )
        {
            M_all_players.push_back( *o );
            M_their_players.push_back( *o );

            if ( (*o)->unum() != Unum_Unknown )
            {
                M_known_opponents[(*o)->unum()] = *o;
            }
        }

    }

    //
    // check kickable player
    //
    M_exist_kickable_teammate
        = check_player_kickable( M_teammates_from_ball.begin(),
                                 M_teammates_from_ball.end(),
                                 ball().posCount(),
                                 0.0,
                                 0.0 );
    M_exist_kickable_opponent
        = check_player_kickable( M_opponents_from_ball.begin(),
                                 M_opponents_from_ball.end(),
                                 ball().posCount(),
                                 std::min( 0.25, ball().distFromSelf() * 0.02 ),
                                 0.02 );
#ifdef DEBUG_PRINT_PLAYER_UPDATE
    dlog.addText( Logger::WORLD,
                  __FILE__" (updatePlayerStateCache)"
                  " size of player set"
                  " ourFromSelf %d"
                  " ourFromBall %d"
                  " oppFromSelf %d"
                  " oppFromBall %d",
                  M_teammates_from_self.size(),
                  M_teammates_from_ball.size(),
                  M_opponents_from_self.size(),
                  M_opponents_from_ball.size() );

    dlog.addText( Logger::WORLD,
                  __FILE__" (updatePlayerMatrix)"
                  " opponent goalie = %d",
                  M_opponent_goalie_unum );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateOffsideLine()
{
    if ( ! ServerParam::i().useOffside() )
    {
        M_offside_line_count = 0;
        M_offside_line_x = ServerParam::i().pitchHalfLength();
        return;
    }

    if ( gameMode().type() == GameMode::KickIn_
         || gameMode().type() == GameMode::CornerKick_
         || ( gameMode().type() == GameMode::GoalKick_
              && gameMode().side() == ourSide() )
         )
    {
        M_offside_line_count = 0;
        M_offside_line_x = ServerParam::i().pitchHalfLength();
        return;
    }

    if ( gameMode().side() != ourSide()
         && ( gameMode().type() == GameMode::GoalieCatch_
              || gameMode().type() == GameMode::GoalKick_ )
         )
    {
        M_offside_line_count = 0;
        M_offside_line_x = ServerParam::i().theirPenaltyAreaLineX();
        return;
    }

    double new_line = M_their_defense_line_x;
    int count = M_their_defense_line_count;

    if ( M_audio_memory->offsideLineTime() == this->time()
         && ! M_audio_memory->offsideLine().empty() )
    {
        double heard_x = 0.0;
        for ( std::vector< AudioMemory::OffsideLine >::const_iterator it
                  = M_audio_memory->offsideLine().begin();
              it != M_audio_memory->offsideLine().end();
              ++it )
        {
            heard_x += it->x_;
        }
        heard_x /= static_cast< double >( M_audio_memory->offsideLine().size() );

        if ( new_line < heard_x - 1.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateOffsideLine) by heard info. %.1f -> %.1f",
                          new_line, heard_x );
#endif

            new_line = heard_x;
            count = 30;
        }
    }

    M_offside_line_x = new_line;
    M_offside_line_count = count;

#ifdef DEBUG_PRINT_LINES
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateOffsideLine) x=%.2f count=%d",
                  new_line, count );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateOurOffenseLine()
{
    double new_line = -ServerParam::i().pitchHalfLength();

    const AbstractPlayerCont::const_iterator end = ourPlayers().end();
    for ( AbstractPlayerCont::const_iterator it = ourPlayers().begin();
          it != end;
          ++it )
    {
        new_line = std::max( new_line, (*it)->pos().x );
    }

    if ( ourPlayers().empty() )
    {
        // new_line is used
    }
    else if ( ourPlayers().size() >= 11 )
    {
        // new_line is used
    }
    else if ( new_line < M_our_offense_line_x - 13.0 )
    {
        // new_line is used
    }
    else if ( new_line < M_our_offense_line_x - 5.0 )
    {
        new_line = M_our_offense_line_x - 1.0;
    }

    if ( ball().posValid()
         && ball().pos().x > new_line )
    {
        new_line = ball().pos().x;
    }

    M_our_offense_line_x = new_line;

#ifdef DEBUG_PRINT_LINES
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateOurOffenseLine) x=%.2f",
                  new_line );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateOurDefenseLine()
{
    double first = 0.0, second = 0.0;
    {
        const AbstractPlayerCont::const_iterator end = ourPlayers().end();
        for ( AbstractPlayerCont::const_iterator it = ourPlayers().begin();
              it != end;
              ++it )
        {
            double x = (*it)->pos().x;
            if ( x < second )
            {
                second = x;
                if ( second < first )
                {
                    std::swap( first, second );
                }
            }
        }
    }

    double new_line = second;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateDefenseLine) base line=%.1f",
                  new_line );
#endif

    const AbstractPlayerObject * goalie = getOurGoalie();
    if ( ! goalie )
    {
        if ( first > ServerParam::i().ourPenaltyAreaLineX() )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateDefenseLine) goalie not found,"
                          " assume goalie is back of defense line. %.1f -> %.1f",
                          new_line, first );
#endif
            new_line = first;
        }
    }

    // consider old line
    if ( ourPlayers().size() >= 11 )
    {
        // new_line is used
    }
    else if ( new_line > M_our_defense_line_x + 13.0 )
    {
        // new_line is used
    }
    else if ( new_line > M_our_defense_line_x + 5.0 )
    {
        new_line = M_our_defense_line_x + 1.0;
    }

    // ball exists on behind of our defense line
    if ( ball().posValid()
         && ball().pos().x < new_line )
    {
        new_line = ball().pos().x;
    }

    if ( M_audio_memory->defenseLineTime() == this->time()
         && ! M_audio_memory->defenseLine().empty() )
    {
        double heard_x = 0.0;
        for ( std::vector< AudioMemory::DefenseLine >::const_iterator it
                  = M_audio_memory->defenseLine().begin();
              it != M_audio_memory->defenseLine().end();
              ++it )
        {
            heard_x += it->x_;
        }
        heard_x /= static_cast< double >( M_audio_memory->defenseLine().size() );

        if ( heard_x + 1.0 < new_line )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateDefenseLine) heard defense line is used. %.1f -> %.1f",
                          new_line, heard_x );
#endif

            new_line = heard_x;
        }
    }

    M_our_defense_line_x = new_line;

#ifdef DEBUG_PRINT_LINES
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateOurDefenseLine) %.2f",
                  new_line );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateTheirOffenseLine()
{
    double new_line = ServerParam::i().pitchHalfLength();
    {
        const AbstractPlayerCont::const_iterator end = theirPlayers().end();
        for ( AbstractPlayerCont::const_iterator it = theirPlayers().begin();
              it != end;
              ++it )
        {
            new_line = std::min( new_line, (*it)->pos().x );
        }
    }

    // consider old line
    if ( theirPlayers().size() >= 11 )
    {
        // new_line is used
    }
    else if ( new_line > M_their_offense_line_x + 13.0 )
    {
        // new_line is used
    }
    else if ( new_line > M_their_offense_line_x + 5.0 )
    {
        new_line = M_their_offense_line_x + 1.0;
    }

    if ( ball().posValid()
         && ball().pos().x < new_line )
    {
        new_line = ball().pos().x;
    }

    M_their_offense_line_x = new_line;

#ifdef DEBUG_PRINT_LINES
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateTheirOffenseLine) x=%.2f",
                  new_line );
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateTheirDefenseLine()
{
    const double speed_rate
        = ( ball().vel().x < -1.0
            ? ServerParam::i().defaultPlayerSpeedMax() * 0.8
            : ServerParam::i().defaultPlayerSpeedMax() * 0.25 );

    //////////////////////////////////////////////////////////////////
    double first = 0.0, second = 0.0;
    int first_count = 1000, second_count = 1000;

    {
        const PlayerPtrCont::const_iterator end = M_opponents_from_self.end();
        for ( PlayerPtrCont::const_iterator it = M_opponents_from_self.begin();
              it != end;
              ++it )
        {
            double x = (*it)->pos().x;
#if 1
            // 2008-04-29 akiyama
            if ( (*it)->velCount() <= 1
                 && (*it)->vel().x > 0.0 )
            {
                x += std::min( 0.8, (*it)->vel().x / (*it)->playerTypePtr()->playerDecay() );
            }
            else if ( (*it)->bodyCount() <= 3
                      && (*it)->body().abs() < 100.0 )
            {
                x -= speed_rate * std::min( 10.0, (*it)->posCount() - 1.5 );
            }
            else
#endif
            {
                x -= speed_rate * std::min( 10, (*it)->posCount() );
            }

            if ( x > second )
            {
                second = x;
                second_count = (*it)->posCount();
                if ( second > first )
                {
                    std::swap( first, second );
                    std::swap( first_count, second_count );
                }
            }
        }
    }

    double new_line = second;
    int count = second_count;

    const PlayerObject * goalie = getOpponentGoalie();
    if ( ! goalie )
    {
        if ( 20.0 < ball().pos().x
             && ball().pos().x < ServerParam::i().theirPenaltyAreaLineX() )
        {
            if ( first < ServerParam::i().theirPenaltyAreaLineX() )
            {
#ifdef DEBUG_PRINT
                dlog.addText( Logger::WORLD,
                              __FILE__" (updateTheirDefenseLine) no goalie. %.1f -> %.1f",
                              second, first );
#endif
                new_line = first;
                count = 30;
            }
        }
    }

    if ( M_opponents_from_self.size() >= 11 )
    {
        // new_line is used directly
    }
    else if ( new_line < M_their_defense_line_x - 13.0 )
    {
        // new_line is used directly
    }
    else if ( new_line < M_their_defense_line_x - 5.0 )
    {
        new_line = M_their_defense_line_x - 1.0;
    }

    if ( new_line < 0.0 )
    {
        new_line = 0.0;
    }

    // ball is more forward than opponent defender line
    if ( gameMode().type() != GameMode::BeforeKickOff
         && gameMode().type() != GameMode::AfterGoal_
         && ball().posCount() <= 3 )
    {
        Vector2D ball_next = ball().pos() + ball().vel();
        if ( ball_next.x > new_line )
        {
            new_line = ball_next.x;
            count = ball().posCount();
        }
    }

    M_their_defense_line_x = new_line;
    M_their_defense_line_count = count;

#ifdef DEBUG_PRINT_LINES
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateTheirDefenseLine) x=%.2f count=%d",
                  new_line, count );
#endif
}


/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updatePlayerLines()
{
    const ServerParam & SP = ServerParam::i();
    {
        double max_x = -SP.pitchHalfLength();
        double min_x = +SP.pitchHalfLength();
        double second_min_x = +SP.pitchHalfLength();

        const AbstractPlayerCont::const_iterator end = ourPlayers().end();
        for ( AbstractPlayerCont::const_iterator it = ourPlayers().begin();
              it != end;
              ++it )
        {
            double x = (*it)->pos().x;

            if ( x > max_x )
            {
                max_x = x;
            }

            if ( x < second_min_x )
            {
                second_min_x = x;
                if ( second_min_x < min_x )
                {
                    std::swap( min_x, second_min_x );
                }
            }
        }

        M_our_offense_player_line_x = max_x;
        M_our_defense_player_line_x = second_min_x;

        const AbstractPlayerObject * goalie = getOurGoalie();
        if ( ! goalie )
        {
            if ( min_x > SP.ourPenaltyAreaLineX() )
            {
                M_our_defense_player_line_x = min_x;
            }
        }
#ifdef DEBUG_PRINT_LINES
        dlog.addText( Logger::WORLD,
                      __FILE__" (updatePlayerLines) our_offensex=%.2f our_defense=%.2f",
                      M_our_offense_player_line_x,
                      M_our_defense_player_line_x );
#endif
    }

    {
        double min_x = +SP.pitchHalfLength();
        double max_x = -SP.pitchHalfLength();
        double second_max_x = -SP.pitchHalfLength();
        const AbstractPlayerCont::const_iterator end = theirPlayers().end();
        for ( AbstractPlayerCont::const_iterator it = theirPlayers().begin();
              it != end;
              ++it )
        {
            double x = (*it)->pos().x;

            if ( x < min_x )
            {
                min_x = x;
            }

            if ( x > second_max_x )
            {
                second_max_x = x;
                if ( second_max_x > max_x )
                {
                    std::swap( max_x, second_max_x );
                }
            }
        }

        M_their_offense_player_line_x = min_x;
        M_their_defense_player_line_x = second_max_x;

        const PlayerObject * goalie = getOpponentGoalie();
        if ( ! goalie )
        {
            if ( max_x < SP.theirPenaltyAreaLineX() )
            {
                M_their_defense_player_line_x = max_x;
            }
        }
#ifdef DEBUG_PRINT_LINES
        dlog.addText( Logger::WORLD,
                      __FILE__" (updatePlayerLines) their_offensex=%.2f their_defense=%.2f",
                      M_their_offense_player_line_x,
                      M_their_defense_player_line_x );
#endif
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateLastKicker()
{
    // if ( ! self().goalie() )
    // {
    //     return;
    // }

    if ( gameMode().type() != GameMode::PlayOn )
    {
        if ( gameMode().isOurSetPlay( ourSide() ) )
        {
            M_last_kicker_side = ourSide();
        }
        else if ( gameMode().isTheirSetPlay( ourSide() ) )
        {
            M_last_kicker_side = theirSide();
        }
        else
        {
            M_last_kicker_side = NEUTRAL;
        }

        return;
    }

    if ( self().kicked() )
    {
        M_last_kicker_side = ourSide();

        dlog.addText( Logger::WORLD,
                      __FILE__" (updateLastKicker) self kicked" );
        return;
    }

    if ( ball().stateRecord().empty() )
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateLastKicker) no ball record" );
        return;
    }

    const ServerParam & SP = ServerParam::i();
    const BallObject::State & prev_state = ball().stateRecord().front();

    //
    // check seen kicker or tackler
    //

    AbstractPlayerCont kickers;

    for ( int i = 0; i < 2; ++i )
    {
        const PlayerPtrCont & players = ( i == 0
                                          ? teammatesFromBall()
                                          : opponentsFromBall() );

        const PlayerPtrCont::const_iterator end = players.end();
        for ( PlayerPtrCont::const_iterator p = players.begin();
              p != end;
              ++p )
        {
            if ( (*p)->kicked()
                 && (*p)->distFromBall() < SP.ballSpeedMax() * 2.0 )
            {
                kickers.push_back( *p );
                dlog.addText( Logger::WORLD,
                              __FILE__" (updateLastKicker) see kicking side=%c unum=%d",
                              ( (*p)->side() == LEFT ? 'L' : (*p)->side() == RIGHT ? 'R' : 'N' ),
                          (*p)->unum() );
            }
            else if ( (*p)->tackleCount() == 0
                      && (*p)->distFromBall() < SP.ballSpeedMax() * 2.0 )
            {
                kickers.push_back( *p );
                dlog.addText( Logger::WORLD,
                              __FILE__" (updateLastKicker) see tackling side=%c unum=%d",
                              ( (*p)->side() == LEFT ? 'L' : (*p)->side() == RIGHT ? 'R' : 'N' ),
                          (*p)->unum() );
            }
        }
    }

    //
    // check ball velocity change
    //

    double angle_diff = ( ball().vel().th() - prev_state.vel_.th() ).abs();
    double prev_speed = prev_state.vel_.r();
    double cur_speed = ball().vel().r();

    bool ball_vel_changed = false;
    if ( cur_speed > prev_speed + 0.1
         || cur_speed < prev_speed * SP.ballDecay() * 0.5
         || ( prev_speed > 0.5         // Magic Number
              && angle_diff > 20.0 ) ) // Magic Number
    {
        ball_vel_changed = true;
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateLastKicker) ball vel changed." );
        dlog.addText( Logger::WORLD,
                      "__ curSpeed=%.3f prevSpeed=%.3f angleDiff=%.1f" );
    }

    //
    // check kicker
    //

    if ( ball_vel_changed
         && ! kickers.empty() )
    {
        if ( kickers.size() == 1 )
        {
            const AbstractPlayerObject * kicker = kickers.front();
            if ( kicker->distFromBall() < SP.ballSpeedMax() * 2.0 )
            {
                if ( kicker->side() != theirSide() )
                {
                    M_last_kicker_side = ourSide();
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (updateLastKicker) set by 1 seen kicker. side=%d unum=%d -> teammate",
                                  ( kicker->side() == LEFT ? 'L' : kicker->side() == RIGHT ? 'R' : 'N' ),
                                  kicker->unum() );
                }
                else
                {
                    M_last_kicker_side = theirSide();
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (updateLastKicker) set by 1 seen kicker. side=%d unum=%d -> opponent",
                                  kicker->side(),
                                  kicker->unum() );
                }
                return;
            }
        }

        bool exist_teammate_kicker = false;
        bool exist_opponent_kicker = false;
        for ( AbstractPlayerCont::const_iterator p = kickers.begin();
              p != kickers.end();
              ++p )
        {
            if ( (*p)->distFromBall() > SP.ballSpeedMax() * 2.0 )
            {
                continue;
            }

            if ( (*p)->side() == ourSide() )
            {
                // teammate
                exist_teammate_kicker = true;
            }
            else
            {
                // opponent
                exist_opponent_kicker = true;
            }
        }

        if ( exist_teammate_kicker
             && exist_opponent_kicker )
        {
            M_last_kicker_side = NEUTRAL;
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateLastKicker) set by seen kicker(s). NEUTRAL"
                          " kicked by teammate and opponent" );
        }
        else if ( ! exist_opponent_kicker )
        {
            M_last_kicker_side = ourSide();
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateLastKicker) set by seen kicker(s). TEAMMATE"
                          " kicked by teammate or unknown" );
        }
        else if ( ! exist_teammate_kicker )
        {
            M_last_kicker_side = theirSide();
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateLastKicker) set by seen kicker(s). OPPONENT"
                          " kicked by opponent" );
        }

        return;
    }

    //
    // check ball nearest player
    //
    if ( ball_vel_changed )
    {
        const double dist_thr2 = std::pow( SP.ballSpeedMax(), 2 );
        bool exist_teammate_kicker = false;
        bool exist_opponent_kicker = false;

        const AbstractPlayerObject * nearest = static_cast< AbstractPlayerObject * >( 0 );
        double min_dist = std::numeric_limits< double >::max();
        double second_min_dist = std::numeric_limits< double >::max();

        const AbstractPlayerCont::const_iterator all_end = allPlayers().end();
        for ( AbstractPlayerCont::const_iterator p = allPlayers().begin();
          p != all_end;
          ++p )
        {
            if ( (*p)->side() == ourSide()
                 && (*p)->unum() == self().unum() )
            {
                continue;
            }

            double d2 = (*p)->pos().dist2( prev_state.pos_ );
            if ( d2 < dist_thr2 )
            {
                if ( (*p)->side() != theirSide() )
                {
                    exist_teammate_kicker = true;
                }
                else
                {
                    exist_opponent_kicker = true;
                }
            }

            if ( d2 < second_min_dist )
            {
                second_min_dist = d2;
                if ( second_min_dist < min_dist )
                {
                    std::swap( min_dist, second_min_dist );
                    nearest = *p;
                }
            }
        }

        min_dist = std::sqrt( min_dist );
        second_min_dist = std::sqrt( second_min_dist );

        //
        // exist ball nearest player
        //
        if ( nearest
             && min_dist < SP.ballSpeedMax()
             && min_dist < second_min_dist - 3.0 )
        {
            const PlayerType * ptype = nearest->playerTypePtr();
            double kickable_move_dist
                = ptype->kickableArea() + ( ptype->realSpeedMax()
                                            * std::pow( ptype->playerDecay(), 2 ) );
            double tackle_dist = ( nearest->isTackling()
                                   ? SP.tackleDist()
                                   : 0.0 );
            if ( nearest->pos().dist( prev_state.pos_ ) < std::max( kickable_move_dist, tackle_dist ) )
            {
                if ( nearest->side() != theirSide() )
                {
                    M_last_kicker_side = ourSide();
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (updateLastKicker) set by nearest teammate or unknown."
                                  " side=%c unum=%d",
                                  ( nearest->side() == LEFT ? 'L'
                                : nearest->side() == RIGHT ? 'R'
                                    : 'N' ),
                                  nearest->unum() );
                }
                else
                {
                    M_last_kicker_side = theirSide();
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (updateLastKicker) set by nearest opponent."
                                  " side=%c unum=%d",
                                  ( nearest->side() == LEFT ? 'L'
                                    : nearest->side() == RIGHT ? 'R'
                                    : 'N' ),
                                  nearest->unum() );
                }

                return;
            }
        }

        if ( exist_teammate_kicker
             && exist_opponent_kicker )
        {
            M_last_kicker_side = NEUTRAL;
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateLastKicker) set NEUTRAL." );
            return;
        }
    }

    if ( ! kickers.empty() )
    {
        bool exist_teammate_kicker = false;
        for ( AbstractPlayerCont::const_iterator p = kickers.begin();
              p != kickers.end();
              ++p )
        {
            if ( (*p)->side() == ourSide() )
            {
                exist_teammate_kicker = true;
            }
        }

        if ( exist_teammate_kicker )
        {
            M_last_kicker_side = ourSide();

            dlog.addText( Logger::WORLD,
                          __FILE__" (updateLastKicker) set by seen teammate kicker." );
            return;
        }
    }

    dlog.addText( Logger::WORLD,
                  __FILE__" (updateLastKicker) no updated. last_kicker_side=%c",
                  ( M_last_kicker_side == LEFT  ? 'L'
                    : M_last_kicker_side == RIGHT ? 'R'
                    : 'N' ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateInterceptTable()
{
    // update interception table
    M_intercept_table->update();

    if ( M_audio_memory->ourInterceptTime() == time() )
    {
        const std::vector< AudioMemory::OurIntercept >::const_iterator end
            = M_audio_memory->ourIntercept().end();
        for ( std::vector< AudioMemory::OurIntercept >::const_iterator it
                  = M_audio_memory->ourIntercept().begin();
              it != end;
              ++it )
        {
            M_intercept_table->hearTeammate( it->interceptor_,
                                             it->cycle_ );
        }
    }

    if ( M_audio_memory->oppInterceptTime() == time()
         && ! M_audio_memory->oppIntercept().empty() )
    {
        const std::vector< AudioMemory::OppIntercept >::const_iterator end
            = M_audio_memory->oppIntercept().end();
        for ( std::vector< AudioMemory::OppIntercept >::const_iterator it
                  = M_audio_memory->oppIntercept().begin();
              it != end;
              ++it )
        {
            M_intercept_table->hearOpponent( it->interceptor_,
                                             it->cycle_ );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::checkGhost( const ViewArea & varea )
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //  NOTE: this method is called from updateAfterSee

    const double angle_buf = 5.0;

    //////////////////////////////////////////////////////////////////
    // ball
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (checkGhost) ball_count=%d, rpos_count=%d",
                  ball().posCount(), ball().rposCount() );
#endif

    if ( ball().rposCount() > 0
         && ball().posValid() )
    {
        const double BALL_VIS_DIST2
            = square( ServerParam::i().visibleDistance()
                      - ( self().vel().r() / self().playerType().playerDecay() ) * 0.1
                      - ( ball().vel().r() / ServerParam::i().ballDecay() ) * 0.05
                      - ( 0.12 * std::min( 4, ball().posCount() ) )
                      - 0.25 );

        Vector2D ballrel = ball().pos() - varea.origin();

#ifdef DEBUG_PRINT_BALL_UPDATE
        dlog.addText( Logger::WORLD,
                      __FILE__" (checkGhost) check ball. global_dist=%.2f."
                      "  visdist=%.2f.  ",
                      ballrel.r(), std::sqrt( BALL_VIS_DIST2 ) );
#endif

        if ( varea.contains( ball().pos(), angle_buf, BALL_VIS_DIST2 ) )
        {
#ifdef DEBUG_PRINT_BALL_UPDATE
            dlog.addText( Logger::WORLD,
                          __FILE__" (checkGhost) forget ball." );
#endif
            M_ball.setGhost( this->time() );
        }
    }

    const double VIS_DIST2
            = square( ServerParam::i().visibleDistance()
                      - ( self().vel().r() / self().playerType().playerDecay() ) * 0.1
                      - 0.25 );
    //////////////////////////////////////////////////////////////////
    // players

    {
        PlayerCont::iterator it = M_teammates.begin();
        while ( it != M_teammates.end() )
        {
            if ( it->posCount() > 0
                 && varea.contains( it->pos(), angle_buf, VIS_DIST2 ) )
            {
                if ( it->unum() == Unum_Unknown
                     && it->posCount() >= 10
                     && it->ghostCount() >= 2 )
                {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (checkGhost) erase teammate (%.1f %.1f)",
                                  it->pos().x, it->pos().y );
#endif
                    it = M_teammates.erase( it );
                    continue;
                }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkGhost) setGhost to teammate %d (%.1f %.1f).",
                              it->unum(), it->pos().x, it->pos().y );
#endif
                it->setGhost();
            }

            ++it;
        }
    }

    {
        PlayerCont::iterator it = M_opponents.begin();
        while ( it != M_opponents.end() )
        {
            if ( it->posCount() > 0
                 && varea.contains( it->pos(), 1.0, VIS_DIST2 ) )
            {
                if ( it->unum() == Unum_Unknown
                     && it->posCount() >= 10
                     && it->ghostCount() >= 2 )
                {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (checkGhost) erase opponent (%.1f %.1f)",
                                  it->pos().x, it->pos().y );
#endif
                    it = M_opponents.erase( it );
                    continue;
                }

                dlog.addText( Logger::WORLD,
                              __FILE__" (checkGhost) setGhost to opponent %d (%.1f %.1f).",
                              it->unum(), it->pos().x, it->pos().y );
                it->setGhost();
            }

            ++it;
        }
    }

    {
        PlayerCont::iterator it = M_unknown_players.begin();
        while ( it != M_unknown_players.end() )
        {
            if ( it->posCount() > 0
                 && varea.contains( it->pos(), 1.0, VIS_DIST2 ) )
            {
                if ( it->distFromSelf() < 40.0 * 1.06
                     || it->isGhost() ) // detect twice
                {
#ifdef DEBUG_PRINT_PLAYER_UPDATE
                    dlog.addText( Logger::WORLD,
                                  __FILE__" (checkGhost) erase unknown player (%.1f %.1f)",
                                  it->pos().x, it->pos().y );
#endif
                    it = M_unknown_players.erase( it );
                    continue;
                }

#ifdef DEBUG_PRINT_PLAYER_UPDATE
                dlog.addText( Logger::WORLD,
                              __FILE__" (checkGhost) setGhost to unknown player (%.1f %.1f)",
                              it->pos().x, it->pos().y );
#endif
                it->setGhost();
            }

            ++it;
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::updateDirCount( const ViewArea & varea )
{
    const double dir_buf// = DIR_STEP * 0.5;
        = ( ( self().lastMove().isValid()
              && self().lastMove().r() > 0.5 )
            ? DIR_STEP * 0.5 + 1.0
            : DIR_STEP * 0.5 );

    const AngleDeg left_limit = varea.angle() - varea.viewWidth() * 0.5 + dir_buf;
    const AngleDeg right_limit = varea.angle() + varea.viewWidth() * 0.5 - dir_buf;

    AngleDeg left_dir = varea.angle() - varea.viewWidth() * 0.5;
    int idx = static_cast< int >( ( left_dir.degree() - 0.5 + 180.0 ) / DIR_STEP );

    AngleDeg dir = -180.0 + DIR_STEP * idx;

    while ( dir.isLeftOf( left_limit ) )
    {
        dir += DIR_STEP;
        idx += 1;
        if ( idx > DIR_CONF_DIVS ) idx = 0;
    }

#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateDirCount) left=%.1f right=%.1f dir buf=%.3f start_dir=%.1f start_idx=%d",
                  left_limit.degree(), right_limit.degree(),
                  dir_buf, dir.degree(), idx );
#endif

    while ( dir.isLeftOf( right_limit ) )
    {
        idx = static_cast< int >( ( dir.degree() - 0.5 + 180.0 ) / DIR_STEP );
        if ( idx > DIR_CONF_DIVS - 1 )
        {
            std::cerr << teamName() << " : " << self().unum()
                      << " DIR_CONF over flow  " << idx << std::endl;
            idx = DIR_CONF_DIVS - 1;
        }
        else if ( idx < 0 )
        {
            std::cerr << teamName() << " : " << self().unum()
                      << " DIR_CONF down flow  " << idx << std::endl;
            idx = 0;
        }
        //#ifdef DEBUG
#if 0
        dlog.addText( Logger::WORLD,
                       __FILE__" (updateDirCount) update dir. index=%d : angle=%.0f",
                      idx, dir.degree() );
#endif
        M_dir_count[idx] = 0;
        dir += DIR_STEP;
    }

    //#ifdef DEBUG
#if 0
    if ( dlog.isLogFlag( Logger::WORLD ) )
    {
        double d = -180.0;
        for ( int i = 0; i < DIR_CONF_DIVS; ++i, d += DIR_STEP )
        {
            dlog.addText( Logger::WORLD,
                           __FILE__" (updateDirCount) __ dir count: %.0f - %d",
                          d, M_dir_count[i] );
        }
    }
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
int
WorldModel::dirRangeCount( const AngleDeg & angle,
                           const double & width,
                           int * max_count,
                           int * sum_count,
                           int * ave_count ) const
{
    if ( width <= 0.0 || 360.0 < width )
    {
        std::cerr << M_time << " " << self().unum() << ":"
                  << " invalid dir range"
                  << std::endl;
        return 1000;
    }

    int counter = 0;
    int tmp_sum_count = 0;
    int tmp_max_count = 0;

    AngleDeg tmp_angle = angle;
    if ( width > DIR_STEP ) tmp_angle -= width * 0.5;

    double add_dir = 0.0;
    while ( add_dir < width )
    {
        int c = dirCount( tmp_angle );

        tmp_sum_count += c;

        if ( c > tmp_max_count )
        {
            tmp_max_count = c;
        }

        add_dir += DIR_STEP;
        tmp_angle += DIR_STEP;
        ++counter;
    }

    if ( max_count )
    {
        *max_count = tmp_max_count;
    }

    if ( sum_count )
    {
        *sum_count = tmp_sum_count;
    }

    if ( ave_count )
    {
        *ave_count = tmp_sum_count / counter;
    }

    return counter;
}

/*-------------------------------------------------------------------*/
/*!

*/
int
WorldModel::getPointCount( const Vector2D & point,
                           const double & dir_thr ) const
{
    const double vis_dist2 = square( ServerParam::i().visibleDistance() - 0.1 );

    int count = 0;
    const ViewAreaCont::const_iterator end = viewAreaCont().end();
    for ( ViewAreaCont::const_iterator it = viewAreaCont().begin();
          it != end;
          ++it, ++count )
    {
        if ( it->contains( point, dir_thr, vis_dist2 ) )
        {
            return count;
        }
    }

    return 1000;
}

/*-------------------------------------------------------------------*/
/*!

*/
AbstractPlayerCont
WorldModel::getPlayerCont( const PlayerPredicate * predicate ) const
{
    AbstractPlayerCont rval;

    if ( ! predicate ) return rval;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            rval.push_back( *it );
        }
    }

    delete predicate;
    return rval;
}

/*-------------------------------------------------------------------*/
/*!

*/
AbstractPlayerCont
WorldModel::getPlayerCont( boost::shared_ptr< const PlayerPredicate > predicate ) const
{
    AbstractPlayerCont rval;

    if ( ! predicate ) return rval;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            rval.push_back( *it );
        }
    }

    return rval;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::getPlayerCont( AbstractPlayerCont & cont,
                           const PlayerPredicate * predicate ) const
{
    if ( ! predicate ) return;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            cont.push_back( *it );
        }
    }

    delete predicate;
}


/*-------------------------------------------------------------------*/
/*!

*/
void
WorldModel::getPlayerCont( AbstractPlayerCont & cont,
                           boost::shared_ptr< const PlayerPredicate > predicate ) const
{
    if ( ! predicate ) return;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            cont.push_back( *it );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
size_t
WorldModel::countPlayer( const PlayerPredicate * predicate ) const
{
    size_t count = 0;

    if ( ! predicate ) return count;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            ++count;
        }
    }

    delete predicate;
    return count;
}

/*-------------------------------------------------------------------*/
/*!

*/
size_t
WorldModel::countPlayer( boost::shared_ptr< const PlayerPredicate > predicate ) const
{
    size_t count = 0;

    if ( ! predicate ) return count;

    const AbstractPlayerCont::const_iterator end = allPlayers().end();
    for( AbstractPlayerCont::const_iterator it = allPlayers().begin();
         it != end;
         ++it )
    {
        if ( (*predicate)( **it ) )
        {
            ++count;
        }
    }

    return count;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
AbstractPlayerObject *
WorldModel::getOurGoalie() const
{
    const PlayerCont::const_iterator end = M_teammates.end();
    for ( PlayerCont::const_iterator it = M_teammates.begin();
          it != end;
          ++it )
    {
        if ( it->goalie() )
        {
            return &(*it);
        }
    }

    if ( M_self.goalie() )
    {
        return &M_self;
    }

    return static_cast< AbstractPlayerObject * >( 0 );
}

/*-------------------------------------------------------------------*/
/*!

*/
const
PlayerObject *
WorldModel::getOpponentGoalie() const
{
    const PlayerCont::const_iterator end = M_opponents.end();
    for ( PlayerCont::const_iterator it = M_opponents.begin();
          it != end;
          ++it )
    {
        if ( it->goalie() )
        {
            return &(*it);
        }
    }

    return static_cast< PlayerObject * >( 0 );
}

/*-------------------------------------------------------------------*/
/*!

*/
const PlayerObject *
WorldModel::getPlayerNearestTo( const Vector2D & point,
                                const PlayerPtrCont & players,
                                const int count_thr,
                                double * dist_to_point ) const
{
    const PlayerObject * p = static_cast< PlayerObject * >( 0 );
    double min_dist2 = 40000.0;

    const PlayerPtrCont::const_iterator end = players.end();
    for ( PlayerPtrCont::const_iterator it = players.begin();
          it != end;
          ++it )
    {
        if ( (*it)->posCount() > count_thr )
        {
            continue;
        }

        double d2 = (*it)->pos().dist2(point);
        if ( d2 < min_dist2 )
        {
            p = *it;
            min_dist2 = d2;
        }
    }

    if ( p
         && dist_to_point )
    {
        *dist_to_point = std::sqrt( min_dist2 );
    }

    return p;
}

}

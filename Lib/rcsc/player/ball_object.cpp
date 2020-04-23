// -*-c++-*-

/*!
  \file ball_object.cpp
  \brief ball object class Source File
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

#include "ball_object.h"

#include "action_effector.h"
#include "self_object.h"
#include "player_command.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/game_mode.h>

#include <iostream>

#define DEBUG_PRINT

namespace rcsc {

int BallObject::S_pos_count_thr = 10;
int BallObject::S_rpos_count_thr = 5;
int BallObject::S_vel_count_thr = 10;

const std::size_t BallObject::MAX_RECORD = 30;

/*-------------------------------------------------------------------*/
/*!

*/
BallObject::State::State()
  : pos_( 0.0, 0.0 )
  , pos_error_( 0.0, 0.0 )
  , pos_count_( 1000 )
  , rpos_( Vector2D::INVALIDATED )
  , rpos_error_( 0.0, 0.0 )
  , rpos_count_( 1000 )
  , seen_pos_( 0.0, 0.0 )
  , seen_rpos_( Vector2D::INVALIDATED )
  , seen_pos_count_( 1000 )
  , heard_pos_( 0.0, 0.0 )
  , heard_pos_count_( 1000 )
  , vel_( 0.0, 0.0 )
  , vel_error_( 0.0, 0.0 )
  , vel_count_( 1000 )
  , seen_vel_( 0.0, 0.0 )
  , seen_vel_count_( 1000 )
  , heard_vel_( 0.0, 0.0 )
  , heard_vel_count_( 1000 )
  , lost_count_( 0 )
  , ghost_count_( 0 )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
BallObject::BallObject()
    : M_state()
    , M_state_record( MAX_RECORD, State() )
    , M_dist_from_self( 1000.0 )
    , M_angle_from_self( 0.0 )
    , M_rpos_prev( Vector2D::INVALIDATED )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::set_count_thr( const int pos_thr,
                           const int rpos_thr,
                           const int vel_thr )
{
    S_pos_count_thr = pos_thr;
    S_rpos_count_thr = rpos_thr;
    S_vel_count_thr = vel_thr;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::setGhost( const GameTime & )
{
    if ( M_state.ghost_count_ > 0 )
    {
        M_state.pos_count_ = 1000;
        M_state.rpos_count_ = 1000;
        M_state.lost_count_ = 0;
        M_state.ghost_count_ += 1;

        M_dist_from_self = 1000.0;
    }
    else
    {
        M_state.ghost_count_ = 1;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::update( const ActionEffector & act,
                    const GameMode & game_mode,
                    const GameTime & )
{
    Vector2D new_vel( 0.0, 0.0 );

    ////////////////////////////////////////////////////////////////////////
    // vel
    if ( velValid() )
    {
        Vector2D accel( 0.0, 0.0 );
        Vector2D accel_err( 0.0, 0.0 );
        double tmp = 0.0;

        new_vel = vel();

        /////////////////////////////////////////////////////////////
        // kicked in last cycle
        // get info from stored action param
        if ( act.lastBodyCommandType() == PlayerCommand::KICK )
        {
            act.getKickInfo( &accel, &accel_err );

            // check max accel
            tmp = accel.r();
            if ( tmp > ServerParam::i().ballAccelMax() )
            {
                accel *= ( ServerParam::i().ballAccelMax() / tmp );
            }

            new_vel += accel;

#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (update) queued_kick_accel=(%.3f, %.3f) new_vel=(%.3f %.3f)",
                          accel.x, accel.y,
                          new_vel.x, new_vel.y );
#endif
        }

        // check max vel
        tmp = new_vel.r();
        if ( tmp > ServerParam::i().ballSpeedMax() )
        {
            new_vel *= ( ServerParam::i().ballSpeedMax() / tmp );
            tmp = ServerParam::i().ballSpeedMax();
        }

        // add move noise.
        // ball speed max is not considerd, therefore value of tmp is not changed.
        M_state.vel_error_.add( tmp * ServerParam::i().ballRand(),
                                tmp * ServerParam::i().ballRand() );
        // add kick noise
        M_state.vel_error_ += accel_err;
    }

    ////////////////////////////////////////////////////////////////////////
    // wind effect
    updateWindEffect();

    ////////////////////////////////////////////////////////////////////////

    const GameMode::Type pmode = game_mode.type();

    if ( pmode == GameMode::PlayOn
         || pmode == GameMode::GoalKick_
         || pmode == GameMode::GoalieCatch_
         || pmode == GameMode::PenaltyTaken_ )
    {
        // ball position may change.
        M_state.pos_count_ = std::min( 1000, M_state.pos_count_ + 1 );
    }
    else
    {
        // if setplay playmode, ball does not move until playmode change to playon.

        if ( pmode == GameMode::BeforeKickOff
             || pmode == GameMode::KickOff_ )
        {
            M_state.pos_.assign( 0.0, 0.0 );
            M_state.pos_count_ = 0;
            M_state.seen_pos_.assign( 0.0, 0.0 );

#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (update) before_kick_off. set to center." );
#endif
        }
        // if I didin't see the ball in this setplay playmode,
        // we must check the ball first.
        else if ( posCount() > 1
                  || ( rposCount() > 0
                       && distFromSelf() < ServerParam::i().visibleDistance() )
                  )
        {
            // NOT seen at last cycle, but internal info means ball visible.
            // !!! IMPORTANT to check the ghost
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (update) SetPlay. but not seen. posCount=%d"
                          " rposCount=%d. distFromSelf=%.3f",
                          posCount(), rposCount(), distFromSelf() );
#endif
            M_state.pos_count_ = 1000;
        }
        else
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (update) SetPlay. seen once. posCount=%d"
                          " rposCount=%d distFromSelf=%.3f",
                          posCount(), rposCount(), distFromSelf() );
#endif
            M_state.pos_count_ = 1;
        }

        // in SetPlay mode, ball velocity must be Zero.
        new_vel.assign( 0.0, 0.0 );

        M_state.vel_error_.assign( 0.0, 0.0 );
        M_state.vel_count_ = 0;
        M_state.seen_vel_.assign( 0.0, 0.0 );
        M_state.seen_vel_count_ = 0;
    }

    // update position with velocity
    if ( posValid() )
    {
        M_state.pos_ += new_vel;
        M_state.pos_error_ += M_state.vel_error_;
    }

    // vel decay
    M_state.vel_ = new_vel;
    M_state.vel_ *= ServerParam::i().ballDecay();
    M_state.vel_error_ *= ServerParam::i().ballDecay();

    // update accuracy counter
    M_state.rpos_count_ = std::min( 1000, M_state.rpos_count_ + 1 );
    M_state.seen_pos_count_ = std::min( 1000, M_state.seen_pos_count_ + 1 );
    M_state.heard_pos_count_ = std::min( 1000, M_state.heard_pos_count_ + 1 );
    M_state.vel_count_ = std::min( 1000, M_state.vel_count_ + 1 );
    M_state.seen_vel_count_ = std::min( 1000, M_state.seen_vel_count_ + 1 );
    M_state.heard_vel_count_ = std::min( 1000, M_state.heard_vel_count_ + 1 );
    M_state.lost_count_ = std::min( 1000, M_state.lost_count_ + 1 );

    // M_state.ghost_count_ = 0;

    // set previous rpos
    M_rpos_prev = rpos();

    // M_rpos is updated using visual info or self info
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateByFullstate( const Vector2D & pos,
                               const Vector2D & vel,
                               const Vector2D & self_pos )
{
    M_state.pos_ = pos;
    M_state.pos_error_.assign( 0.0, 0.0 );
    M_state.pos_count_ = 0;

    M_state.rpos_ = pos - self_pos;
    M_state.rpos_error_.assign( 0.0, 0.0 );
    M_state.rpos_count_ = 0;

    M_state.seen_pos_ = pos;
    M_state.seen_rpos_ = M_state.rpos_;
    M_state.seen_pos_count_ = 0;

    M_state.vel_ = vel;
    M_state.vel_error_.assign( 0.0, 0.0 );
    M_state.vel_count_ = 0;

    M_state.seen_vel_ = vel;
    M_state.seen_vel_count_ = 0;

    M_state.lost_count_ = 0;

    M_state.ghost_count_ = 0;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateWindEffect()
{
    // ball_speed_max is not considerd in rcssserver
    // wind effect
#if 0
    if ( ! ServerParam::i().windNone() ) // use wind
    {
        if ( ! ServerParam::i().useWindRandom() ) // but static initialization
        {
            Vector2D wind_vector( 1,
                                  ServerParam::i().windForce(),
                                  ServerParam::i().windDir() );
            double speed = M_vel.r();

            Vector2D wind_effect( speed * wind_vector.x / (weight * WIND_WEIGHT),
                                  speed * wind_vector.y / (weight * WIND_WEIGHT) );
            M_state.vel_ += wind_effect;

            Vector2D wind_error( speed * wind_vector.x * ServerParam::i().windRand()
                                 / (ServerParam::i().playerWeight() * WIND_WEIGHT),
                                 speed * wind_vector.y * ServerParam::i().windRand()
                                 / (ServerParam::i().playerWeight() * WIND_WEIGHT) );
            M_state.vel_error_.add( wind_error, wind_error );
        }
        else
        {
            // it is necessary to estimate wind force & dir

        }
    }
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateByCollision( const Vector2D & pos,
                               const int pos_count,
                               const Vector2D & rpos,
                               const int rpos_count,
                               const Vector2D & vel,
                               const int vel_count )
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateByCollision)"
                  " pos(%.2f %.2f)count=%d rpos=(%2.f %.2f)count=%d vel=(%.2f %.2f)count=%d",
                  pos.x, pos.y, pos_count,
                  rpos.x, rpos.y, rpos_count,
                  vel.x, vel.y, vel_count );
#endif
    M_state.pos_ = pos;
    M_state.pos_count_ = pos_count;
    M_state.rpos_ = rpos;
    M_state.rpos_count_ = rpos_count;
    M_state.vel_ = vel;
    M_state.vel_count_ = vel_count;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateOnlyRelativePos( const Vector2D & rpos,
                                   const Vector2D & rpos_err )
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateOnlyRelativePos)"
                  " rpos=(%2.f %.2f) error=(%f %f)",
                  rpos.x, rpos.y,
                  rpos_err.x, rpos_err.y );
#endif
    M_state.rpos_ = rpos;
    M_state.rpos_error_ = rpos_err;
    M_state.rpos_count_ = 0;

    M_state.seen_rpos_ = rpos;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateOnlyVel( const Vector2D & vel,
                           const Vector2D & vel_err,
                           const int vel_count )
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateOnlyVel)"
                  " vel=(%2.f %.2f) error=(%f %f) count=%d",
                  vel.x, vel.y,
                  vel_err.x, vel_err.y,
                  vel_count );
#endif
    M_state.vel_ = vel;
    M_state.vel_error_ = vel_err;
    M_state.vel_count_ = vel_count;

    M_state.seen_vel_ = vel;
    M_state.seen_vel_count_ = vel_count;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::setOpponentControlEffect()
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (setOpponentControlEffect)"
                  " velocity is set to Zero." );
#endif
    M_state.vel_error_ += vel();
    M_state.vel_count_ += 1;

    M_state.vel_.assign( 0.0, 0.0 );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updatePos( const Vector2D & pos,
                       const Vector2D & pos_err,
                       const int pos_count,
                       const Vector2D & rpos,
                       const Vector2D & rpos_err )
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updatePos)"
                  " pos(%.2f %.2f) count=%d",
                  pos.x, pos.y, pos_count );
#endif

    M_state.pos_ = pos;
    M_state.pos_error_ = pos_err;
    M_state.pos_count_ = pos_count;
    M_state.seen_pos_ = pos;
    M_state.seen_pos_count_ = 0;

    updateOnlyRelativePos( rpos, rpos_err );

    M_state.lost_count_ = 0;
    M_state.ghost_count_ = 0;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateAll( const Vector2D & pos,
                       const Vector2D & pos_err,
                       const int pos_count,
                       const Vector2D & rpos,
                       const Vector2D & rpos_err,
                       const Vector2D & vel,
                       const Vector2D & vel_err,
                       const int vel_count )
{
#ifdef DEBUG_PRINT
    dlog.addText( Logger::WORLD,
                  __FILE__" (updateAll)" );
#endif
    updatePos( pos, pos_err, pos_count, rpos, rpos_err );
    updateOnlyVel( vel, vel_err, vel_count );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateByHear( const ActionEffector & act,
                          const double & sender_to_ball_dist,
                          const Vector2D & heard_pos,
                          const Vector2D & heard_vel )
{
    double heard_speed = 0.0;
    if ( heard_vel.isValid() )
    {
        heard_speed = heard_vel.r();
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear)"
                      " heard_pos=(%.2f, %.2f)"
                      " heard_vel=(%.2f, %.2f)",
                      heard_pos.x, heard_pos.y,
                      heard_vel.x, heard_vel.y );
#endif
    }
#ifdef DEBUG_PRINT
    else
    {
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear)"
                      " heard_pos=(%.2f, %.2f) no vel",
                      heard_pos.x, heard_pos.y );
    }
#endif

    M_state.heard_pos_ = heard_pos;
    M_state.heard_pos_count_ = 0;
    M_state.heard_vel_ = heard_vel;
    M_state.heard_vel_count_ = 0;

    if ( act.lastBodyCommandType() == PlayerCommand::KICK )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear) last command is kick." );
#endif
        return;
    }

    if ( M_state.ghost_count_ > 0 )
    {
        if ( heard_pos.dist( pos() ) < 3.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateByHear) ghost detected." );
#endif
            M_state.pos_ = heard_pos;
            M_state.pos_count_ = 1;
            if ( heard_vel.isValid()
                 && heard_speed > 0.001 )
            {
                M_state.vel_ = heard_vel;
                M_state.vel_count_ = 1;
            }
            return;
        }
    }

    if ( posCount() >= 1
         && ( heard_pos.dist2( pos() ) > std::pow( 2.0, 2 )
              || std::fabs( heard_vel.x - vel().x ) > 1.0
              || std::fabs( heard_vel.y - vel().y ) > 1.0 )
         )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear) big difference from last internal state." );
#endif
        M_state.pos_ = heard_pos;
        M_state.pos_count_ = 1;
        if ( heard_vel.isValid()
             && heard_speed > 0.001 )
        {
            M_state.vel_ = heard_vel;
            M_state.vel_count_ = 1;
        }
        return;
    }

    const double prev_dist = ( M_rpos_prev.isValid()
                               ? M_rpos_prev.r()
                               : 1000.0 );

    if ( sender_to_ball_dist < 5.0
         && posCount() >= 1 )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear) sent from the player near to the ball." );
#endif
        M_state.pos_ = heard_pos;
        M_state.pos_count_ = 1;
        if ( heard_vel.isValid()
             && heard_speed > 0.001 )
        {
            M_state.vel_ = heard_vel;
            M_state.vel_count_ = 1;
        }
    }
    else if ( sender_to_ball_dist < 20.0
              && posCount() >= 2 )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear) low accuracy." );
#endif
        M_state.pos_ = heard_pos;
        M_state.pos_count_ = 1;
        if ( heard_vel.isValid()
             && heard_speed > 0.001 )
        {
            M_state.vel_ = heard_vel;
            M_state.vel_count_ = 1;
        }
    }
    else if ( heard_vel.isValid()
              && heard_speed > 0.001
              && ( velCount() >= 4
                   || ( velCount() >= 3
                        && prev_dist > 1.4
                        && sender_to_ball_dist < 10.0 )
                   || ( velCount() >= 2
                        && prev_dist > 1.4
                        && sender_to_ball_dist < 10.0 )
                   )
              )
    {
#ifdef DEBUG_PRINT
        dlog.addText( Logger::WORLD,
                      __FILE__" (updateByHear) low vel accuracy."
                      " velCount=%d"
                      " sender_ball_dist=%.3f prev_my_dist=%.3f",
                      velCount(),
                      sender_to_ball_dist, prev_dist );
#endif
        M_state.pos_ = heard_pos;
        M_state.pos_count_ = 1;
        if ( heard_vel.isValid()
             && heard_speed > 0.001 )
        {
            M_state.vel_ = heard_vel;
            M_state.vel_count_ = 1;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateSelfRelated( const SelfObject & self )
{
    // seen
    if ( rposCount() == 0 )
    {
        // M_rpos is already updated
        M_dist_from_self = rpos().r();
        M_angle_from_self = rpos().th();
    }
    // not seen
    else
    {
        // update rpos
        if ( M_rpos_prev.isValid()
             && self.lastMove().isValid() )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateSelfRelated) update rpos using self move(%.2f %.2f)",
                          self.lastMove().x, self.lastMove().y );
#endif
            // M_rpos_prev is updated in update()
            M_state.rpos_
                = M_rpos_prev
                + ( vel() / ServerParam::i().ballDecay() )
                - self.lastMove();
            M_state.rpos_error_ += velError();
            M_state.rpos_error_ += ( self.velError() / self.playerType().playerDecay() );
        }
        // it is not necessary to consider other case.

        // update dist & angle

        // at least, rpos is valid
        if ( rpos().isValid()
             && posCount() > rposCount() )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateSelfRelated) set pos by rpos(%.2f %.2f)",
                          rpos().x, rpos().y );
#endif
            M_state.pos_ = self.pos() + this->rpos();
            M_state.pos_error_ = self.posError() + this->rposError();
            M_dist_from_self = rpos().r();
            M_angle_from_self = rpos().th();
        }
        else if ( posValid() )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateSelfRelated) set rpos by pos" );
#endif
            M_state.rpos_ = pos() - self.pos();
            M_state.rpos_error_ = posError() + self.posError();
            M_dist_from_self = rpos().r();
            M_angle_from_self = rpos().th();
        }
        else
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::WORLD,
                          __FILE__" (updateSelfRelated) failed" );
#endif
            M_dist_from_self = 1000.0;
            M_angle_from_self = 0.0;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallObject::updateRecord()
{
    M_state_record.pop_back();
    M_state_record.push_front( M_state );
}

/*-------------------------------------------------------------------*/
/*!

*/
const
BallObject::State *
BallObject::getState( const size_t history_index ) const
{
    if ( history_index >= M_state_record.size() )
    {
        return static_cast< State * >( 0 );
    }

    return &M_state_record[ history_index ];
}

/*-------------------------------------------------------------------*/
/*!

*/
Vector2D
BallObject::inertiaTravel( const int cycle ) const
{
    return inertia_n_step_travel( vel(),
                                  cycle,
                                  ServerParam::i().ballDecay() );
}

/*-------------------------------------------------------------------*/
/*!

*/
Vector2D
BallObject::inertiaPoint( const int cycle ) const
{
    return inertia_n_step_point( pos(),
                                 vel(),
                                 cycle,
                                 ServerParam::i().ballDecay() );
}

/*-------------------------------------------------------------------*/
/*!

*/
Vector2D
BallObject::inertiaFinalPoint() const
{
    return inertia_final_point( pos(),
                                vel(),
                                ServerParam::i().ballDecay() );
}

/*-------------------------------------------------------------------*/
/*!

*/
double
BallObject::calc_travel_step( const double & distance,
                              const double & first_speed )
{
    return calc_length_geom_series( first_speed,
                                    distance,
                                    ServerParam::i().ballDecay() );
}

}

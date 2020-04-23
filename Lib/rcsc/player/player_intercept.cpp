// -*-c++-*-

/*!
  \file player_intercept.cpp
  \brief intercept predictor for other players Source File
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

#include "player_intercept.h"
#include "world_model.h"
#include "ball_object.h"
#include "player_object.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/soccer_math.h>

// #define DEBUG
// #define DEBUG2

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
int
PlayerIntercept::predict( const PlayerObject & player,
                          const PlayerType & player_type,
                          const int max_cycle ) const
{
    const double penalty_x_abs = ServerParam::i().pitchHalfLength() - ServerParam::i().penaltyAreaLength();
    const double penalty_y_abs = ServerParam::i().penaltyAreaHalfWidth();

    const int pos_count = std::min( player.seenPosCount(), player.posCount() );
    const Vector2D & player_pos = ( player.seenPosCount() <= player.posCount()
                                    ? player.seenPos()
                                    : player.pos() );
    int min_cycle = 0;
    {
        Vector2D ball_to_player = player_pos - M_world.ball().pos();
        ball_to_player.rotate( - M_world.ball().vel().th() );
        min_cycle = static_cast< int >( std::floor( ball_to_player.absY()
                                                    / player_type.realSpeedMax() ) );
    }

    if ( player.isTackling() )
    {
        min_cycle += std::max( 0,
                               ServerParam::i().tackleCycles() - player.tackleCount() - 2 );
    }

    min_cycle = std::max( 0,
                          min_cycle - std::min( player.seenPosCount(), player.posCount() ) );

#ifdef DEBUG
    dlog.addText( Logger::INTERCEPT,
                  "Intercept Player %d %d (%.1f %.1f)---- min_cycle=%d max_cycle=%d",
                  player.side(),
                  player.unum(),
                  player.pos().x, player.pos().y,
                  min_cycle, max_cycle );
#endif
    if ( min_cycle > max_cycle )
    {
        return predictFinal( player, player_type );
    }

    const std::size_t MAX_LOOP = std::min( static_cast< std::size_t >( max_cycle ),
                                           M_ball_pos_cache.size() );

    for ( std::size_t cycle = static_cast< std::size_t >( min_cycle );
          cycle < MAX_LOOP;
          ++cycle )
    {
        const Vector2D & ball_pos = M_ball_pos_cache.at( cycle );
#ifdef DEBUG2
        dlog.addText( Logger::INTERCEPT,
                      "*** cycle=%d  ball(%.2f %.2f)",
                      cycle, ball_pos.x, ball_pos.y );
#endif
        const double control_area = ( ( player.goalie()
                                        && ball_pos.absX() > penalty_x_abs
                                        && ball_pos.absY() < penalty_y_abs )
                                      ? ServerParam::i().catchableArea()
                                      : player_type.kickableArea() );

        if ( control_area + player_type.realSpeedMax() * ( cycle + pos_count ) + 0.5
             < player_pos.dist( ball_pos ) )
        {
            // never reach
#ifdef DEBUG2
            dlog.addText( Logger::INTERCEPT,
                          "--->cycle=%d  never reach! ball(%.2f %.2f)",
                          cycle, ball_pos.x, ball_pos.y );
#endif
            continue;
        }

        if ( canReachAfterTurnDash( cycle,
                                    player, player_type,
                                    control_area,
                                    ball_pos ) )
        {
#ifdef DEBUG
            dlog.addText( Logger::INTERCEPT,
                          "--->cycle=%d  Sucess! ball(%.2f %.2f)",
                          cycle, ball_pos.x, ball_pos.y );
#endif
            return cycle;
        }
    }

    return predictFinal( player, player_type );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
PlayerIntercept::canReachAfterTurnDash( const int cycle,
                                        const PlayerObject & player,
                                        const PlayerType & player_type,
                                        const double & control_area,
                                        const Vector2D & ball_pos ) const
{
    int n_turn = predictTurnCycle( cycle,
                                   player,
                                   player_type,
                                   control_area,
                                   ball_pos );
#ifdef DEBUG2
    dlog.addText( Logger::INTERCEPT,
                  "______ loop %d  turn step = %d",
                  cycle, n_turn );
#endif

    int n_dash = cycle - n_turn;
    if ( n_dash < 0 )
    {
        return false;
    }

    return canReachAfterDash( n_turn,
                              n_dash,
                              player,
                              player_type,
                              control_area,
                              ball_pos );
}

/*-------------------------------------------------------------------*/
/*!

*/
int
PlayerIntercept::predictTurnCycle( const int cycle,
                                   const PlayerObject & player,
                                   const PlayerType & player_type,
                                   const double & control_area,
                                   const Vector2D & ball_pos ) const
{
//     if ( player.bodyCount() > std::min( player.seenPosCount(), player.posCount() ) )
//     {
//         return 0;
//     }

    const Vector2D & ppos = ( player.seenPosCount() <= player.posCount()
                              ? player.seenPos()
                              : player.pos() );
    const Vector2D & pvel = ( player.seenVelCount() <= player.velCount()
                              ? player.seenVel()
                              : player.vel() );

    Vector2D inertia_pos = player_type.inertiaPoint( ppos,
                                                     pvel,
                                                     cycle );
    Vector2D target_rel = ball_pos - inertia_pos;
    double target_dist = target_rel.r();
    double turn_margin = 180.0;
    if ( control_area < target_dist )
    {
        turn_margin = AngleDeg::asin_deg( control_area / target_dist );
    }
    turn_margin = std::max( turn_margin, 12.0 );

    double angle_diff = ( target_rel.th() - player.body() ).abs();

    if ( target_dist < 5.0 // XXX magic number XXX
         && angle_diff > 90.0 )
    {
        // assume back dash
        angle_diff = 180.0 - angle_diff;
    }

    int n_turn = 0;

    double speed = player.vel().r();
    if ( angle_diff > turn_margin )
    {
        double max_turn = player_type.effectiveTurn( ServerParam::i().maxMoment(),
                                                     speed );
        angle_diff -= max_turn;
        speed *= player_type.playerDecay();
        ++n_turn;
    }

    //return bound( 0, n_turn - player.bodyCount(), 5 );
    return n_turn;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
PlayerIntercept::canReachAfterDash( const int n_turn,
                                    const int max_dash,
                                    const PlayerObject & player,
                                    const PlayerType & player_type,
                                    const double & control_area,
                                    const Vector2D & ball_pos ) const
{
    const int pos_count = std::min( player.seenPosCount(), player.posCount() );
    const Vector2D & ppos = ( player.seenPosCount() <= player.posCount()
                              ? player.seenPos()
                              : player.pos() );
    const Vector2D & pvel = ( player.seenVelCount() <= player.velCount()
                              ? player.seenVel()
                              : player.vel() );

    Vector2D player_pos = inertia_n_step_point( ppos, pvel,
                                                n_turn + max_dash,
                                                player_type.playerDecay() );

    Vector2D player_to_ball = ball_pos - player_pos;
    double player_to_ball_dist = player_to_ball.r();
    player_to_ball_dist -= control_area;

    if ( player_to_ball_dist < 0.0 )
    {
#ifdef DEBUG
        dlog.addText( Logger::INTERCEPT,
                      "______ %d (%.1f %.1f) can reach(1). turn=%d dash=0",
                      player.unum(),
                      player.pos().x, player.pos().y,
                      n_turn );
#endif
        return true;
    }

    int estimate_dash = player_type.cyclesToReachDistance( player_to_ball_dist );
    int n_dash = estimate_dash;
    if ( player.side() != M_world.ourSide() )
    {
        n_dash -= bound( 0, pos_count - n_turn, std::min( 6, M_world.ball().seenPosCount() + 1 ) );
    }
    else
    {
        //n_dash -= bound( 0, pos_count - n_turn, 1 );
        n_dash -= bound( 0, pos_count - n_turn, std::min( 1, M_world.ball().seenPosCount() ) );
    }

    if ( player.isTackling() )
    {
        n_dash += std::max( 0, ServerParam::i().tackleCycles() - player.tackleCount() - 2 );
    }

    if ( n_dash <= max_dash )
    {
#ifdef DEBUG
        dlog.addText( Logger::INTERCEPT,
                      "______ %d (%.1f %.1f) can reach(2). turn=%d dash=%d(%d) dist=%.3f",
                      player.unum(),
                      player.pos().x, player.pos().y,
                      n_turn, n_dash, estimate_dash,
                      player_to_ball_dist );
#endif
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
int
PlayerIntercept::predictFinal( const PlayerObject & player,
                               const PlayerType & player_type ) const
{
    const double penalty_x_abs = ServerParam::i().pitchHalfLength() - ServerParam::i().penaltyAreaLength();
    const double penalty_y_abs = ServerParam::i().penaltyAreaHalfWidth();

    const int pos_count = std::min( player.seenPosCount(), player.posCount() );
    const Vector2D & ppos = ( player.seenPosCount() <= player.posCount()
                              ? player.seenPos()
                              : player.pos() );
    const Vector2D & pvel = ( player.seenVelCount() <= player.velCount()
                              ? player.seenVel()
                              : player.vel() );

    const Vector2D & ball_pos = M_ball_pos_cache.back();
    const int ball_step = static_cast< int >( M_ball_pos_cache.size() );

    const double control_area = ( ( player.goalie()
                                    && ball_pos.absX() > penalty_x_abs
                                    && ball_pos.absY() < penalty_y_abs )
                                  ? ServerParam::i().catchableArea()
                                  : player_type.kickableArea() );

    int n_turn = predictTurnCycle( 100,
                                   player,
                                   player_type,
                                   control_area,
                                   ball_pos );

    Vector2D inertia_pos = player_type.inertiaPoint( ppos, pvel,
                                                     100 );
    double dash_dist = inertia_pos.dist( ball_pos );
    dash_dist -= control_area;

    if ( player.side() != M_world.ourSide() )
    {
        dash_dist -= player.distFromSelf() * 0.03;
    }

    if ( dash_dist < 0.0 )
    {
        return ball_step;
    }

    int n_dash = player_type.cyclesToReachDistance( dash_dist );

    if ( player.side() != M_world.ourSide() )
    {
        n_dash -= bound( 0, pos_count - n_turn, 10 );
    }
    else
    {
        n_dash -= bound( 0, pos_count - n_turn, 1 );
    }

    n_dash = std::max( 1, n_dash );

#ifdef DEBUG
    dlog.addText( Logger::INTERCEPT,
                  "____No Solution. final point(%.2f %.2f)"
                  " ball_step=%d n_turn=%d n_dash=%d dash_dist=%.2f",
                  ball_pos.x, ball_pos.y,
                  ball_step,
                  n_turn,
                  n_dash,
                  dash_dist );
#endif
    return std::max( ball_step, n_turn + n_dash );
}

}

// -*-c++-*-

/*!
  \file intercept_table.cpp
  \brief interception info holder Source File
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

#include "intercept_table.h"
//#include "self_intercept.h"
#include "self_intercept_v13.h"
#include "player_intercept.h"
#include "world_model.h"
#include "player_object.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/game_time.h>

#include <algorithm>

// #define DEBUG_PRINT

namespace rcsc {

const std::size_t InterceptTable::MAX_CYCLE = 30;

/*-------------------------------------------------------------------*/
/*!

*/
InterceptTable::InterceptTable( const WorldModel & world )
    : M_world( world )
    , M_update_time( 0, 0 )
{
    M_ball_pos_cache.reserve( MAX_CYCLE + 2 );
    //M_ball_vel_cache.reserve( MAX_CYCLE + 2 );

    M_self_cache.reserve( ( MAX_CYCLE + 2 ) * 2 );

    clear();
}

/*-------------------------------------------------------------------*/
/*!

*/
InterceptTable::~InterceptTable()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::clear()
{
    M_ball_pos_cache.clear();

    M_self_reach_cycle = 1000;
    M_self_exhaust_reach_cycle = 1000;
    M_teammate_reach_cycle = 1000;
    M_second_teammate_reach_cycle = 1000;
    M_goalie_reach_cycle = 1000;
    M_opponent_reach_cycle = 1000;
    M_second_opponent_reach_cycle = 1000;

    M_fastest_teammate = static_cast< PlayerObject * >( 0 );
    M_second_teammate = static_cast< PlayerObject * >( 0 );
    M_fastest_opponent = static_cast< PlayerObject * >( 0 );
    M_second_opponent = static_cast< PlayerObject * >( 0 );

    M_self_cache.clear();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::update()
{
    if ( M_world.time() == M_update_time )
    {
        return;
    }
    M_update_time = M_world.time();

    dlog.addText( Logger::INTERCEPT,
                  __FILE__" (update)" );

    // clear all data
    this->clear();

    // playmode check
    if ( M_world.gameMode().type() == GameMode::TimeOver
         || M_world.gameMode().type() == GameMode::BeforeKickOff )
    {
        return;
    }

    if ( ! M_world.self().posValid()
         || ! M_world.ball().posValid() )
    {
        dlog.addText( Logger::INTERCEPT,
                      __FILE__" (update) Invalid self or ball pos" );
        return;
    }

#ifdef DEBUG
    if ( M_world.self().isKickable()
         || M_world.existKickableTeammate()
         || M_world.existKickableOpponent() )
    {
        dlog.addText( Logger::INTERCEPT,
                      __FILE__" (update) Already exist kickable player" );
    }
#endif

    createBallCache();

#ifdef DEBUG
    dlog.addText( Logger::INTERCEPT,
                  "==========Intercept Predict Self==========" );
#endif

    predictSelf();

#ifdef DEBUG
    dlog.addText( Logger::INTERCEPT,
                  "==========Intercept Predict Opponent==========" );
#endif

    predictOpponent();

#ifdef DEBUG
    dlog.addText( Logger::INTERCEPT,
                  "==========Intercept Predict Teammate==========" );
#endif

    predictTeammate();

    dlog.addText( Logger::INTERCEPT,
                  "<-----Intercept Self reach cycle = %d. exhaust reach step = %d ",
                  M_self_reach_cycle,
                  M_self_exhaust_reach_cycle );
    if ( M_fastest_teammate )
    {
        dlog.addText( Logger::INTERCEPT,
                      "<-----Intercept Teammate  fastest reach step = %d."
                      " teammate %d (%.1f %.1f)",
                      M_teammate_reach_cycle,
                      M_fastest_teammate->unum(),
                      M_fastest_teammate->pos().x,
                      M_fastest_teammate->pos().y );

    }

    if ( M_second_teammate )
    {
        dlog.addText( Logger::INTERCEPT,
                      "<-----Intercept Teammate  2nd     reach step = %d."
                      " teammate %d (%.1f %.1f)",
                      M_second_teammate_reach_cycle,
                      M_second_teammate->unum(),
                      M_second_teammate->pos().x,
                      M_second_teammate->pos().y );
    }

    if ( M_fastest_opponent )
    {
        dlog.addText( Logger::INTERCEPT,
                      "<-----Intercept Opponent  fastest reach step = %d."
                      " opponent %d (%.1f %.1f)",
                      M_opponent_reach_cycle,
                      M_fastest_opponent->unum(),
                      M_fastest_opponent->pos().x,
                      M_fastest_opponent->pos().y );
    }

    if ( M_second_opponent )
    {
        dlog.addText( Logger::INTERCEPT,
                      "<-----Intercept Opponent  2nd     reach step = %d."
                      " opponent %d (%.1f %.1f)",
                      M_second_opponent_reach_cycle,
                      M_second_opponent->unum(),
                      M_second_opponent->pos().x,
                      M_second_opponent->pos().y );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::hearTeammate( const int unum,
                              const int cycle )
{
    if ( M_fastest_teammate
         && cycle >= M_teammate_reach_cycle )
    {
        return;
    }

    const PlayerObject * p = static_cast< PlayerObject * >( 0 );

    const PlayerPtrCont::const_iterator end = M_world.teammatesFromSelf().end();
    for ( PlayerPtrCont::const_iterator it = M_world.teammatesFromSelf().begin();
          it != end;
          ++it )
    {
        if ( (*it)->unum() == unum )
        {
            p = *it;
            break;
        }
    }

    if ( p )
    {
        M_fastest_teammate = p;
        M_teammate_reach_cycle = cycle;

        dlog.addText( Logger::INTERCEPT,
                      "<----- Hear Intercept Teammate  fastest reach step = %d."
                      " teammate %d (%.1f %.1f)",
                      M_teammate_reach_cycle,
                      M_fastest_teammate->unum(),
                      M_fastest_teammate->pos().x,
                      M_fastest_teammate->pos().y );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::hearOpponent( const int unum,
                              const int cycle )
{
    if ( M_fastest_opponent )
    {
        if ( cycle >= M_opponent_reach_cycle )
        {
            dlog.addText( Logger::INTERCEPT,
                          "<----- Hear Intercept Opponent. no update."
                          " exist faster reach cycle %d >= %d",
                          cycle, M_opponent_reach_cycle );
            return;
        }

        if ( M_fastest_opponent->unum() == unum
             && M_fastest_opponent->posCount() == 0 )
        {
            dlog.addText( Logger::INTERCEPT,
                          "<----- Hear Intercept Opponent . no update."
                          " opponent %d (%.1f %.1f) is seen",
                          M_fastest_opponent->unum(),
                          M_fastest_opponent->pos().x,
                          M_fastest_opponent->pos().y );
            return;
        }
    }

    const PlayerObject * p = static_cast< PlayerObject * >( 0 );

    const PlayerPtrCont::const_iterator end = M_world.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator it = M_world.opponentsFromSelf().begin();
          it != end;
          ++it )
    {
        if ( (*it)->unum() == unum )
        {
            p = *it;
            break;
        }
    }

    if ( p )
    {
        M_fastest_opponent = p;
        M_opponent_reach_cycle = cycle;

        dlog.addText( Logger::INTERCEPT,
                      "<----- Hear Intercept Opponent  fastest reach step = %d."
                      " opponent %d (%.1f %.1f)",
                      M_opponent_reach_cycle,
                      M_fastest_opponent->unum(),
                      M_fastest_opponent->pos().x,
                      M_fastest_opponent->pos().y );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::createBallCache()
{
    const ServerParam & SP = ServerParam::i();
    const double pitch_x_max = ( SP.keepawayMode()
                                 ? SP.keepawayLength() * 0.5
                                 : SP.pitchHalfLength() + 5.0 );
    const double pitch_y_max = ( SP.keepawayMode()
                                 ? SP.keepawayWidth() * 0.5
                                 : SP.pitchHalfWidth() + 5.0 );
    const double bdecay = SP.ballDecay();

    Vector2D bpos = M_world.ball().pos();
    Vector2D bvel = M_world.ball().vel();

    M_ball_pos_cache.push_back( bpos );

    if ( M_world.self().isKickable() )
    {
        return;
    }

    for ( std::size_t i = 1; i <= MAX_CYCLE; ++i )
    {
        bpos += bvel;
        bvel *= bdecay;

        M_ball_pos_cache.push_back( bpos );

        if ( i >= 5
             && bvel.r2() < 0.01*0.01 )
        {
            // ball stopped
            break;
        }

        if ( bpos.absX() > pitch_x_max
             || bpos.absY() > pitch_y_max )
        {
            // out of pitch
            break;
        }
    }

    if ( M_ball_pos_cache.size() == 1 )
    {
        M_ball_pos_cache.push_back( bpos );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::predictSelf()
{
    if ( M_world.self().isKickable() )
    {
        dlog.addText( Logger::INTERCEPT,
                      "Intercept Self. already kickable. no estimation loop!" );
        M_self_reach_cycle = 0;
        return;
    }

    std::size_t max_cycle = std::min( MAX_CYCLE, M_ball_pos_cache.size() );

    //SelfIntercept predictor( M_world, M_ball_pos_cache );
    SelfInterceptV13 predictor( M_world, M_ball_pos_cache );
    predictor.predict( max_cycle, M_self_cache );

    if ( M_self_cache.empty() )
    {
        std::cerr << M_world.self().unum() << ' '
                  << M_world.time()
                  << " Interecet Self cache is empty!"
                  << std::endl;
        dlog.addText( Logger::INTERCEPT,
                      "Intercept Self. Self cache is empty!" );
        // if self cache is empty,
        // reach point should be the inertia final point of the ball
        return;
    }

// #ifdef SELF_INTERCEPT_USE_NO_SAVE_RECEVERY
//     std::sort( M_self_cache.begin(),
//                M_self_cache.end(),
//                InterceptInfo::Cmp() );
//     M_self_cache.erase( std::unique( M_self_cache.begin(),
//                                      M_self_cache.end(),
//                                      InterceptInfo::Equal() ),
//                         M_self_cache.end() );
// #endif

    int min_cycle = M_self_reach_cycle;
    int exhaust_min_cycle = M_self_exhaust_reach_cycle;

    const std::vector< InterceptInfo >::iterator end = M_self_cache.end();
    for ( std::vector< InterceptInfo >::iterator it = M_self_cache.begin();
          it != end;
          ++it )
    {
        if ( it->mode() == InterceptInfo::NORMAL )
        {
            if ( it->reachCycle() < min_cycle )
            {
                min_cycle = it->reachCycle();
                break;
            }
        }
        else if ( it->mode() == InterceptInfo::EXHAUST )
        {
            if ( it->reachCycle() < exhaust_min_cycle )
            {
                exhaust_min_cycle = it->reachCycle();
                break;
            }
        }
    }

    dlog.addText( Logger::INTERCEPT,
                  "Intercept Self. solution size = %d",
                  M_self_cache.size() );

    M_self_reach_cycle = min_cycle;
    M_self_exhaust_reach_cycle = exhaust_min_cycle;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::predictTeammate()
{
    const PlayerPtrCont & teammates = M_world.teammatesFromBall();
    const PlayerPtrCont::const_iterator t_end = teammates.end();

    if ( M_world.existKickableTeammate() )
    {
        dlog.addText( Logger::INTERCEPT,
                      "Intercept Teammate. exist kickable teammate. no estimation loop!" );
        M_teammate_reach_cycle = 0;

        for ( PlayerPtrCont::const_iterator t = teammates.begin();
              t != t_end;
              ++t )
        {
            if ( (*t)->isGhost()
                   || (*t)->posCount() > M_world.ball().posCount() + 1 )
            {
                continue;
            }

            M_fastest_teammate = *t;
            dlog.addText( Logger::INTERCEPT,
                          "---> set fastest teammate %d (%.1f %.1f)",
                          (*t)->unum(),
                          (*t)->pos().x, (*t)->pos().y );
            break;
        }
        return;
    }

    int min_cycle = 1000;
    int second_min_cycle = 1000;

    PlayerIntercept predictor( M_world, M_ball_pos_cache );

    for ( PlayerPtrCont::const_iterator it = teammates.begin();
          it != t_end;
          ++it )
    {
        if ( (*it)->posCount() >= 10 )
        {
            dlog.addText( Logger::INTERCEPT,
                          "Intercept Teammate %d.(%.1f %.1f) Low accuracy %d. skip...",
                          (*it)->unum(),
                          (*it)->pos().x, (*it)->pos().y,
                          (*it)->posCount() );
            continue;
        }

        const PlayerType * player_type = (*it)->playerTypePtr();
        if ( ! player_type )
        {
            std::cerr << M_world.time()
                      << " " << __FILE__ << ":" << __LINE__
                      << "  Failed to get teammate player type. unum = "
                      << (*it)->unum()
                      << std::endl;
            dlog.addText( Logger::INTERCEPT,
                          "ERROR. Intercept. Failed to get teammate player type.",
                          " unum = %d",
                          (*it)->unum() );
            continue;
        }

        int cycle = predictor.predict( *(*it), *player_type,
                                       second_min_cycle );
        dlog.addText( Logger::INTERCEPT,
                      "---> Teammate %d.(%.1f %.1f) type=%d cycle=%d",
                      (*it)->unum(),
                      (*it)->pos().x, (*it)->pos().y,
                      player_type->id(),
                      cycle );
        if ( (*it)->goalie() )
        {
            M_goalie_reach_cycle = cycle;
        }
        else if ( cycle < second_min_cycle )
        {
            second_min_cycle = cycle;
            M_second_teammate = *it;

            if ( second_min_cycle < min_cycle )
            {
                std::swap( min_cycle, second_min_cycle );
                std::swap( M_fastest_teammate, M_second_teammate );
            }
        }
    }

    if ( M_second_teammate && second_min_cycle < 1000 )
    {
        M_second_teammate_reach_cycle = second_min_cycle;
    }

    if ( M_fastest_teammate && min_cycle < 1000 )
    {
        M_teammate_reach_cycle = min_cycle;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
InterceptTable::predictOpponent()
{
    const PlayerPtrCont & opponents = M_world.opponentsFromBall();
    const PlayerPtrCont::const_iterator o_end = opponents.end();

    if ( M_world.existKickableOpponent() )
    {
        dlog.addText( Logger::INTERCEPT,
                      "Intercept Opponent. exist kickable opponent. no estimation loop!" );
        M_opponent_reach_cycle = 0;

        for ( PlayerPtrCont::const_iterator o = opponents.begin();
              o != o_end;
              ++o )
        {
            if ( (*o)->isGhost()
                   || (*o)->posCount() > M_world.ball().posCount() + 1 )
            {
                continue;
            }

            M_fastest_opponent = *o;
            dlog.addText( Logger::INTERCEPT,
                          "---> set fastest opponent %d (%.1f %.1f)",
                          (*o)->unum(),
                          (*o)->pos().x, (*o)->pos().y );
            break;
        }

        return;
    }

    int min_cycle = 1000;
    int second_min_cycle = 1000;

    PlayerIntercept predictor( M_world, M_ball_pos_cache );

    for ( PlayerPtrCont::const_iterator it = opponents.begin();
          it != o_end;
          ++it )
    {
        if ( (*it)->posCount() >= 15 )
        {
            dlog.addText( Logger::INTERCEPT,
                          "Intercept Opponent %d.(%.1f %.1f) Low accuracy %d. skip...",
                          (*it)->unum(),
                          (*it)->pos().x, (*it)->pos().y,
                          (*it)->posCount() );
            continue;
        }

        const PlayerType * player_type = (*it)->playerTypePtr();
        if ( ! player_type )
        {
            std::cerr << M_world.time()
                      << " " << __FILE__ << ":" << __LINE__
                      << "  Failed to get opponent player type. unum = "
                      << (*it)->unum()
                      << std::endl;
            dlog.addText( Logger::INTERCEPT,
                          "ERROR. Intercept Failed to get opponent player type."
                          " unum = %d",
                          (*it)->unum());
            continue;
        }

        int cycle = predictor.predict( *(*it), *player_type,
                                       second_min_cycle );
        dlog.addText( Logger::INTERCEPT,
                      "---> Opponent.%d (%.1f %.1f) type=%d cycle=%d",
                      (*it)->unum(),
                      (*it)->pos().x, (*it)->pos().y,
                      player_type->id(),
                      cycle );
        if ( cycle < second_min_cycle )
        {
            second_min_cycle = cycle;
            M_second_opponent = *it;

            if ( second_min_cycle < min_cycle )
            {
                std::swap( min_cycle, second_min_cycle );
                std::swap( M_fastest_opponent, M_second_opponent );
            }
        }
    }

    if ( M_second_opponent && second_min_cycle < 1000 )
    {
        M_second_opponent_reach_cycle = second_min_cycle;
    }

    if ( M_fastest_opponent && min_cycle < 1000 )
    {
        M_opponent_reach_cycle = min_cycle;
    }
}

}

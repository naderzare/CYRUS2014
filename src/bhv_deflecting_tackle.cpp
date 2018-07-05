// -*-c++-*-

/*!
  \file bhv_deflecting_tackle.cpp
  \brief tackle ball to out of our goal
*/

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_deflecting_tackle.h"


#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/math_util.h>
#include <cmath>
#include <vector>


namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
Bhv_DeflectingTackle::Bhv_DeflectingTackle( double tackle_prob_threshold,
                                            bool force_tackle )
    : M_tackle_prob_threshold( tackle_prob_threshold )
    , M_force_tackle( force_tackle )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DeflectingTackle::execute( rcsc::PlayerAgent * agent )
{
    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__":Bhv_DeflectingTackle::execute(): "
                        "probability_threshold = %f, force = %s",
                        M_tackle_prob_threshold,
                        ( M_force_tackle ? "true" : "false" ) );

    const rcsc::WorldModel & wm = agent->world();

    const int teammate_reach_cycle = wm.interceptTable()->teammateReachCycle();
    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();

    if ( ! M_force_tackle
         && ! ( this->isShootBall( wm.ball().vel(), wm, 2.0 )
                && wm.ball().inertiaPoint( std::min( teammate_reach_cycle,
                                                     self_reach_cycle ) ).x
                <= rcsc::ServerParam::i().ourTeamGoalLineX() ) )
    {
        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": no need to deflecting" );

        return false;
    }


    //
    // check tackle success probability
    //
    const double tackle_probability = wm.self().tackleProbability();

    rcsc::dlog.addText( rcsc::Logger::ACTION,
                        __FILE__": tackle probability = %f",
                        tackle_probability );

    if ( tackle_probability < M_tackle_prob_threshold )
    {
        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": tackle probability too low" );
        return false;
    }


    const double goal_half_width = rcsc::ServerParam::i().goalHalfWidth();
    const double goal_line_x = rcsc::ServerParam::i().ourTeamGoalLineX();
    const rcsc::Vector2D goal_plus_post( goal_line_x, +goal_half_width );
    const rcsc::Vector2D goal_minus_post( goal_line_x, -goal_half_width );

    const rcsc::Vector2D ball_pos = wm.ball().pos();


    //
    // create candidates
    //
    std::vector<rcsc::AngleDeg> candidates;

    if ( ball_pos.y > 0.0 )
    {
        candidates.push_back( ( goal_plus_post + rcsc::Vector2D( 0.0, +5.0 )
                                - ball_pos ).th() );
    }
    else
    {
        candidates.push_back( ( goal_minus_post + rcsc::Vector2D( 0.0, -5.0 )
                                - ball_pos ).th() );
    }

    for( int d = -180; d < 180; d += 10 )
    {
        candidates.push_back( rcsc::AngleDeg( d ) );
    }


    //
    // evalute candidates
    //
    static const double not_shoot_ball_eval = 10000;

    struct EvaluateFunction
    {
        static
        double evaluate( const rcsc::Vector2D & vec,
                         const rcsc::WorldModel & wm,
                         PlayerAgent * agent )
        {
            double eval = 0.0;

            if ( ! isShootBall( vec, wm, 2.0 ) )
            {
                eval += not_shoot_ball_eval;
            }

            rcsc::Vector2D final_ball_pos;
            final_ball_pos = rcsc::inertia_final_point( wm.ball().pos(),
                                                        vec,
                                                        rcsc::ServerParam::i().ballDecay() );

            if ( vec.x < 0.0
                 && wm.ball().pos().x > rcsc::ServerParam::i().ourTeamGoalLineX() + 0.5 )
            {
                const double goal_half_width = rcsc::ServerParam::i().goalHalfWidth();
                const double pitch_half_width = rcsc::ServerParam::i().pitchHalfWidth();
                const double goal_line_x = rcsc::ServerParam::i().ourTeamGoalLineX();
                const rcsc::Vector2D corner_plus_post( goal_line_x, +pitch_half_width );
                const rcsc::Vector2D corner_minus_post( goal_line_x, -pitch_half_width );

                const rcsc::Line2D goal_line( corner_plus_post, corner_minus_post );

                const rcsc::Segment2D ball_segment( wm.ball().pos(), final_ball_pos );
                rcsc::Vector2D cross_point = ball_segment.intersection( goal_line );
                if ( cross_point.isValid() )
                {
                    eval += 1000.0;

                    double c = std::min( std::fabs( cross_point.y ),
                                         rcsc::ServerParam::i().pitchHalfWidth() );

                    if ( c > goal_half_width + 3.0
                         && ( cross_point.y * wm.self().pos().y >= 0.0
                              && c > wm.self().pos().absY() ) )
                    {
                        eval += rcsc::ServerParam::i().pitchHalfWidth() - c;
                    }           
                }
            }
            else
            {
                if ( final_ball_pos.x > rcsc::ServerParam::i().ourTeamGoalLineX() + 5.0 )
                {
                    eval += 1000.0;
                }

                eval += rcsc::sign( wm.ball().pos().y ) * vec.y;
            }

            agent->debugClient().addLine( wm.ball().pos(), final_ball_pos );

            return eval;
        }
    };

    double max_eval = -1;
    size_t max_index = 0;

    for( size_t i = 0; i < candidates.size(); ++i )
    {
        const rcsc::Vector2D result_vec = this->getTackleResult( candidates[i], wm );

        double eval = EvaluateFunction::evaluate( result_vec, wm, agent );

        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": th = %f, result_vec = [%f, %f], "
                            "eval = %f",
                            candidates[i].degree(), result_vec.x, result_vec.y,
                            eval );

        if ( eval > max_eval )
        {
            max_index = i;
            max_eval = eval;
        }
    }


    const double tackle_dir = ( candidates[max_index] - wm.self().body() ).degree();
    //
    // debug print
    //
    {
        const rcsc::Vector2D & best_vec = this->getTackleResult( candidates[max_index], wm );
        const rcsc::Vector2D next_ball_pos = wm.ball().pos() + best_vec;

        const rcsc::Vector2D final_ball_pos = rcsc::inertia_final_point( wm.ball().pos(),
                                                                         best_vec,
                                                                         rcsc::ServerParam::i().ballDecay() );

        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": best candidate: th = %f, eval = %f, "
                            "next ball pos = [%f, %f], final ball pos = [%f, %f ]",
                            candidates[max_index].degree(),
                            max_eval,
                            next_ball_pos.x, next_ball_pos.y,
                            final_ball_pos.x, final_ball_pos.y );

        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": tackle_dir = %f, self body direction = %f",
                            tackle_dir, wm.self().body().degree() );

        agent->debugClient().setTarget( final_ball_pos );
    }


    //
    // do tackle
    //
    if ( max_eval < not_shoot_ball_eval
         && ! M_force_tackle )
    {
        rcsc::dlog.addText( rcsc::Logger::ACTION,
                            __FILE__": no valid tackle course found" );
        return false;
    }


    agent->doTackle( tackle_dir );

    agent->setNeckAction( new rcsc::Neck_TurnToBall() );

    return true;
}


bool
Bhv_DeflectingTackle::isShootBall( const rcsc::Vector2D & vec,
                                   const rcsc::WorldModel & wm,
                                   double buffer )
{
    const double goal_half_width = rcsc::ServerParam::i().goalHalfWidth();
    const double goal_line_x = rcsc::ServerParam::i().ourTeamGoalLineX();
    const rcsc::Vector2D goal_plus_post( goal_line_x,
                                         +goal_half_width + buffer );
    const rcsc::Vector2D goal_minus_post( goal_line_x,
                                          -goal_half_width - buffer );

    return ( ( (goal_plus_post - wm.ball().pos() ).th()
               - vec.th() ).degree() < 0
             && ( ( goal_minus_post - wm.ball().pos() ).th()
                  - vec.th() ).degree() > 0 );
}




rcsc::Vector2D
Bhv_DeflectingTackle::getTackleResult( const rcsc::AngleDeg & absolute_dir,
                                       const rcsc::WorldModel & wm )
{
    return wm.ball().vel() + getTackleAccel( absolute_dir, wm );
}

rcsc::Vector2D
Bhv_DeflectingTackle::getTackleAccel( const rcsc::AngleDeg & absolute_dir,
                                      const rcsc::WorldModel & wm )
{
    const rcsc::ServerParam & param = rcsc::ServerParam::i();

    double rel_target_dir = ( absolute_dir - wm.self().body() ).degree();

    const double tackle_target_penalty = 1.0 - (std::fabs( rel_target_dir ) / 180.0);
    const double ball_relative_penalty = 1.0 - 0.5 * ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() / 180.0 );

    double effective_power = ( param.maxBackTacklePower()
                               + ( param.maxTacklePower()
                                   - param.maxBackTacklePower() )
                                  * tackle_target_penalty
                                  * ball_relative_penalty )
                                  * param.tacklePowerRate();

    return rcsc::Vector2D::polar2vector( effective_power, absolute_dir );
}

}

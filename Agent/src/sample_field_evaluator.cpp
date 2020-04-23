// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sample_field_evaluator.h"

#include "field_analyzer.h"
#include "simple_pass_checker.h"

#include <rcsc/player/player_evaluator.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/math_util.h>

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>

// #define DEBUG_PRINT

using namespace rcsc;

static const int VALID_PLAYER_THRESHOLD = 8;


/*-------------------------------------------------------------------*/
/*!

 */
static double evaluate_state( const PredictState & state );


/*-------------------------------------------------------------------*/
/*!

 */
SampleFieldEvaluator::SampleFieldEvaluator()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
SampleFieldEvaluator::~SampleFieldEvaluator()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
double
SampleFieldEvaluator::operator()( const PredictState & state,
		const std::vector< ActionStatePair > & path ) const
{
	const double final_state_evaluation = evaluate_state( state );

	int safe = true;
	if ( path.size() > 0 )
		if ( !path[0].state().M_safe ){
			safe = false;
		}

	//
	// ???
	//

	double result = final_state_evaluation;
	if ( !safe ) result-=2;
	return result;
}


/*-------------------------------------------------------------------*/
/*!

 */
static
double
evaluate_state( const PredictState & state )
{
	const ServerParam & SP = ServerParam::i();

	const AbstractPlayerObject * holder = state.ballHolder();


#ifdef DEBUG_PRINT
	dlog.addText( Logger::ACTION_CHAIN,
			"========= (evaluate_state) ==========" );
#endif

	//
	// if holder is invalid, return bad evaluation
	//
	if ( ! holder )
	{
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) XXX null holder" );
#endif
		return - DBL_MAX / 2.0;
	}

	const int holder_unum = holder->unum();


	//
	// ball is in opponent goal
	//
	if ( state.ball().pos().x > + ( SP.pitchHalfLength() - 0.1 )
			&& state.ball().pos().absY() < SP.goalHalfWidth() + 2.0 )
	{
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) *** in opponent goal" );
#endif
		return +1.0e+7;
	}

	//
	// ball is in our goal
	//
	if ( state.ball().pos().x < - ( SP.pitchHalfLength() - 0.1 )
			&& state.ball().pos().absY() < SP.goalHalfWidth() )
	{
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) XXX in our goal" );
#endif

		return -1.0e+7;
	}

	//
	// out of pitch
	//
	if ( state.ball().pos().absX() > SP.pitchHalfLength()
			|| state.ball().pos().absY() > SP.pitchHalfWidth() )
	{
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) XXX out of pitch" );
#endif

		return - DBL_MAX / 2.0;
	}

	double eval=SampleFieldEvaluator().evaluator(state.ball().pos());

	//
	// add bonus for goal, free situation near offside line
	//
	if ( FieldAnalyzer::can_shoot_from
			( holder->unum() == state.self().unum(),
					holder->pos(),
					state.getPlayerCont( new OpponentOrUnknownPlayerPredicate( state.ourSide() ) ),
					VALID_PLAYER_THRESHOLD ) )
	{
		eval += 1.0e+6;
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) bonus for goal %f (%f)", 1.0e+6, point );
#endif

		eval += state.ball().pos().x;
		if ( holder_unum == state.self().unum() )
		{
//			eval += 5.0e+5;
#ifdef DEBUG_PRINT
			dlog.addText( Logger::ACTION_CHAIN,
					"(eval) bonus for goal self %f (%f)", 5.0e+5, point );
#endif
		}
	}


	if(state.currentTime().cycle()>3000 && state.currentTime().cycle()<4500)
		return eval;





	//
	// set basic evaluation
	//
	double point = state.ball().pos().x+52.5;

	point += std::max( 0.0,
			40.0 - ServerParam::i().theirTeamGoalPos().dist( state.ball().pos() ) );

	if(state.ball().pos().y>20 && state.ball().pos().x < 30)
		point -= ((state.ball().pos().y-20)/2);
#ifdef DEBUG_PRINT
	dlog.addText( Logger::ACTION_CHAIN,
			"(eval) ball pos (%f, %f)",
			state.ball().pos().x, state.ball().pos().y );

	dlog.addText( Logger::ACTION_CHAIN,
			"(eval) initial value (%f)", point );
#endif

	//
	// add bonus for goal, free situation near offside line
	//
	if ( FieldAnalyzer::can_shoot_from
			( holder->unum() == state.self().unum(),
					holder->pos(),
					state.getPlayerCont( new OpponentOrUnknownPlayerPredicate( state.ourSide() ) ),
					VALID_PLAYER_THRESHOLD ) )
	{
		point += 1.0e+6;
#ifdef DEBUG_PRINT
		dlog.addText( Logger::ACTION_CHAIN,
				"(eval) bonus for goal %f (%f)", 1.0e+6, point );
#endif
		point += state.ball().pos().x;

		if ( holder_unum == state.self().unum() )
		{
//			point += 5.0e+5;
#ifdef DEBUG_PRINT
			dlog.addText( Logger::ACTION_CHAIN,
					"(eval) bonus for goal self %f (%f)", 5.0e+5, point );
#endif
		}
	}
	return point;
}
static double eval_pass[15][22]=
{{8.0,10.0,14.0,16.0,20.0,20.0,24.0,32.0,32.0,40.0,40.0,42.0,60.0,62.8,68.0,70.0,76.0,80.0,80.0,80.0,78.8,68.0},
{8.0,12.0,14.0,18.0,22.0,24.0,26.0,32.0,34.0,40.0,44.0,50.8,62.0,66.0,70.0,72.8,76.8,82.8,83.6,83.6,85.2,74.8},
{10.0,12.0,16.0,18.0,22.0,24.0,26.0,30.8,34.0,40.0,46.0,52.8,62.0,66.0,70.0,72.8,78.0,82.8,84.4,85.6,86.0,82.8},
{8.0,10.0,16.0,18.0,22.0,26.0,26.0,30.8,36.0,42.8,46.8,52.0,60.0,64.0,68.8,72.0,78.0,82.0,84.8,88.0,87.6,86.0},
{4.0,8.0,16.0,18.0,22.0,26.0,26.8,30.8,40.0,44.0,48.8,52.8,60.0,62.0,68.8,72.0,76.0,80.0,86.0,88.0,90.0,88.0},
{4.0,6.0,16.0,16.0,22.0,24.0,26.8,32.8,40.8,48.0,48.8,54.0,60.0,62.0,64.0,70.0,74.0,80.0,86.0,92.0,96.0,94.0},
{4.0,4.0,12.0,16.0,20.0,24.0,28.8,32.8,40.8,48.0,48.8,52.0,56.0,60.0,64.0,70.0,74.0,78.0,84.0,94.0,98.0,96.8},
{0.0,4.0,10.8,16.8,18.0,24.0,26.8,32.8,40.8,46.0,48.0,50.8,56.0,60.0,64.0,68.0,70.8,76.0,84.0,98.0,100.0,100.0},
{2.0,4.0,10.0,16.0,18.0,22.0,26.0,32.0,36.8,46.0,48.0,50.8,56.0,58.0,64.0,64.0,70.0,76.0,82.0,96.0,96.0,96.0},
{2.0,2.0,10.8,14.0,16.0,22.0,26.0,30.0,36.8,44.8,46.0,50.0,54.0,56.0,64.0,64.0,68.0,74.0,82.0,96.0,94.8,96.0},
{4.0,4.0,10.0,14.0,16.0,22.0,26.0,28.8,36.0,44.0,44.8,48.0,50.0,54.0,56.0,64.0,68.0,74.0,82.0,88.0,88.0,88.0},
{4.0,6.0,10.0,12.8,16.0,22.0,22.8,24.8,30.0,38.0,40.0,46.0,48.0,50.0,56.0,56.0,64.0,66.0,72.0,76.0,78.0,72.0},
{4.0,6.0,8.0,12.8,16.0,18.0,22.0,24.0,30.0,28.0,38.0,42.0,42.0,48.0,48.0,56.0,64.0,66.0,62.0,62.0,58.0,56.0},
{4.0,6.0,8.0,10.8,16.0,16.0,16.0,22.0,24.0,28.0,26.0,40.0,42.0,42.0,44.0,56.0,56.0,56.0,58.0,58.0,56.0,56.0},
{2.8,4.8,6.0,8.0,10.0,16.0,16.0,16.0,16.0,16.0,24.0,32.0,32.0,32.0,40.0,48.0,48.0,48.0,48.0,48.0,48.0,48.0}}

;

double SampleFieldEvaluator::evaluator(Vector2D target,int cycle)
{
	double point = target.x+52.5;
	point += std::max( 0.0,
			40.0 - ServerParam::i().theirTeamGoalPos().dist( target ) );

	if(target.y>20 && target.x < 30)
		point -= ((target.y-20)/2);

	if(cycle<3000 ||cycle>4500)return point;

	if(target.absY()>33.8||target.x<-50)return 0;
	double x_target= target.x +52.5;
	double y_target=target.y +35;
    int x1 =static_cast<int>(x_target/5)*5;
	int x2 =x1+5;
    int y1= static_cast<int>(y_target/5)*5;
	int y2= y1+5;
	double s1= (x_target-x1)*(y_target-y1);
	double s2= (x2-x_target)*(y_target-y1);
	double s3= (x_target-x1)*(y2-y_target);
	double s4= (x2-x_target)*(y2-y_target);

    double m1= eval_pass[static_cast<int>(y1/5)][static_cast<int>(x1/5)];
    double m2 = eval_pass[static_cast<int>(y1/5)][static_cast<int>(x2/5)];
    double m3 = eval_pass[static_cast<int>(y2/5)][static_cast<int>(x1/5)];
    double m4 = eval_pass[static_cast<int>(y2/5)][static_cast<int>(x2/5)];
	double tmp = (s4*m1)+(s3*m2)+(s2*m3)+(s1*m4);
	tmp /=(25);
	return tmp;
}


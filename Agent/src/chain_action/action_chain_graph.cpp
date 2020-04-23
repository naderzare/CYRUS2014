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

#include "action_chain_graph.h"

#include "hold_ball.h"
#include "../sample_field_evaluator.h"
#include "../cyrus.h"
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/time/timer.h>
#include <rcsc/player/intercept_table.h>
#include <string>
#include <sstream>
#include <queue>
#include <utility>
#include <limits>
#include <cstdio>
#include <cmath>

//#define DEBUG_PROFILE
//#define ACTION_CHAIN_DEBUG
// #define DEBUG_PAINT_EVALUATED_POINTS

//#define ACTION_CHAIN_LOAD_DEBUG
//#define DEBUG_COMPARE_SEARCH_TYPES

#ifdef ACTION_CHAIN_LOAD_DEBUG
#include <iostream>
#endif

#include <iostream>

using namespace rcsc;

bool tcode = false;

size_t ActionChainGraph::DEFAULT_MAX_CHAIN_LENGTH = 4;
size_t ActionChainGraph::DEFAULT_MAX_EVALUATE_LIMIT = 500;

std::vector<std::pair<Vector2D, double> > ActionChainGraph::S_evaluated_points;

std::vector<ActionStatePair> empty_path;

std::vector<ActionStatePair> ActionChainGraph::My_result = empty_path; /////cyrus
double ActionChainGraph::My_best_evaluation = 0; /////cyrus

namespace {

const double HEAT_COLOR_SCALE = 128.0;
const double HEAT_COLOR_PERIOD = 2.0 * M_PI;

inline
int heat_color(const double & x) {
	//return std::floor( ( std::cos( x ) + 1.0 ) * HEAT_COLOR_SCALE );
	return static_cast<int>(std::floor((std::cos(x) + 1.0) * HEAT_COLOR_SCALE));
}

inline
void debug_paint_evaluate_color(const Vector2D & pos, const double & value,
		const double & min, const double & max) {
	double position = (value - min) / (max - min);
	if (position < 0.0)
		position = -position;
	if (position > 2.0)
		position = std::fmod(position, 2.0);
	if (position > 1.0)
		position = 1.0 - (position - 1.0);

	double shift = 0.5 * position + 1.7 * (1.0 - position);
	double x = shift + position * HEAT_COLOR_PERIOD;

	int r = heat_color(x);
	int g = heat_color(x + M_PI * 0.5);
	int b = heat_color(x + M_PI);

	// dlog.addText( Logger::ACTION_CHAIN,
	//               "(paint) (%.2f %.2f) eval=%f -> %d %d %d",
	//               pos.x, pos.y, value, r, g, b );
	char str[16];
	snprintf(str, 16, "%lf", value);
	dlog.addMessage(Logger::ACTION_CHAIN, pos, str, "#ffffff");
	dlog.addRect(Logger::ACTION_CHAIN, pos.x - 0.1, pos.y - 0.1, 0.2, 0.2, r, g,
			b, true);
}

}

/*-------------------------------------------------------------------*/
/*!

 */
int ActionChainGraph::tmmate = 0;
ActionChainGraph::ActionChainGraph(const FieldEvaluator::ConstPtr & evaluator,
		const ActionGenerator::ConstPtr & generator,
		unsigned long max_chain_length, long max_evaluate_limit) :
		M_evaluator(evaluator), M_action_generator(generator), M_chain_count(0), M_best_chain_count(
				0), M_max_chain_length(max_chain_length), M_max_evaluate_limit(
				max_evaluate_limit), M_result(), M_best_evaluation(
				-std::numeric_limits<double>::max()) {
#ifdef DEBUG_PAINT_EVALUATED_POINTS
	S_evaluated_points.clear();
	S_evaluated_points.reserve( DEFAULT_MAX_EVALUATE_LIMIT + 1 );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */

void ActionChainGraph::calculateResult(const WorldModel & wm) {
	debugPrintCurrentState(wm);

	if (!wm.self().isKickable()) {
		ActionChainGraph::DEFAULT_MAX_CHAIN_LENGTH = 3;
		ActionChainGraph::DEFAULT_MAX_EVALUATE_LIMIT = 100;
	} else {
		ActionChainGraph::DEFAULT_MAX_CHAIN_LENGTH = 4;
		ActionChainGraph::DEFAULT_MAX_EVALUATE_LIMIT = 1500;
	}
#if (defined DEBUG_PROFILE) || (defined ACTION_CHAIN_LOAD_DEBUG)
	Timer timer;
#endif

	unsigned long n_evaluated = 0;
	M_chain_count = 0;
	M_best_chain_count = 0;

	//
	// best first
	//
	calculateResultBestFirstSearch(wm, &n_evaluated);
	if (!M_result.empty()) {
        for (size_t i = 0; i < M_result.size(); i++) {
			if (i == 0)
				tmmate = wm.interceptTable()->fastestTeammate()->unum();
			else
				tmmate = M_result[i - 1].state().ballHolderUnum();
			if (M_result[i].action().targetPlayerUnum() == wm.self().unum()) {
				break;
			}
			tmmate = 0;
		}
		//		if(M_result.front().action().targetPlayerUnum()==wm.self().unum()){
		//			tmmate = wm.interceptTable()->fastestTeammate()->unum();
		//		}else{
		//			tmmate = 0;
		//		}
	}
	if (M_result.empty()) {
		const PredictState current_state(wm);

		PredictState::ConstPtr result_state(new PredictState(current_state, 1));
		CooperativeAction::Ptr action(
				new HoldBall(wm.self().unum(), wm.ball().pos(), 1,
						"defaultHold"));
		action->setFinalAction(true);

		M_result.push_back(ActionStatePair(action, result_state));
	}

	write_chain_log(">>>>> best chain: ", wm, M_best_chain_count, M_result,
			M_best_evaluation);

#if (defined DEBUG_PROFILE) || (defined ACTION_CHAIN_LOAD_DEBUG)
	const double msec = timer.elapsedReal();
#ifdef DEBUG_PROFILE
	dlog.addText( Logger::ACTION_CHAIN,
			__FILE__": PROFILE size=%d elapsed %f [ms]",
			M_chain_count,
			msec );
#endif
#ifdef ACTION_CHAIN_LOAD_DEBUG
	std::fprintf( stderr,
			"# recursive search: %2d [%ld] % 10lf msec, n=%4lu, average=% 10lf\n",
			wm.self().unum(),
			wm.time().cycle(),
			msec,
			n_evaluated,
			( n_evaluated == 0 ? 0.0 : msec / n_evaluated ) );
#endif
#endif

#ifdef DEBUG_PAINT_EVALUATED_POINTS
	if ( ! S_evaluated_points.empty() )
	{
		double min_eval = +std::numeric_limits< double >::max();
		double max_eval = -std::numeric_limits< double >::max();
		const std::vector< std::pair< Vector2D, double > >::const_iterator end = S_evaluated_points.end();
		for ( std::vector< std::pair< Vector2D, double > >::const_iterator it = S_evaluated_points.begin();
				it != end;
				++it )
		{
			if ( min_eval > it->second ) min_eval = it->second;
			if ( max_eval < it->second ) max_eval = it->second;
		}

		if ( std::fabs( min_eval - max_eval ) < 1.0e-5 )
		{
			max_eval = min_eval + 1.0e-5;
		}

		for ( std::vector< std::pair< Vector2D, double > >::const_iterator it = S_evaluated_points.begin();
				it != end;
				++it )
		{
			debug_paint_evaluate_color( it->first, it->second, min_eval, max_eval );
		}
	}
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void ActionChainGraph::calculateResultChain(const WorldModel & wm,
		unsigned long * n_evaluated) {
	M_result.clear();
	M_best_evaluation = -std::numeric_limits<double>::max();

	const PredictState current_state(wm);

	std::vector<ActionStatePair> empty_path;
	double evaluation;

	doSearch(wm, current_state, empty_path, &M_result, &evaluation, n_evaluated,
			M_max_chain_length, M_max_evaluate_limit);

	M_best_evaluation = evaluation;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool ActionChainGraph::doSearch(const WorldModel & wm,
		const PredictState & state, const std::vector<ActionStatePair> & path,
		std::vector<ActionStatePair> * result, double * result_evaluation,
		unsigned long * n_evaluated, unsigned long max_chain_length,
		long max_evaluate_limit) {
	if (path.size() > max_chain_length) {
		return false;
	}

	//
	// check evaluation limit
	//
	if (max_evaluate_limit != -1
			&& *n_evaluated >= static_cast<unsigned int>(max_evaluate_limit)) {
		dlog.addText(Logger::ACTION_CHAIN, "cut by max evaluate limit %d",
				*n_evaluated);
#if 0
#if defined( ACTION_CHAIN_DEBUG ) || defined( ACTION_CHAIN_LOAD_DEBUG )
		std::cerr << "Max evaluate limit " << max_evaluate_limit
		<< " exceeded" << std::endl;
#endif
#endif
		return false;
	}

	//
	// check current state
	//
	std::vector<ActionStatePair> best_path = path;
	double max_ev = (*M_evaluator)(state, path);
#ifdef DO_MONTECARLO_SEARCH
	double eval_sum = 0;
	long n_eval = 0;
#endif

	++M_chain_count;
#ifdef ACTION_CHAIN_DEBUG
	write_chain_log( wm, M_chain_count, path, max_ev );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
	S_evaluated_points.push_back( std::pair< Vector2D, double >( state.ball().pos(), max_ev ) );
#endif
	++(*n_evaluated);

	//
	// generate candidates
	//
	std::vector<ActionStatePair> candidates;
	if (path.empty() || !(*(path.rbegin())).action().isFinalAction()) {
		M_action_generator->generate(&candidates, state, wm, path);
	}

	//
	// test each candidate
	//
	for (std::vector<ActionStatePair>::const_iterator it = candidates.begin();
			it != candidates.end(); ++it) {
		std::vector<ActionStatePair> new_path = path;
		std::vector<ActionStatePair> candidate_result;

		new_path.push_back(*it);

		double ev;
		const bool exist = doSearch(wm, (*it).state(), new_path,
				&candidate_result, &ev, n_evaluated, max_chain_length,
				max_evaluate_limit);
		if (!exist) {
			continue;
		}
#ifdef ACTION_CHAIN_DEBUG
		write_chain_log( wm, M_chain_count, candidate_result, ev );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
		S_evaluated_points.push_back( std::pair< Vector2D, double >( it->state().ball().pos(), ev ) );
#endif
		if (ev > max_ev) {
			max_ev = ev;
			best_path = candidate_result;
		}

#ifdef DO_MONTECARLO_SEARCH
		eval_sum += ev;
		++n_eval;
#endif
	}

	*result = best_path;
#ifdef DO_MONTECARLO_SEARCH
	if ( n_eval == 0 )
	{
		*result_evaluation = max_ev;
	}
	else
	{
		*result_evaluation = eval_sum / n_eval;
	}
#else
	*result_evaluation = max_ev;
#endif

	return true;
}

class ChainComparator {
public:
	bool operator()(const std::pair<std::vector<ActionStatePair>, double> & a,
			const std::pair<std::vector<ActionStatePair>, double> & b) {
		return (a.second < b.second);
	}
};

/*-------------------------------------------------------------------*/
/*!

 */

double ActionChainGraph::passOrDribleEffect(double *cases,
		std::vector<ActionStatePair> *candidate_series, bool pass, double root,
		double current) {
	int i = 0;	//in for , it starts from the state we are in
	double minuseEv = 0;
	bool is = false;
	if ((root + current) > 0)
		is = true;

	ActionStatePair rootPair = candidate_series->front();
	ActionStatePair currentPair = candidate_series->front();

	for (std::vector<ActionStatePair>::const_iterator it =
			candidate_series->begin(); it != candidate_series->end(); ++it) {
		currentPair = (*it);
	}


	switch (is) {
	case 1:
        if(!pass){

            if (abs(root - 1.0) < 0.1) {
                if ((rootPair).action().category()
                        == CooperativeAction::Dribble)
                minuseEv += ((rootPair).state().M_safeState) * root;//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
            }
            if (abs(current - 1.0) < 0.1) {
                if ((currentPair).action().category()
                        == CooperativeAction::Dribble)
                    minuseEv += ((currentPair).state().M_safeState) * current;//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
            }

        }else{
            if (abs(root - 1.0) < 0.1) {
                minuseEv += ((rootPair).state().M_safeState) * root;//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
            }
            if (abs(current - 1.0) < 0.1) {
                minuseEv += ((currentPair).state().M_safeState) * current;//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
            }
        }
		break;
	case 0:
        if(!pass){
            for (std::vector<ActionStatePair>::const_iterator it =
                    candidate_series->begin(); it != candidate_series->end();
                    ++it) {
                if (cases[i] > 0) {
                    if ((*it).action().category()
                            == CooperativeAction::Dribble) {
                        minuseEv += ((*it).state().M_safeState) * (cases[i]);//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
                        i++;
                    }
                }
            }
        }else{
            for (std::vector<ActionStatePair>::const_iterator it =
                    candidate_series->begin(); it != candidate_series->end();
                    ++it) {

                if (cases[i] > 0) {
                    minuseEv += ((*it).state().M_safeState) * (cases[i]);//shayad cases[i] yek meghdar joz 0 ya 1 dasht.
                    i++;
                }
            }
        }
		break;
	}
	////cout << "##########minuseTTTTTTTTTTTTTTT" << minuseEv << endl;
	return minuseEv;

}
void ActionChainGraph::calculateResultBestFirstSearch(const WorldModel & wm,
		unsigned long * n_evaluated)

		{
	dlog.addText(Logger::TEAM, __FILE__": calcRBFS");

	bool pass = true;

	double case9[4] = { 1, 1 / 2, 1 / 4, 1 / 8 };

	if(cyrus().amIwin(wm,2))
		tcode =true;
	if(wm.time().cycle()>5000){
		if(cyrus().amIlost(wm,1)){
			tcode =true;
		}
	}
	//
	// initialize
	//
	M_result.clear();
	M_best_evaluation = -std::numeric_limits<double>::max();
	*(n_evaluated) = 0;

	//
	// create priority queue
	//
	std::priority_queue<std::pair<std::vector<ActionStatePair>, double>,
			std::vector<std::pair<std::vector<ActionStatePair>, double> >,
			ChainComparator> queue;

	//
	// check current state
	//
	const PredictState current_state(wm);
	const std::vector<ActionStatePair> empty_path;

	

	double current_evaluation = (*M_evaluator)(current_state, empty_path) - 10;
	int cycle = 100;
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();
	for (PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end;
			++it) {
		Vector2D target = wm.ball().pos();
		int oppc = (*it)->playerTypePtr()->cyclesToReachDistance(
				target.dist((*it)->pos()))
				- std::min(std::max(0, (*it)->posCount()), 3);
		if (oppc < cycle)
			cycle = oppc;
	}
	if (tcode) {
		double block = cyrus().block_drible(current_state);
			////cout << "__block__" << block << endl;

			double thr = 1.5;
			thr *= block;
			current_evaluation-=thr;
	} else {
		if (cycle <= 15) {
			current_evaluation -= 3;
		}
	}

//	cout<<"***"<<wm.time().cycle()<<" too h"<<" mev "<<thr<<" "<<current_evaluation<<endl;

	My_result = empty_path;	/////cyrus

	My_best_evaluation =
			(current_state.ballHolderUnum() == current_state.self().unum()) ?
					current_evaluation : 0;	/////cyrus

	++M_chain_count;
	++(*n_evaluated);
#ifdef ACTION_CHAIN_DEBUG
	write_chain_log( wm, M_chain_count, empty_path, current_evaluation );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
	S_evaluated_points.push_back( std::pair< Vector2D, double >
			( current_state.ball().pos(), current_evaluation ) );
#endif
	M_result = empty_path;
	M_best_evaluation = current_evaluation;

	queue.push(
			std::pair<std::vector<ActionStatePair>, double>(empty_path,
					current_evaluation));

	//
	// main loop
	//
	dlog.addText(Logger::TEAM, __FILE__": start main loop");
	for (;;) {
		//
		// pick up most valuable action chain
		//
		if (queue.empty()) {
			break;
		}

		std::vector<ActionStatePair> series = queue.top().first;
		queue.pop();

		//
		// get state candidates
		//
		const PredictState * state;
		if (series.empty()) {
			state = &current_state;
		} else {
			state = &((*(series.rbegin())).state());
		}

		//
		// generate action candidates
		//
		std::vector<ActionStatePair> candidates;
		if (series.size() < M_max_chain_length
				&& (series.empty()
						|| !(*(series.rbegin())).action().isFinalAction())) {
			M_action_generator->generate(&candidates, *state, wm, series);
#ifdef ACTION_CHAIN_DEBUG
			dlog.addText( Logger::ACTION_CHAIN,
					">>>> generate (%s[%d]) candidate_size=%d <<<<<",
					( series.empty() ? "empty" : series.rbegin()->action().description() ),
					( series.empty() ? -1 : series.rbegin()->action().index() ),
					candidates.size() );
#endif
		}

		//
		// evaluate each candidate and push to priority queue
		//
		for (std::vector<ActionStatePair>::const_iterator it =
				candidates.begin(); it != candidates.end(); ++it) {
			++M_chain_count;
			std::vector<ActionStatePair> candidate_series = series;
			candidate_series.push_back(*it);

			double ev = (*M_evaluator)((*it).state(), candidate_series);
	
			if ((*it).action().category() == CooperativeAction::Dribble) {
				Vector2D target =
						candidate_series.front().action().targetPoint();
				//            	ev-=15;
				int cycle = 100;
				const PlayerPtrCont & opps = wm.opponentsFromSelf();
				const PlayerPtrCont::const_iterator opps_end = opps.end();
				for (PlayerPtrCont::const_iterator oo = opps.begin();
						oo != opps_end; ++oo) {
					if ((*oo)->unum() < 0)
						continue;
					if ((*oo)->goalie())
						continue;
					int oppc = (*oo)->playerTypePtr()->cyclesToReachDistance(
							target.dist((*oo)->pos()))
							- std::min(std::max(0, (*oo)->posCount()), 3);
                    int matc = static_cast<int>(candidate_series.front().state().spendTime() / 4);

					oppc -= matc;
					if (oppc < 0)
						oppc = 0;

					if (oppc < cycle)
						cycle = oppc;
				}
				if (tcode) {
					if (cycle <= 11) {
						double calcEv = passOrDribleEffect(case9,
								&candidate_series, pass, 0, 0);

						ev -= calcEv;
					}
				} 
			} else {
				Vector2D target =
						candidate_series.front().action().targetPoint();
				int cycle = 100;
				const PlayerPtrCont & opps = wm.opponentsFromSelf();
				const PlayerPtrCont::const_iterator opps_end = opps.end();
				for (PlayerPtrCont::const_iterator it = opps.begin();
						it != opps_end; ++it) {
					if ((*it)->unum() < 0)
						continue;

					int oppc = (*it)->playerTypePtr()->cyclesToReachDistance(
							target.dist((*it)->pos()))
							- std::min(std::max(0, (*it)->posCount()), 2);
                    int matc = static_cast<int>(candidate_series.front().state().spendTime());

//					oppc -= matc;
					if (oppc < 0)
						oppc = 0;
					if (oppc < cycle)
						cycle = oppc;
				}
				if (tcode) {
					if (cycle <= 11) {
						double calcEv = passOrDribleEffect(case9,
								&candidate_series, pass, 0, 0);

						ev -= calcEv;
					}
				} 
			}
            int spend = 0;		//(*it).state().spendTime();

            for (size_t i = 0; i < candidate_series.size(); i++) {
				spend += candidate_series[i].state().spendTime();
			}
			if (spend > 50)
				spend = 50;
			if (spend > 5)
				ev -= (spend / 10);
//			cout<<"ev"<<ev<<endl;

			++(*n_evaluated);
#ifdef ACTION_CHAIN_DEBUG
			write_chain_log( wm, M_chain_count, candidate_series, ev );
#endif
#ifdef DEBUG_PAINT_EVALUATED_POINTS
			S_evaluated_points.push_back( std::pair< Vector2D, double >
					( it->state().ball().pos(), ev ) );
#endif

			if (ev > M_best_evaluation) {
#ifdef ACTION_CHAIN_DEBUG
				dlog.addText( Logger::ACTION_CHAIN,
						"<<<< update best result." );
#endif
				M_best_chain_count = M_chain_count;
				M_best_evaluation = ev;
				M_result = candidate_series;
			}

			for (std::vector<ActionStatePair>::const_iterator it =
					candidate_series.begin(); it != candidate_series.end();
					++it) {

				if ((*it).statePtr()->ballHolderUnum()
						== (*it).statePtr()->self().unum()
						&& ev > My_best_evaluation) {		/////cyrus
					My_best_evaluation = ev;		/////cyrus
					My_result = candidate_series;		/////cyrus
				}		/////cyrus

			}

			if (M_max_evaluate_limit != -1
					&& *n_evaluated
							>= static_cast<unsigned int>(M_max_evaluate_limit)) {
#ifdef ACTION_CHAIN_DEBUG
				dlog.addText( Logger::ACTION_CHAIN,
						"***** over max evaluation count *****" );
#endif
				if (wm.self().isKickable()) {
					//					cout<<"***"<<wm.time().cycle()<<"***"<<endl;
					//					cout<<"0 "<<n_pass0<<" "<<n_drible0<<endl;
					//					cout<<"1 "<<n_pass1<<" "<<n_drible1<<endl;
				}
				return;
			}

			queue.push(
					std::pair<std::vector<ActionStatePair>, double>(
							candidate_series, ev));
		}
	}

}

/*-------------------------------------------------------------------*/
/*!

 */
void ActionChainGraph::debugPrintCurrentState(const WorldModel & wm) {
#ifndef ACTION_CHAIN_DEBUG
	static_cast<void>(wm);
#else
	dlog.addText( Logger::ACTION_CHAIN, "=====" );

	for ( int n = 1; n <= ServerParam::i().maxPlayer(); ++n )
	{
		if ( n == wm.self().unum() )
		{
			dlog.addText( Logger::ACTION_CHAIN,
					"n = %d, self", n );
		}
		else
		{
			const AbstractPlayerObject * t = wm.ourPlayer( n );

			if ( t )
			{
				dlog.addText( Logger::ACTION_CHAIN,
						"n = %d, OK", n );
			}
			else
			{
				dlog.addText( Logger::ACTION_CHAIN,
						"n = %d, null", n );
			}
		}
	}
	dlog.addText( Logger::ACTION_CHAIN, "=====" );

	AbstractPlayerCont::const_iterator end = wm.ourPlayers().end();
	for ( AbstractPlayerCont::const_iterator it = wm.ourPlayers().begin();
			it != end;
			++it )
	{
		if ( (*it)->isSelf() )
		{
			dlog.addText( Logger::ACTION_CHAIN,
					"teammate %d, self", (*it)->unum() );
		}
		else
		{
			dlog.addText( Logger::ACTION_CHAIN,
					"teammate %d", (*it)->unum() );
		}
	}

	dlog.addText( Logger::ACTION_CHAIN, "=====" );
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void ActionChainGraph::debug_send_chain(PlayerAgent * agent,
		const std::vector<ActionStatePair> & path) {
	const double DIRECT_PASS_DIST = 3.0;

	const PredictState current_state(agent->world());

	for (size_t i = 0; i < path.size(); ++i) {
		std::string action_string = "?";
		std::string target_string = "";

		const CooperativeAction & action = path[i].action();
		const PredictState * s0;
		const PredictState * s1;

		if (i == 0) {
			s0 = &current_state;
			s1 = &(path[0].state());
		} else {
			s0 = &(path[i - 1].state());
			s1 = &(path[i].state());
		}

		switch (action.category()) {
		case CooperativeAction::Hold: {
			action_string = "hold";
			break;
		}

		case CooperativeAction::Dribble: {
			action_string = "dribble";

			agent->debugClient().addLine(s0->ball().pos(), s1->ball().pos());
			break;
		}

		case CooperativeAction::Pass: {
			action_string = "pass";

			std::stringstream buf;
			buf << action.targetPlayerUnum();
			target_string = buf.str();

			if (i == 0
					&& s0->ballHolderUnum() == agent->world().self().unum()) {
				agent->debugClient().setTarget(action.targetPlayerUnum());

				if (s0->ourPlayer(action.targetPlayerUnum())->pos().dist(
						s1->ball().pos()) > DIRECT_PASS_DIST) {
					agent->debugClient().addLine(s0->ball().pos(),
							s1->ball().pos());
				}
			} else {
				agent->debugClient().addLine(s0->ball().pos(),
						s1->ball().pos());
			}

			break;
		}

		case CooperativeAction::Shoot: {
			action_string = "shoot";

			agent->debugClient().addLine(s0->ball().pos(),
					Vector2D(ServerParam::i().pitchHalfLength(), 0.0));

			break;
		}

		case CooperativeAction::Move: {
			action_string = "move";
			break;
		}
		default: {
			action_string = "?";

			break;
		}
		}

		if (action.description()) {
			action_string += '|';
			action_string += action.description();
			if (!target_string.empty()) {
				action_string += ':';
				action_string += target_string;
			}
		}

		agent->debugClient().addMessage("%s", action_string.c_str());

		dlog.addText(Logger::ACTION_CHAIN, "chain %d %s, time = %lu", i,
				action_string.c_str(), s1->spendTime());
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void ActionChainGraph::write_chain_log(const WorldModel & world,
		const int count, const std::vector<ActionStatePair> & path,
		const double & eval) {
	write_chain_log("", world, count, path, eval);
}

/*-------------------------------------------------------------------*/
/*!

 */
void ActionChainGraph::write_chain_log(const std::string & pre_log_message,
		const WorldModel & world, const int count,
		const std::vector<ActionStatePair> & path, const double & eval) {
	if (!pre_log_message.empty()) {
		dlog.addText(Logger::ACTION_CHAIN, pre_log_message.c_str());
	}

	dlog.addText(Logger::ACTION_CHAIN, "%d: evaluation=%f", count, eval);

	const PredictState current_state(world);

	for (size_t i = 0; i < path.size(); ++i) {
		const CooperativeAction & a = path[i].action();
		const PredictState * s0;
		const PredictState * s1;

		if (i == 0) {
			s0 = &current_state;
			s1 = &(path[0].state());
		} else {
			s0 = &(path[i - 1].state());
			s1 = &(path[i].state());
		}

		switch (a.category()) {
		case CooperativeAction::Hold: {
			dlog.addText(Logger::ACTION_CHAIN, "__ %d: hold (%s) t=%d", i,
					a.description(), s1->spendTime());
			break;
		}

		case CooperativeAction::Dribble: {
			dlog.addText(Logger::ACTION_CHAIN,
					"__ %d: dribble (%s[%d]) t=%d unum=%d target=(%.2f %.2f)",
					i, a.description(), a.index(), s1->spendTime(),
					s0->ballHolderUnum(), a.targetPoint().x, a.targetPoint().y);
			break;
		}

		case CooperativeAction::Pass: {
			dlog.addText(Logger::ACTION_CHAIN,
					"__ %d: pass (%s[%d]) t=%d from[%d](%.2f %.2f)-to[%d](%.2f %.2f)",
					i, a.description(), a.index(), s1->spendTime(),
					s0->ballHolderUnum(), s0->ball().pos().x,
					s0->ball().pos().y, s1->ballHolderUnum(), a.targetPoint().x,
					a.targetPoint().y);
			break;
		}

		case CooperativeAction::Shoot: {
			dlog.addText(Logger::ACTION_CHAIN, "__ %d: shoot (%s) t=%d unum=%d",
					i, a.description(), s1->spendTime(), s0->ballHolderUnum());

			break;
		}

		case CooperativeAction::Move: {
			dlog.addText(Logger::ACTION_CHAIN, "__ %d: move (%s)", i,
					a.description(), s1->spendTime());
			break;
		}

		default: {
			dlog.addText(Logger::ACTION_CHAIN, "__ %d: ???? (%s)", i,
					a.description(), s1->spendTime());
			break;
		}
		}
	}
}

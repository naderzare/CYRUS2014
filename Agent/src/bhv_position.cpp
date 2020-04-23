/*
 * bhv_position.cpp
 *
 *  Created on: Nov 12, 2012
 *      Author: 007
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"
#include "bhv_pressing.h"
#include "cyrus.h"

#include "intention_receive.h"

#include "bhv_defense_mark_move.h"
#include <rcsc/player/say_message_builder.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/math_util.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <vector>

#include <rcsc/geom/ray_2d.h>
#include "neck_offensive_intercept_neck.h"
#include <iostream>
using namespace std;
using namespace rcsc;
#include <rcsc/math_util.h>
#include "bhv_position.h"
#include "sample_field_evaluator.h"
#include "chain_action/action_chain_graph.h"

static bool debug = false;
 int bhv_position::last_cycle = 0;
Vector2D bhv_position::target_pos = Vector2D(0, 0);
bool bhv_position::updatePos(PlayerAgent * agent){
	const WorldModel & wm = agent->world();

	vector<bhv_position::Position> posPosition;

	if (!canIpos(agent)) {
		return false;
	}

	if(last_cycle > 0&&target_pos.x<wm.offsideLineX()){
		last_cycle--;
		Arm_PointToPoint(target_pos).execute(agent);

		return Body_GoToPoint(target_pos, 0.2, 100).execute(agent);

	}

	posfor unum = whatTm(agent);

	if (unum.firstUnum > 0) {
//		//Arm_PointToPoint(wm.ourPlayer(unum.firstUnum)->pos()).execute(agent);
	} else {
//		Arm_Off().execute(agent);
		return false;
	}

	simulate_dash(agent, unum.firstUnum, unum.firstmaxD2pos, unum.firstmaxD2tar,
			posPosition);

	if (posPosition.size() == 0) {
		return false;
	}

	double evalmax = 0;
	int best = -1; //-1=not 0=last other=other
    for (size_t i = 0; i < posPosition.size(); i++) {
		double ev = posPosition[i].eval;
		if (ev > evalmax) {
			best = i;
			evalmax = ev;
		}
	}
	if (best == -1)
		return false;
	if (excute(agent, posPosition[best])) {
		ActionChainGraph::tmmate = 0;
		return true;
	}
	return false;
}

bool bhv_position::canIpos(PlayerAgent * agent){

	const WorldModel & wm = agent->world();

	int tmcycle = wm.interceptTable()->teammateReachCycle();
	int oppcycle = wm.interceptTable()->opponentReachCycle();
	int unum = wm.self().unum();
	int stamina = wm.self().stamina();
	double dist2target = Strategy::instance().getPosition(unum).dist(
			wm.self().pos());

	int maxSta = 3000;

	if (wm.self().unum() > 8) {
		if (wm.ball().pos().x > 30)
			maxSta = 2700;
		else if (wm.ball().pos().x > 10)
			maxSta = 3000;
		else if (wm.ball().pos().x > -30)
			maxSta = 3500;
		else if (wm.ball().pos().x > -55)
			maxSta = 4000;
	} else if (wm.self().unum() < 7) {
		if (wm.ball().pos().x > 30)
			maxSta = 5000;
		else if (wm.ball().pos().x > 10)
			maxSta = 4000;
		else if (wm.ball().pos().x > -30)
			maxSta = 3300;
		else if (wm.ball().pos().x > -55)
			maxSta = 2500;
	} else {
		if (wm.ball().pos().x > 30)
			maxSta = 3000;
		else if (wm.ball().pos().x > 10)
			maxSta = 3500;
		else if (wm.ball().pos().x > -30)
			maxSta = 3300;
		else if (wm.ball().pos().x > -55)
			maxSta = 2000;
	}

	if (oppcycle < tmcycle || stamina < maxSta || dist2target > 10) {
		if (debug)
			dlog.addText(Logger::POSITIONING,
					"can not for opp cycle or stamina or dist");
		return false;
	}

	if (oppcycle == tmcycle)
		if (unum < 6) {
			if (debug)
				dlog.addText(Logger::POSITIONING,
						"can not for opp cycle or stamina or dist def");
			return false;
		}

	if (wm.self().isFrozen()) {
		if (debug)
			dlog.addText(Logger::POSITIONING, "can not for frozen");
		return false;
	}

	return true;
}

bhv_position::posfor bhv_position::whatTm(rcsc::PlayerAgent *agent){

	const WorldModel & wm = agent->world();

	Vector2D myPos = wm.self().pos();
	int kicker = ActionChainGraph::tmmate;
	//wm.interceptTable()->fastestTeammate()->unum();
	int cycleTm = wm.interceptTable()->teammateReachCycle();
	Vector2D ball = wm.ball().inertiaPoint(cycleTm);
	bool ImTheNearest2Kicker = false;
	int counterForFor = 0;
	const PlayerPtrCont & tms = wm.teammatesFromBall();
	for (PlayerPtrCont::const_iterator it = tms.begin(); it != tms.end();
			++it) {
		if ((*it)->unum() < 1)
			continue;
		counterForFor++;
		if (counterForFor > 2)
			break;
		else {
			if (myPos.dist(ball) < (*it)->pos().dist(ball))
				ImTheNearest2Kicker = true;
			break;
		}
	}

	if (kicker == 0) {
		kicker = wm.interceptTable()->fastestTeammate()->unum();
		if (!ImTheNearest2Kicker)
			kicker = 0;
	}

	if (kicker == 0 && ball.x > 30) {
		const PlayerPtrCont & tms = wm.teammatesFromSelf();
		for (PlayerPtrCont::const_iterator it = tms.begin(); it != tms.end();
				++it) {
			if ((*it)->unum() < 1)
				continue;
			Vector2D myPos = wm.self().pos();
			Vector2D tmm = (*it)->pos();
			if (ball.y < myPos.y) {
				if (tmm.y < myPos.y) {
					kicker = (*it)->unum();
					break;
				}
			} else {
				if (tmm.y > myPos.y) {
					kicker = (*it)->unum();
					break;
				}
			}
		}
	}
	Vector2D ballpos = wm.ball().inertiaPoint(cycleTm);

	posfor ppp = posfor(kicker, leadPos, 5, 5);
	if (debug)
		dlog.addText(Logger::POSITIONING, "Positionig For %d", kicker);

	return ppp;
}

void bhv_position::simulate_dash(rcsc::PlayerAgent * agent, int tm,
		double maxD2pos, double maxD2tar,
		vector<bhv_position::Position> & posPosition){

	bool log = false;

	const WorldModel & wm = agent->world();

	if (tm <= 0)
		return;

	const AbstractPlayerObject * t = wm.ourPlayer(tm);
	if (!t || t->unum() < 1)
		return;

	int tmCycle = wm.interceptTable()->teammateReachCycle();

	Vector2D ball_pos = wm.ball().inertiaPoint(tmCycle);
	Vector2D self_pos = wm.self().inertiaFinalPoint();
	Vector2D str_pos = Strategy::instance().getPosition(wm.self().unum());
	Vector2D tm_pos = t->pos();
	Vector2D self_vel = wm.self().vel();

	AngleDeg self_body = wm.self().body().degree();

	const PlayerType * self_type = &(wm.self().playerType());

	double self_max_speed = self_type->realSpeedMax();
	double self_speed = self_vel.r();
	double offside_lineX = wm.offsideLineX();

	if (ball_pos.dist(self_pos) > 35)
		return;

	int MAX_DASH = 10;
	int ANGLE_DIVS = 10;
	double ANGLE_STEP = 360.0 / ANGLE_DIVS;

	for (int i = 0; i < ANGLE_DIVS; i++) {

		AngleDeg angle = i * ANGLE_STEP + self_body;

		int n_turn = predict_player_turn_cycle(self_type, self_body, self_speed,
				5, angle, 0, false);

		int n_dash = 0;

		Vector2D new_self_pos = self_pos;

		double Speed = (n_turn == 0 ? self_speed : 0);
		double accel = ServerParam::i().maxDashPower()
				* self_type->dashPowerRate() * self_type->effortMax();

		for (int n_step = n_turn; n_step < MAX_DASH; n_step++) {

			n_dash++;

			if (Speed + accel > self_max_speed) {
				accel = self_max_speed - Speed;
			}

			Speed += accel;

			new_self_pos += Vector2D::polar2vector(Speed, angle);

			Speed *= self_type->playerDecay();

			if (debug)
				dlog.addText(Logger::POSITIONING,
						"   angle(%.1f),n_turn(%d),n_dash(%d),target(%.1f,%.1f)",
						angle.degree(), n_turn, n_dash, new_self_pos.x,
						new_self_pos.y);

			if (new_self_pos.x > offside_lineX) {
				if (debug)
					dlog.addText(Logger::POSITIONING, "    over offside");
				continue;
			}

			double str_thr_dist = 10;
			if (wm.self().unum() == 11
					|| (wm.self().unum() >= 6 && wm.ball().pos().x > 0))
				str_thr_dist = 7;

			if (new_self_pos.dist(str_pos) > str_thr_dist)
				continue;

			double min_tm_dist =
					ServerParam::i().theirPenaltyArea().contains(new_self_pos) ?
							5 : 8;
			if (dist_near_tm(wm, new_self_pos) < min_tm_dist) {
				if (debug)
					dlog.addText(Logger::POSITIONING, "    near teammate(%.1f)",
							dist_near_tm(wm, new_self_pos));
				continue;
			}

			if (new_self_pos.absX() > 52 || new_self_pos.absY() > 31.5)
				continue;

			int opp_danger_cycle = wm.interceptTable()->opponentReachCycle();
			if ((opp_danger_cycle - tmCycle) < n_step - 2)
				continue;
			vector<passpos> passes;
			lead_pass_simulat(wm, tm_pos, new_self_pos, n_step, passes);

			if (passes.size() > 0) {

				double pos_eval = 0;
				Position newpos = Position(ball_pos, tm, new_self_pos, n_step,
						leadPos, true, true, pos_eval, passes);
				pos_eval = posEval(wm, newpos, agent);
				newpos.eval = pos_eval;
				posPosition.push_back(newpos);

			}
		}
	}
}

int bhv_position::predict_player_turn_cycle(const rcsc::PlayerType * ptype,
		const rcsc::AngleDeg & player_body, const double & player_speed,
		const double & target_dist, const rcsc::AngleDeg & target_angle,
		const double & dist_thr, const bool use_back_dash){
	const ServerParam & SP = ServerParam::i();

	int n_turn = 0;

	double angle_diff = (target_angle - player_body).abs();

	if (use_back_dash && target_dist < 5.0 // Magic Number
	&& angle_diff > 90.0 && SP.minDashPower() < -SP.maxDashPower() + 1.0) {
		angle_diff = std::fabs(angle_diff - 180.0);    // assume backward dash
	}

	double turn_margin = 180.0;
	if (dist_thr < target_dist) {
		turn_margin = std::max(15.0, // Magic Number
				rcsc::AngleDeg::asin_deg(dist_thr / target_dist));
	}

	double speed = player_speed;
	while (angle_diff > turn_margin) {
		angle_diff -= ptype->effectiveTurn(SP.maxMoment(), speed);
		speed *= ptype->playerDecay();
		++n_turn;
	}

	return n_turn;
}

double bhv_position::dist_near_tm(const WorldModel & wm, Vector2D point){

	double dist = 1000;
	const PlayerPtrCont::const_iterator o_end = wm.teammatesFromSelf().end();
	for (PlayerPtrCont::const_iterator o = wm.teammatesFromSelf().begin();
			o != o_end; ++o) {
		if ((*o)->unum() < 1)
			continue;
		if (!(*o)->pos().isValid())
			continue;
		if (dist > (*o)->pos().dist(point))
			dist = (*o)->pos().dist(point);
	}
	return dist;
}

void bhv_position::lead_pass_simulat(const WorldModel & wm, Vector2D passer_pos,
		Vector2D new_self_pos, int n_step, vector<passpos> & passes){

	Vector2D pass_start = wm.ball().pos();
	Vector2D current_self_pos = wm.self().pos();

	int tmCycle = wm.interceptTable()->teammateReachCycle();
	pass_start = wm.ball().inertiaPoint(tmCycle);

	AngleDeg self_dist_angle = (new_self_pos - pass_start).th() + 90;

	int dist_step = 2;

	double pass_speed = 3;
	double distance = new_self_pos.dist(pass_start);

	if (distance >= 20.0)
		pass_speed = 2.7;
	else if (distance >= 8.0)
		pass_speed = 2.5;
	else if (distance >= 5.0)
		pass_speed = 2.0;
	else
		pass_speed = 1.5;

	for (int i = -2; i <= 2; i++) {
		Vector2D pass_target = new_self_pos
				+ Vector2D::polar2vector(i * dist_step, self_dist_angle);
		double self_body = (new_self_pos - current_self_pos).th().degree();
		double self_speed = 0.3;
		int pass_cycle = self_cycle_intercept(wm, pass_start, pass_speed,
				pass_target, self_body, self_speed);
		int opps_cycle = opps_cycle_intercept(wm, pass_start, pass_speed,
				pass_cycle, pass_target);
		if (debug)
			dlog.addText(Logger::POSITIONING,
					"      pass_start(%.1f,%.1f),pass_target(%.1f,%.1f),self_cycle(%d),opp_cycle(%d)",
					pass_start.x, pass_start.y, pass_target.x, pass_target.y,
					pass_cycle, opps_cycle);

		if (pass_cycle < opps_cycle) {

			double pass_eval = SampleFieldEvaluator().evaluator(pass_target,
					wm.time().cycle());

			if (debug)
				dlog.addText(Logger::POSITIONING, "         eval(%.1f)",
						pass_eval);

			passpos directpass = passpos(pass_target, pass_speed, true,
					pass_eval, pass_cycle);

			passes.push_back(directpass);
		}
	}

}

int bhv_position::self_cycle_intercept(const WorldModel & wm,
		Vector2D pass_start, double pass_speed, Vector2D & pass_target,
		double self_body, double speed){

	const ServerParam & SP = ServerParam::i();

	AngleDeg pass_angle = (pass_target - pass_start).th();

	Vector2D pass_start_vel = Vector2D::polar2vector(pass_speed, pass_angle);

	const PlayerType * self_type = &(wm.self().playerType());

	for (int cycle = 1; cycle < 50; cycle++) {
		const Vector2D ball_pos = inertia_n_step_point(pass_start,
				pass_start_vel, cycle, SP.ballDecay());

		double dash_dist = ball_pos.dist(pass_target);
		dash_dist += 0.5;

		int n_turn = abs(pass_angle.degree() - self_body) > 15 ? 1 : 0;
		int n_dash = self_type->cyclesToReachDistance(dash_dist);

		int n_step = n_dash + n_turn;

		if (speed > 0.25 && n_turn == 0)
			n_step--;

		if (n_step <= cycle) {
			pass_target = ball_pos;
			return cycle;
		}
	}
	return 51;
}

int bhv_position::opps_cycle_intercept(const WorldModel & wm,
		Vector2D pass_start, double pass_speed, int pass_cycle,
		Vector2D pass_target){

	int min_cycle = 1000;

	const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
	for (PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
			o != o_end; ++o) {
		if ((*o)->unum() < 1)
			continue;
		int opp_cycle = opp_cycle_intercept(wm, *o, pass_start, pass_speed,
				pass_cycle, pass_target);

		if (min_cycle > opp_cycle)
			min_cycle = opp_cycle;

	}
	return min_cycle;

}

int bhv_position::opp_cycle_intercept(const WorldModel & wm,
		AbstractPlayerObject * opp, Vector2D pass_start, double pass_speed,
		int pass_cycle, Vector2D pass_target){

	const ServerParam & SP = ServerParam::i();

	AngleDeg pass_angle = (pass_target - pass_start).th();

	Vector2D pass_start_vel = Vector2D::polar2vector(pass_speed, pass_angle);
	Vector2D opp_pos = (*opp).pos();

	const PlayerType * opp_type = (*opp).playerTypePtr();

	for (int cycle = 1; cycle < 50; cycle++) {
		const Vector2D ball_pos = inertia_n_step_point(pass_start,
				pass_start_vel, cycle, SP.ballDecay());

		double dash_dist = ball_pos.dist(opp_pos);
		dash_dist -= 0.5;

		int n_dash = opp_type->cyclesToReachDistance(dash_dist) - 1;
		int n_step = n_dash;
		if (n_step <= cycle) {
			return cycle;
		}
	}
	return 50;
}

double bhv_position::posEval(const WorldModel & wm, Position pos,
		rcsc::PlayerAgent * agent){
	double sumEval = 0;
	double best_pass_eval = 0;
	double opp_eval = 10;
	for (int i = 0; i <= pos.passposvector.size(); i++) {

		if (best_pass_eval < pos.passposvector[i].eval)
			best_pass_eval = pos.passposvector[i].eval;

		sumEval += pos.passposvector[i].eval;
	}

	const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
	for (PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
			o != o_end; ++o) {
		if ((*o)->unum() < 1)
			continue;
		double opp_dist = (*o)->pos().dist(pos.target);
		if (opp_dist < opp_eval)
			opp_eval = opp_dist;

	}

	bool have_turn =
			((pos.target - wm.self().pos()).th() - wm.self().body()).abs()
					< 15 ? false : true;
	bool up_pos =
			wm.self().unum() >= 6
					&& (pos.target - wm.self().pos()).th().abs() < 60 ?
					true : false;
	sumEval /= pos.passposvector.size();
	sumEval += (sumEval * pos.passposvector.size() / 10);
	sumEval += best_pass_eval;
	sumEval += opp_eval;
	(!have_turn) ? sumEval += 10 : sumEval += 0;
	(up_pos) ? sumEval += 10 : sumEval += 0;
	//////////////Ehsan values/////////////

	(PointofAdjacency2Ball(agent, pos)) ? sumEval += 7.0 : sumEval += 0.0;
	(PointofExistenceInOppDanger(agent, pos)) ? sumEval += 7.0 : sumEval += 0.0;
	(PointofAdjacency2OppGoal(agent, pos)) ? sumEval += 5.0 : sumEval += 0.0;
	(PointofDistance2Me(agent, pos)) ? sumEval += 5.0 : sumEval += 0.0;
	(pointofBeingNearTm(agent, pos)) ? sumEval -= 20.0 : sumEval -= 0.0;
	sumEval += PointofAbsenceofOpps(agent, pos);

	///////////////End////////////////////
	return sumEval;
}

bool bhv_position::excute(PlayerAgent * agent, Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D ball = wm.ball().pos();
	Vector2D me = wm.self().pos();
	Vector2D homePos = Strategy::i().getPosition(wm.self().unum());
	const int self_min = wm.interceptTable()->selfReachCycle();
	const int mate_min = wm.interceptTable()->teammateReachCycle();
	const int opp_min = wm.interceptTable()->opponentReachCycle();
	const int our_min = std::min(self_min, mate_min);

	if (wm.self().unum() > 8 && ball.x < 40 && ball.x > -20
			&& (wm.self().unum() < 11 || ball.x < 48) && me.dist(homePos) < 10
			&& wm.self().pos().x < wm.offsideLineX() - 0.3
			&& // wm.self().pos().x > wm.offsideLineX() - 3.0 &&
			homePos.x > wm.offsideLineX() - 10.0 && wm.self().body().abs() < 15.0
			&& mate_min < opp_min && self_min > mate_min
			&& wm.self().stamina() > 4000) {
		agent->doDash(100, 0.0);
		target_pos = me+Vector2D(5,0);
		last_cycle = 3;
		agent->setNeckAction(new Neck_TurnToBallOrScan());
//		Arm_PointToPoint(me+Vector2D(10,0)).execute(agent);
		return true;
	}
	if (wm.self().unum() > 8 && ball.x < 40 && ball.x > -20
			&& (wm.self().unum() < 11 || ball.x < 48) && me.dist(homePos) < 10
			&& wm.self().pos().x < wm.offsideLineX() - 0.3
			&& // wm.self().pos().x > wm.offsideLineX() - 3.0 &&
			homePos.x > wm.offsideLineX() - 10.0 && mate_min < opp_min
			&& self_min > mate_min && wm.self().stamina() > 4000) {
		Body_TurnToAngle(0).execute(agent);
		last_cycle = 0;
		agent->setNeckAction(new Neck_TurnToBallOrScan());
		return true;
	}

	double thr = 0.5;
	if (agent->world().self().inertiaPoint(1).dist(Pos.target) < thr) {
//		//Arm_PointToPoint(agent->world().self().pos()+Vector2D::polar2vector(100,(Pos.ballpos-Pos.target).th()+80)).execute(agent);
		AngleDeg bestAngle = (Pos.ballpos - Pos.target).th() + 80;
		if (abs(bestAngle.degree()) > 90)
			bestAngle = (Pos.ballpos - Pos.target).th() - 80;
		last_cycle = 0;
		Body_TurnToAngle(bestAngle).execute(agent);
		return true;
	}
	Arm_PointToPoint(Pos.target).execute(agent);
	double enerji = (
			Pos.target.x > 30 ?
					100 : Strategy::get_normal_dash_power(agent->world()));
	if (Pos.target.x > 30)
		agent->addSayMessage(
				new SelfMessage(agent->world().self().pos(),
						agent->world().self().body(),
						agent->world().self().stamina()));

	if (Pos.target.x > 30)
		thr = 0.2;
	last_cycle = 3;
	target_pos = Pos.target;
	return Body_GoToPoint(Pos.target, thr, enerji).execute(agent);
}
bool bhv_position::PointofAdjacency2Ball(PlayerAgent * agent, Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D ball = wm.ball().pos();
	Vector2D myPos = wm.self().pos();
	Vector2D target = Pos.target;
	bool ImTheNearest2Kicker = false;
	int counterForFor = 0;
	const PlayerPtrCont & tms = wm.teammatesFromBall();
	for (PlayerPtrCont::const_iterator it = tms.begin(); it != tms.end();
			++it) {
		if ((*it)->unum() < 1)
			continue;
		counterForFor++;

		if ((*it) != NULL)
			continue;
		if (counterForFor < 2)
			continue;
		else if (counterForFor > 2)
			break;
		else {
			if (myPos.dist(ball) < (*it)->pos().dist(ball)
					&& (*it)->pos().isValid())
				ImTheNearest2Kicker = true;
		}
	}
	if (ImTheNearest2Kicker && target.dist(ball) < 8.0
			&& target.dist(ball) > 4.0)
		return true;
	return false;
}
bool bhv_position::PointofExistenceInOppDanger(PlayerAgent * agent,
		Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D target = Pos.target;
	if (target.x > 36.0 && target.absY() < 20.0)
		return true;
	return false;
}
bool bhv_position::PointofAdjacency2OppGoal(PlayerAgent * agent, Position Pos){
	Vector2D target = Pos.target;
	Vector2D oppGoal = Vector2D(52.5, 0.0);
	if (target.dist(oppGoal) < 10.0)
		return true;
	return false;
}
bool bhv_position::PointofDistance2Me(PlayerAgent * agent, Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D target = Pos.target;
	Vector2D myPos = wm.self().pos();
	if (myPos.dist(target) < 5.0 && myPos.dist(target) > 2.0)
		return true;
	return false;
}
double bhv_position::PointofAbsenceofOpps(PlayerAgent * agent, Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D target = Pos.target;
	Vector2D nearestOpp2Target;
	double dist = 100.0;
	const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
	for (PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
			o != o_end; ++o) {
		if ((*o)->unum() < 1)
			continue;
		if ((*o) != NULL) {
			if ((*o)->pos().dist(target) < dist && (*o)->pos().isValid()) {
				dist = (*o)->pos().dist(target);
				nearestOpp2Target = (*o)->pos();
			}
		}
	}
	if (dist == 100.0)
		return 0.0;
	return (dist * 2.0);
}
bool bhv_position::pointofBeingNearTm(PlayerAgent * agent, Position Pos){
	const WorldModel & wm = agent->world();
	Vector2D target = Pos.target;
	const PlayerPtrCont & tms = wm.teammatesFromBall();
	for (PlayerPtrCont::const_iterator it = tms.begin(); it != tms.end();
			++it) {
		if ((*it)->unum() < 1)
			continue;
		if ((*it) != NULL) {
			if ((*it)->pos().dist(target) < 4.0 && (*it)->pos().isValid())
				return true;
		}
	}
	return false;
}


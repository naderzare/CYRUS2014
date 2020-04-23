/*
 * bhv_cyrus_mark.cpp
 *
 *  Created on: Mar 17, 2014
 *      Author: nkpo
 */

#include "bhv_cyrus_mark.h"
#include "bhv_cyrus_block.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>

#include <rcsc/action/body_clear_ball.h>

#include <rcsc/geom/rect_2d.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>

#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/action/arm_point_to_point.h>
#include "neck_offensive_intercept_neck.h"
#include <iostream>
using namespace std;
using namespace rcsc;

bool bhv_cyrus_mark::execute(PlayerAgent * agent, vector<int> tmwork) {
	const WorldModel & wm = agent->world();
	if (wm.self().stamina() < 4500 && wm.self().unum() > 8) {
		tmwork[wm.self().unum()] = 1;
		return false;
	} else if (wm.self().stamina() < 3000 && wm.self().unum() > 5) {
		tmwork[wm.self().unum()] = 1;
		return false;
	} else if (wm.self().stamina() < 4500 && wm.self().unum() < 5) {
		tmwork[wm.self().unum()] = 1;
		return false;
	}
	////cout << " start set mark action " << endl;
	/*//////cout << wm.time().cycle()
			<< "*************************************************" << endl;
*/
	if (wm.self().unum() == 1 || wm.self().unum() == -1) {
		////cout << "out of mark" << endl;
		return false;

	}
	if (wm.self().stamina() < 2000)
		return false;

	//bhv_cyrus_block block_class = bhv_cyrus_block();
	//vector<bhv_block> block_vect = block_class.get_block_vector(agent);

	const PlayerPtrCont & opps = wm.opponentsFromBall();
	const PlayerObject * oppBall = wm.interceptTable()->fastestOpponent();
	/*(
	 opps.empty() ? static_cast<PlayerObject *>(0) : opps.front());*/
	// //cout << "   set_opp_evaluation(agent)" << endl;
	vector<double> oppev = set_opp_evaluation(agent);

	////cout << "   get_weight(agent, oppev)" << endl;
	vector<vector<double> > weights = get_weight(agent, oppev);
	vector<int> markedOpp(12, 0);
	////cout << "   mate opp weight" << endl;
/*	for (int i = 1; i < 12; i++) {

		for (int j = 1; j < 12; j++) {
			//cout << "      " << i << " " << j << " " << weights[i][j] << endl;

		}
		//cout << endl;
	}*/
	// //cout << "   get_danger_opp(agent, weights)" << endl;
	vector<vector<double> > danger_opp = get_danger_opp(agent, weights);
	if (danger_opp[0][0] == -1)
		return false;
	int opp_unum = get_best_opp(oppev);
	int tmm_unum = get_best_tmm(opp_unum, danger_opp, weights,
			wm.ball().inertiaPoint(wm.interceptTable()->opponentReachCycle()),
			tmwork);
	if (oppBall->unum() == opp_unum
			&& (tmm_unum == wm.self().unum()
					|| danger_opp[wm.self().unum()][0] == 0)) {
		/*	int tmblocker = block_class.decision(agent, tmwork);
		 oppev[opp_unum] -= 10;
		 if (wm.self().unum() == tmblocker && tmm_unum == tmblocker) {
		 return true;
		 }
		 tmm_unum = tmblocker;*/
		if (wm.self().unum() == tmm_unum)
			return false;
	}
	//cout << "tm" << tmm_unum << " mark opp" << opp_unum << endl;

	int counter = 10;
	while (tmm_unum != wm.self().unum() && counter < 20) {

		double last_opp_ev = oppev[opp_unum];

		oppev[opp_unum] = 1;
		if (wm.theirPlayer(opp_unum) != NULL && wm.ourPlayer(tmm_unum) != NULL
				&& ((wm.theirPlayer(opp_unum)->pos()
						!= wm.ourPlayer(tmm_unum)->pos())
						|| !(wm.theirPlayer(opp_unum)->pos().x
								- wm.ourPlayer(tmm_unum)->pos().x < 2.5
								&& wm.theirPlayer(opp_unum)->pos().x
										- wm.ourPlayer(tmm_unum)->pos().x > 0
								&& wm.ball().pos().x < -35)))
			markedOpp[opp_unum] += 1;
		danger_opp[tmm_unum] = vector<double>(3, -1000);
		weights[tmm_unum] = vector<double>(12, -1000);
		for (int i = 1; i < 12; i++) {
			//weights[i][opp_unum] = (weights[i][opp_unum] / last_opp_ev);
			//	 * oppev[opp_unum];
			weights[i][opp_unum] /= 2;
		}
		tmwork[tmm_unum] = 1;

		danger_opp = get_danger_opp(agent, weights);

		opp_unum = get_best_opp(oppev);
		tmm_unum = get_best_tmm(opp_unum, danger_opp, weights,
				wm.ball().inertiaPoint(
						wm.interceptTable()->opponentReachCycle()), tmwork);
		if (oppBall->unum() == opp_unum
				&& (tmm_unum == wm.self().unum()
						|| danger_opp[wm.self().unum()][0] == 0)) {
			/*	int tmblocker = block_class.decision(agent, tmwork);
			 oppev[opp_unum] -= 10;
			 if (wm.self().unum() == tmblocker) {
			 return true;
			 }

			 tmm_unum = tmblocker;*/
			if (wm.self().unum() == tmm_unum)
				return false;
		}

		//cout << "tm" << tmm_unum << " mark opp" << opp_unum << endl;
		if (wm.self().unum() == tmm_unum)
			break;
		counter++;
	}

	if (tmm_unum == wm.self().unum() && weights[tmm_unum][opp_unum] > 0) {
		Vector2D start_pass_pos = wm.ball().inertiaPoint(
				wm.interceptTable()->opponentReachCycle());
		const PlayerPtrCont & opps = wm.opponentsFromSelf();
		rcsc::Vector2D markPoint = Vector2D(-52.5, 0.0);
		Vector2D ball = wm.ball().inertiaPoint(
				wm.interceptTable()->opponentReachCycle());
		Vector2D ourGoal = Vector2D(-52.5, 0);
		bool search = false;
		const PlayerPtrCont & tms = wm.teammatesFromSelf();
		const PlayerPtrCont::const_iterator tms_end = tms.end();
		int myUnum = wm.self().unum();
		Vector2D home = Strategy::i().getPosition(wm.self().unum());

		Vector2D myPos = wm.self().pos();

		for (PlayerPtrCont::const_iterator it = opps.begin(); it != opps.end();
				++it) {
			if ((*it)->unum() < 1)
				continue;

			if ((*it)->unum() == opp_unum) {
				search = true;
				Vector2D oppPos = (*it)->pos() + (*it)->vel();
				int oppUnum = (*it)->unum();
				markPoint = oppPos;
				if (markedOpp[opp_unum] >= 1
						&& ((wm.ourDefenseLineX() - oppPos.x) > 1))
					break;

				markPoint = get_pass_cut_mark_point(agent, oppPos);
				bool offsideTrap = false;
				if ((wm.ourDefenseLineX() < home.x) && (oppPos.x < home.x)
						&& oppPos.x < wm.ourDefenseLineX() && ball.x < -10
						&& ball.x > wm.ourDefenseLineX()) {
					markPoint = Vector2D(wm.ourDefenseLineX(), markPoint.y);
					offsideTrap = true;
				}
				if (myUnum < 4 && abs(markPoint.y) > 8 && ball.x > 15) {
					/*//cout << "mark kon" << opp_unum << " 2shart" << tmm_unum
							<< "in" << markPoint << endl;*/
					return false;
				}
				if (!markPoint.isValid()) {

					return false;
				}

				if (wm.self().isFrozen()) {
					return false;
				}
				if (wm.self().isGhost()) {
					return false;
				}
				if (wm.self().isTackling()) {
					return false;
				}
				if (wm.self().stamina() < 5000 && wm.self().unum() > 8) {
					return false;
				} else if (wm.self().stamina() < 3000 && wm.self().unum() > 5) {
					return false;
				}
				/*markPoint = oppPos + ((*it)->vel())
				 - (Vector2D(1, 0) * (*it)->posCount()+1); // + (*it)->vel();*/
				////Arm_PointToPoint(markPoint).execute(agent);
				break;
			}

		}

		/*if (wm.self().unum() < 6 && markPoint.x > -35)
		 markPoint.y = Strategy::i().getPosition(wm.self().unum()).y;
		 */
		Arm_PointToPoint(markPoint).execute(agent);
		if (mark_move(agent, markPoint)) {
			/*//cout << "mark kon" << opp_unum << " mark move " << tmm_unum << "in"
					<< markPoint << endl;*/
		} else {
			double dist_thr = wm.ball().pos().dist(markPoint) / 20;
			Body_GoToPoint2010(markPoint, dist_thr, 100, 1.5, 5, false, 15).execute(
					agent);
		}

		agent->setNeckAction(
				new Neck_TurnToPlayerOrScan(wm.theirPlayer(opp_unum), 1));

		return true;

	}

	return false;
}

/*
 * cyrus mark get pos for finding cut mark position
 * @ PlayerAgent for finding wordmodel
 * @ opp_pos for finding mark poing
 * developed at 93/1/15 02:55
 * */

Vector2D bhv_cyrus_mark::get_pass_cut_mark_point(PlayerAgent * agent,
		Vector2D opp_pos) {
	const WorldModel & wm = agent->world();
	int my_unum = wm.self().unum();
	Vector2D ball = wm.ball().inertiaPoint(
			wm.interceptTable()->opponentReachCycle());
	Vector2D our_goal = Vector2D(-52.5, 0);
	Vector2D home = Strategy::i().getPosition(wm.self().unum());
	Vector2D my_pos = wm.self().pos();
	Vector2D markPoint = Vector2D(-52, 0);
	int min_dist = 1;
	int max_dist = 50;
	double dist_eval = min_dist
			+ ((opp_pos.dist(ball) > max_dist ? max_dist : opp_pos.dist(ball))
					/ (max_dist / 4.0));

	markPoint = opp_pos + (ball - opp_pos).setLengthVector(dist_eval);

	return markPoint;
}

vector<double> bhv_cyrus_mark::set_opp_evaluation(PlayerAgent *agent) {
	vector<double> oppev(12, -100);
	const WorldModel & wm = agent->world();

	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerObject * oppBall = wm.interceptTable()->fastestOpponent();

	for (PlayerPtrCont::const_iterator it = opps.begin(); it != opps.end();
			++it) {
		if ((*it)->unum() < 1)
			continue;

		if ((*it)->goalie() || (*it)->unum() == -1 || (*it)->unum() == 1) {
			oppev[1] = 0;
			continue;
		}
		Vector2D start_pass_pos = wm.ball().inertiaPoint(
				wm.interceptTable()->opponentReachCycle());
		/*if opponent can get a pass*/
		AngleDeg center_deg = (start_pass_pos - (*it)->pos()).dir();
		Sector2D tmp = Sector2D(start_pass_pos, 0,
				start_pass_pos.dist((*it)->pos()), center_deg - 30,
				center_deg + 30);
		bool ifpass = false;
		if (wm.existTeammateIn(tmp, 0, false)
				&& start_pass_pos.dist((*it)->pos()) < 40)
			ifpass = true;

		// if opponent dist to our offside line less than 5 meter can get through pass
		bool ifthrough_pass = false;
		if ((*it)->pos().x - wm.offsideLineX() < 7
				&& (*it)->pos().x - wm.offsideLineX() >= -0.5)
			ifthrough_pass = true;
		if (wm.offsideLineX() >= 0 && (*it)->pos().x < wm.offsideLineX())
			ifthrough_pass = true;
		Vector2D goal = Vector2D(-51, 0);
		Vector2D drible_vector =
				(Vector2D(-52.5, 0) - (*it)->pos()).setLengthVector(10);
		Vector2D target = (*it)->pos();
		if (oppBall->unum() == (*it)->unum())
			target += drible_vector;
		double eval = (100 - goal.dist(target)) * (ifpass ? 1.2 : 1)
				* (ifthrough_pass ? 1.3 : 1);

		// //cout << "    opp " << (*it)->unum() << " " << eval << "   " << ifpass
		//		<< " " << ifthrough_pass << endl;
		oppev[(*it)->unum()] = eval;

	}

	return oppev;
}

vector<vector<double> > bhv_cyrus_mark::get_weight(PlayerAgent * agent,
		vector<double> & oppev) {
	const WorldModel & wm = agent->world();
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont & tmms = wm.teammatesFromSelf();
	Vector2D ball = wm.ball().pos();
	vector<vector<double> > weights(12, vector<double>(12, -100));
	int base = 5;
	if (ball.x > 35.0) {
		return weights;
	}
	for (PlayerPtrCont::const_iterator it_tm = tmms.begin();
			it_tm != tmms.end(); ++it_tm) {
		Vector2D tmPos = (*it_tm)->pos() + (*it_tm)->vel();
		Vector2D tmHome = Strategy::i().getPosition((*it_tm)->unum());
		int tmUnum = (*it_tm)->unum();
		for (PlayerPtrCont::const_iterator it = opps.begin(); it != opps.end();
				++it) {
			if ((*it)->unum() < 1)
				continue;

			Vector2D oppPos = (*it)->pos() + (*it)->vel();
			int oppUnum = (*it)->unum();

			if ((*it)->unum() == -1 || (*it_tm)->unum() == -1)
				continue;
			if ((*it_tm)->isTackling())
				break;
			if (!wm.ball().pos().x < -35) {
				if (oppPos.dist(ball) < 1.0) {
					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;

				}
				if (oppPos.x > -30.0 && oppPos.dist(ball) < 5.0
						&& (tmUnum == 2 || tmUnum == 3 || tmUnum == 4
								|| tmUnum == 5)) {

					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
				if (tmHome.dist(oppPos) > 10
						&& tmHome.dist(Vector2D(oppPos.x, tmPos.y)) > 5.0
						&& !(tmUnum == 4 || tmUnum == 5)) {
					// //cout << "in shart maskhare!" << endl;
					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
				if (tmPos.dist(oppPos) > 15
						&& tmHome.dist(Vector2D(oppPos.x, tmPos.y)) > 6.0) {

					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
				// num 3 :
				if ((tmUnum == 2 || tmUnum == 3)
						&& (oppPos.dist(
								rcsc::Vector2D(wm.ourDefenseLineX(), oppPos.y))
								> 8) && (tmHome.dist(oppPos) > 10)) {
					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
				// num 4 :
				if (oppPos.dist(wm.ball().pos()) > 50.0) {
					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
				// num 5 :
				if (tmUnum == 4 && ball.y > 0.0 && oppPos.y < 0.0
						&& tmPos.y < 0.0 && ball.dist(oppPos) > 30.0) {
					weights[(*it_tm)->unum()][(*it)->unum()] = 0;
					break;
				}
			}
			if (wm.theirPlayer(oppUnum)->posCount() > 4) {

				weights[(*it_tm)->unum()][(*it)->unum()] = 0;
				continue;
			}

			double MPE = 15;
			double MHE = 10;

			double posEval = (MPE - tmPos.dist(oppPos)) * 3;
			double homeEval = (MHE - tmHome.dist(oppPos)) * 2;

			double distToGoalEval = 1;
			if (((int) (*it_tm)->unum() / 6) == 0)
				distToGoalEval *= 15;
			else if (((int) (*it_tm)->unum() / 10) == 0)
				distToGoalEval *= 5;
			else
				distToGoalEval *= 2;

			/*double frontEval = (int) (((*it)->pos().x + (52))
			 + ((*it_tm)->pos().x + (52)));
			 if (frontEval == 0 || frontEval == -1)
			 frontEval = 1.0;
			 else if (frontEval > -5 && frontEval < -1
			 && (*it_tm)->pos().dist((*it)->pos()) < 10)
			 frontEval = 2.0;
			 if ((frontEval == 1 || frontEval == 2)
			 && (*it_tm)->pos().dist((*it)->pos()) > 10)
			 frontEval = 5.0;
			 frontEval = 1.0 / frontEval;
			 int deltay = (int) (abs((*it_tm)->pos().y - (*it)->pos().y));
			 frontEval *= (1.0 / ((deltay == 0) ? 1.0 : deltay));

			 else if (frontEval > -5 && frontEval < -1
			 && (*it_tm)->pos().dist((*it)->pos()) < 10)
			 frontEval = 2;
			 if ((frontEval == 1 || frontEval == 2)
			 && (*it_tm)->pos().dist((*it)->pos()) > 10)
			 frontEval = 5;
			 frontEval = 20.0 * frontEval;*/
			AngleDeg opp2ball_angle = ((*it)->pos() - (*it_tm)->pos()).th();
			Vector2D opp_vel = (*it)->vel();
			//		int opp_cycle = wm.interceptTable()->opponentReachCycle();
			double frontEval = (int) ((*it)->pos().x - (*it_tm)->pos().x);
			if (frontEval == 0 || frontEval == -1)
				frontEval = 1.0;
			else if (frontEval > -5 && frontEval < -1
					&& (*it_tm)->pos().dist((*it)->pos()) < 10)
				frontEval = 2.0;
			if ((frontEval == 1 || frontEval == 2)
					&& (*it_tm)->pos().dist((*it)->pos()) > 10)
				frontEval = 5.0;
			frontEval = 1.0 / frontEval;
			int deltay = (int) (abs((*it_tm)->pos().y - (*it)->pos().y));
			frontEval *= (1.0 / ((deltay == 0) ? 1.0 : deltay));
			frontEval = 20.0 * frontEval;

			double deg_of_body = abs(opp2ball_angle.degree());

			/*	double dist_vel = opp_vel.r() * ((opp_cycle < 3 ? 3 : opp_cycle));*/

			frontEval = (frontEval) + (5 * (1 - (deg_of_body / 180)));
			/*+ (((*it)->pos().dist((*it_tm)->pos()) - dist_vel)
			 > 15 ?
			 15 :
			 ((*it)->pos().dist((*it_tm)->pos())
			 - dist_vel));*/
			if (posEval + homeEval < 0) {
				weights[(*it_tm)->unum()][(*it)->unum()] = 0;
			} else {
				int base = 3;
				switch (base) {
				case 1:
					weights[(*it_tm)->unum()][(*it)->unum()] = posEval * posEval
							* homeEval;
					break;
				case 2:
					weights[(*it_tm)->unum()][(*it)->unum()] = posEval * posEval
							+ homeEval;
					break;
				case 3:
					weights[(*it_tm)->unum()][(*it)->unum()] = (posEval
							+ homeEval
							+ ((frontEval > 0) ?
									(distToGoalEval * frontEval) :
									distToGoalEval)) / 2;
					break;
				case 4:
					weights[(*it_tm)->unum()][(*it)->unum()] = posEval
							* homeEval;
					break;
				case 5:
					weights[(*it_tm)->unum()][(*it)->unum()] = posEval
							+ homeEval * homeEval;
					break;
					break;
				default:
					break;
				}
				/*weights[(*it_tm)->unum()][(*it)->unum()] *=
				 oppev[(*it)->unum()];*/

			}

		}
	}
	Vector2D myPos = wm.self().pos() + wm.self().vel();
	Vector2D myHome = Strategy::i().getPosition(wm.self().unum());
	int myUnum = wm.self().unum();
	for (PlayerPtrCont::const_iterator it = opps.begin(); it != opps.end();
			++it) {
		if ((*it)->unum() < 1)
			continue;
		if (wm.self().isTackling())
			break;
		Vector2D oppPos = (*it)->pos() + (*it)->vel();
		int oppUnum = (*it)->unum();

		if (!wm.ball().pos().x < -35) {
			if (oppPos.dist(ball) < 1.0) {
				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			if (oppPos.x > -30.0 && oppPos.dist(ball) < 5.0
					&& (myUnum == 2 || myUnum == 3 || myUnum == 4 || myUnum == 5)) {

				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			if (myHome.dist(oppPos) > 10
					&& myHome.dist(Vector2D(oppPos.x, myPos.y)) > 5.0
					&& !(myUnum == 4 || myUnum == 5)) {

				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			if (myPos.dist(oppPos) > 15
					&& myHome.dist(Vector2D(oppPos.x, myPos.y)) > 6.0) {

				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			// num 3 :
			if ((myUnum == 2 || myUnum == 3)
					&& (oppPos.dist(
							rcsc::Vector2D(wm.ourDefenseLineX(), oppPos.y)) > 8)
					&& (myHome.dist(oppPos) > 10)) {
				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			// num 4 :
			if (oppPos.dist(wm.ball().pos()) > 50.0) {
				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
			// num 5 :
			if (myUnum == 4 && ball.y > 0.0 && oppPos.y < 0.0 && myPos.y < 0.0
					&& ball.dist(oppPos) > 30.0) {
				weights[wm.self().unum()][(*it)->unum()] = 0;
				break;
			}
		}
		if (wm.theirPlayer(oppUnum)->posCount() > 4) {

			weights[wm.self().unum()][(*it)->unum()] = 0;
			continue;
		}

		double MPE = 15;
		double MHE = 10;

		double posEval = (MPE - myPos.dist(oppPos)) * 3;
		double homeEval = (MHE - myHome.dist(oppPos)) * 2;
		double distToGoalEval = 1;
		if ((int) (wm.self().unum() / 6) == 0)
			distToGoalEval *= 15;
		else if ((int) (wm.self().unum() / 10) == 0)
			distToGoalEval *= 5;
		else
			distToGoalEval *= 2;

		/*double frontEval = (int) (((*it)->pos().x + (52))
		 + (wm.self().pos().x + (52)));
		 if (frontEval == 0 || frontEval == -1)
		 frontEval = 1.0;
		 else if (frontEval > -5 && frontEval < -1
		 && wm.self().pos().dist((*it)->pos()) < 10)
		 frontEval = 2.0;
		 if ((frontEval == 1 || frontEval == 2)
		 && wm.self().pos().dist((*it)->pos()) > 10)
		 frontEval = 5.0;
		 frontEval = 1.0 / frontEval;
		 int deltay = (int) (abs(wm.self().pos().y - (*it)->pos().y));
		 frontEval *= (1.0 / ((deltay == 0) ? 1.0 : deltay));
		 frontEval = 20.0 * frontEval;*/
		AngleDeg opp2ball_angle = ((*it)->pos() - wm.self().pos()).th();
		Vector2D opp_vel = (*it)->vel();
		//	int opp_cycle = (*it)->pos().dist(ball) / (*it)->vel().r();
		double frontEval = (int) ((*it)->pos().x - wm.self().pos().x);
		if (frontEval == 0 || frontEval == -1)
			frontEval = 1.0;
		else if (frontEval > -5 && frontEval < -1
				&& wm.self().pos().dist((*it)->pos()) < 10)
			frontEval = 2.0;
		if ((frontEval == 1 || frontEval == 2)
				&& wm.self().pos().dist((*it)->pos()) > 10)
			frontEval = 5.0;
		frontEval = 1.0 / frontEval;
		int deltay = (int) (abs(wm.self().pos().y - (*it)->pos().y));
		frontEval *= (1.0 / ((deltay == 0) ? 1.0 : deltay));
		frontEval = 20.0 * frontEval;

		double deg_of_body = abs(opp2ball_angle.degree());

		//double dist_vel = opp_vel.r() * ((opp_cycle < 3 ? 3 : opp_cycle));

		frontEval = (frontEval) + (5 * (1 - (deg_of_body / 180)));
		/*+ (((*it)->pos().dist(wm.self().pos()) - dist_vel) > 15 ?
		 15 : ((*it)->pos().dist(wm.self().pos()) - dist_vel));*/
		if (posEval + homeEval < 0) {
			weights[wm.self().unum()][(*it)->unum()] = 0;

		} else {
			int base = 3;
			switch (base) {
			case 1:
				weights[wm.self().unum()][(*it)->unum()] = posEval * posEval
						* homeEval;
				break;
			case 2:
				weights[wm.self().unum()][(*it)->unum()] = posEval * posEval
						+ homeEval;
				break;
			case 3:
				weights[wm.self().unum()][(*it)->unum()] = (posEval + homeEval
						+ ((frontEval > 0) ?
								(distToGoalEval * frontEval) : distToGoalEval))
						/ 2;
				break;
			case 4:
				weights[wm.self().unum()][(*it)->unum()] = posEval * homeEval;
				break;
			case 5:
				weights[wm.self().unum()][(*it)->unum()] = posEval
						+ homeEval * homeEval;
				break;
				break;
			default:
				break;
			}

			/*weights[wm.self().unum()][(*it)->unum()] *= oppev[(*it)->unum()];*/
		}

	}
	return weights;
}

vector<vector<double> > bhv_cyrus_mark::get_danger_opp(PlayerAgent * agent,
		vector<vector<double> > &weights) {
	const WorldModel & wm = agent->world();
	const PlayerPtrCont & tmms = wm.teammatesFromBall();
	vector<vector<double> > tmp_weights = weights;
	vector<vector<double> > danger_opps_weight(12, vector<double>(3, -100));
	vector<vector<double> > danger_opps_num(12, vector<double>(3, -100));
// //cout << wm.time().cycle() << "here" << endl;
	bool should_check_mark = false;
	for (int j = 1; j < 12; j++) {
		// get maximum weight for each team mate and get opp unum
		for (int counter_1 = 0; counter_1 < 3; counter_1++) {
			danger_opps_num[j][counter_1] = 2;
			danger_opps_weight[j][counter_1] = tmp_weights[j][2];
			if (tmp_weights[j][2] > 0)
				should_check_mark = true;
			for (int counter_2 = 3; counter_2 < 12; counter_2++) {
				if (tmp_weights[j][counter_2]
						> danger_opps_weight[j][counter_1]) {
					danger_opps_weight[j][counter_1] =
							tmp_weights[j][counter_2];
					if (tmp_weights[j][counter_2] > 0)
						should_check_mark = true;
					danger_opps_num[j][counter_1] = counter_2;
				}

			}
			tmp_weights[j][danger_opps_num[j][counter_1]] = -1000;
		}

	}

	/*for (int i = 1; i < 12; i++) {
		// //cout << " tm " << i << " ";
		for (int j = 0; j < 3; j++) {
			// //cout << danger_opps_num[i][j] << ":" << danger_opps_weight[i][j]
			//	<< "  ";
		}
		// //cout << endl;
	}*/
	if (!should_check_mark)
		vector<vector<double> > danger_opps_num(12, vector<double>(3, -1));

	return danger_opps_num;
}

int bhv_cyrus_mark::get_best_opp(vector<double> & oppev) {

	int best_opp = 1;

	for (int i = 2; i < 12; i++) {
		if (oppev[best_opp] < oppev[i])
			best_opp = i;
	}

	return best_opp;

}

int bhv_cyrus_mark::get_best_tmm(int opp_unum,
		vector<vector<double> > & danger_opp, vector<vector<double> > & weights,
		Vector2D ball, vector<int> tmwork) {
	int tmm_num = 0;
	int max_weight = 0;
	for (int i = 2; i < 12; i++) {
		for (int j = 0; j < 3; j++) {
			if (danger_opp[i][j] == opp_unum && tmwork[i] != 1
					&& (int) (max_weight)
							< (int) (weights[i][danger_opp[i][j]])) {
				tmm_num = i;
				max_weight = weights[i][danger_opp[i][j]];

			}
		}
	}
	return tmm_num;
}
bool bhv_cyrus_mark::mark_move(rcsc::PlayerAgent * agent, Vector2D target) {

	const WorldModel & wm = agent->world();
	Vector2D self_pos = wm.self().pos();
	Vector2D self_vel = wm.self().vel();
	Vector2D start_pos = wm.self().inertiaFinalPoint();

	double self_vel_r = self_vel.r();
	double self_vel_dir = self_vel.th().degree();
	double body_dir = wm.self().body().degree();
	double target_dir = (target - start_pos).th().degree();
	double target_dist = target.dist(start_pos);

	double move_dir = (target - start_pos).th().degree();
	double dif_dir = abs(body_dir - move_dir);
	if (dif_dir < 0)
		dif_dir *= (-1);
	if (dif_dir > 180)
		dif_dir = 360 - dif_dir;
	bool back_move = dif_dir > 90 ? true : false;
	if (target_dist < 1) {
		// //cout << "A" << endl;
		return Body_GoToPoint2010(target, 0.1, 100, 1, 1, false, 10).execute(
				agent);
	} else if (target_dist < 2) {
		if (back_move) {
			double move_body_dir = move_dir - body_dir;
			if (move_body_dir > 180)
				move_body_dir = -(360 - move_body_dir);
			if (move_body_dir < -180)
				move_body_dir = (360 + move_body_dir);
			move_body_dir += 180;
			if (move_body_dir > 180)
				move_body_dir = -(360 - move_body_dir);
			if (move_body_dir < -180)
				move_body_dir = (360 + move_body_dir);
			// //cout << "B" << endl;
			return agent->doDash(-200, move_body_dir);

		} else {
			double move_body_dir = move_dir - body_dir;
			if (move_body_dir > 180)
				move_body_dir = -(360 - move_body_dir);
			if (move_body_dir < -180)
				move_body_dir = (360 + move_body_dir);
			// //cout << "C" << endl;
			return agent->doDash(100, move_body_dir);
		}
	} else if (target_dist < 4) {
		if (back_move) {
			double temp = abs(body_dir - move_dir);
			if ((temp > 174 && temp < 186) || (temp < -174 && temp > -186)) {
				double move_body_dir = move_dir - body_dir;
				if (move_body_dir > 180)
					move_body_dir = -(360 - move_body_dir);
				if (move_body_dir < -180)
					move_body_dir = (360 + move_body_dir);
				move_body_dir += 180;
				if (move_body_dir > 180)
					move_body_dir = -(360 - move_body_dir);
				if (move_body_dir < -180)
					move_body_dir = (360 + move_body_dir);
				return agent->doDash(-200, move_body_dir);
				// //cout << "D" << endl;
			}
		} else {
			if (self_vel_r > 0.3 && abs(self_vel_dir - target_dir) < 10
					&& abs(body_dir - target_dir) < 15) {
				double move_body_dir = move_dir - body_dir;
				if (move_body_dir > 180)
					move_body_dir = -(360 - move_body_dir);
				if (move_body_dir < -180)
					move_body_dir = (360 + move_body_dir);
				// //cout << "E" << endl;
				return agent->doDash(100, move_body_dir);
			} else if (abs(self_vel_dir - target_dir) < 10
					&& abs(body_dir - target_dir) < 10) {
				double move_body_dir = move_dir - body_dir;
				if (move_body_dir > 180)
					move_body_dir = -(360 - move_body_dir);
				if (move_body_dir < -180)
					move_body_dir = (360 + move_body_dir);
				// //cout << "F" << endl;
				return agent->doDash(100, move_body_dir);
			}
		}
	}

	if (target_dist < 10) {
		// //cout << "G" << endl;
		return Body_GoToPoint2010(target, 1, 100, 1, 1, false, 20).execute(
				agent);
	} else {
		// //cout << "H" << endl;
		return Body_GoToPoint2010(target, 1, 100, 1, 1, false, 15).execute(
				agent);
	}
}

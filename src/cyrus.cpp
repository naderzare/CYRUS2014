/*
 * cyrus.cpp
 *
 *  Created on: Apr 27, 2013
 *      Author: 007
 */

#include "cyrus.h"
#include <rcsc/common/server_param.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/math_util.h>
#include <iostream>
#include <rcsc/player/intercept_table.h>

#include "chain_action/action_state_pair.h"
#include "chain_action/action_chain_graph.h"
using namespace std;
using namespace rcsc;
cyrus::cyrus() {
	// TODO Auto-generated constructor stub
}

cyrus::~cyrus() {
	// TODO Auto-generated destructor stub
}

int cyrus::cyclesToReachToPoint(const WorldModel& wm,
		const AbstractPlayerObject * p, Vector2D target, double distThr) {

	const PlayerType * pt = p->playerTypePtr();
	const ServerParam & SP = ServerParam::i();
	StaminaModel stamina_model;
	stamina_model.init(*pt);

	Vector2D playerPos = p->pos();
	Vector2D playerSpeedVec = p->vel();
	Vector2D target_relative = target - playerPos;
	AngleDeg target_angle = target_relative.th();
	AngleDeg player_body = p->body();
	double speed = playerSpeedVec.r();
	double dash_power = SP.maxDashPower();
	double target_dist = target.dist(playerPos);
	double accel = SP.maxDashPower() * pt->dashPowerRate() * pt->effortMax();
	Vector2D accelVec = Vector2D::polar2vector(accel, target_angle);

	//turn
	int n_turn = 0;

	double angle_diff = (target_angle - player_body).abs();

	if (target_dist < 5.0 // Magic Number
	&& angle_diff > 90.0 && SP.minDashPower() < -SP.maxDashPower() + 1.0) {
		angle_diff = std::fabs(angle_diff - 180.0);    // assume backward dash
	}

	double turn_margin = 180.0;
	if (0.5 < target_dist) {
		turn_margin = std::max(15.0, // Magic Number
				rcsc::AngleDeg::asin_deg(0.5 / target_dist));
	}
	if (playerPos.dist(target) <= distThr)
		return 0;
	while (angle_diff > turn_margin) {
		angle_diff -= pt->effectiveTurn(SP.maxMoment(), speed);
		speed *= pt->playerDecay();
		playerSpeedVec *= pt->playerDecay();
		playerPos += Vector2D::polar2vector(speed, player_body);
		stamina_model.simulateWait(*pt);
		++n_turn;
	}

	for (int counter = n_turn; counter <= 50; ++counter) {
		if ((playerSpeedVec + accelVec).r() > pt->playerSpeedMax()) {
			playerSpeedVec = Vector2D::polar2vector(pt->playerSpeedMax(),
					target_angle);
			dash_power = std::min(SP.maxDashPower(),
					accel / (pt->dashPowerRate() * stamina_model.effort()));
		} else {
			dash_power = SP.maxDashPower();
			playerSpeedVec += accelVec;
		}

		playerPos += playerSpeedVec;

		if (playerPos.dist(target) <= distThr)
			return counter;

		playerSpeedVec *= pt->playerDecay();
		stamina_model.simulateDash(*pt, dash_power);
		if (stamina_model.stamina() <= 0.0) {
			//return 1000;
		}
	}
}
int cyrus::state_confidence(const PredictState & wm) {

    double eval = -1000.0;
	Vector2D ball_pos = wm.ball().pos();
	Vector2D self_pos = wm.self().pos();
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();

	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp) == NULL || (*opp)->unum() < 1)
			continue;
		double opp_eval = 0;
		Vector2D opp_pos = (*opp)->pos() + (*opp)->vel();
		Vector2D opp_vel = (*opp)->vel();
		AngleDeg opp2ball_angle = (opp_pos - ball_pos).th();
		int opp_count = (*opp)->posCount();
        int opp_cycle = static_cast<int>(opp_pos.dist(ball_pos) / (*opp)->vel().r());

		if (opp_count > 10)
			continue;

        double frontEval = static_cast<int> (opp_pos.x - self_pos.x);
        if (abs(frontEval) < 0.1)
			frontEval = 1.0;
		else if (frontEval > -5 && frontEval < -1
				&& self_pos.dist(opp_pos) < 10)
			frontEval = 2.0;
        if (( abs(frontEval-1.0) < 0.1 || abs(frontEval-2.0) < 0.1) && self_pos.dist(opp_pos) > 10)
			frontEval = 5.0;
		frontEval = 1.0 / frontEval;
        int deltay = static_cast<int> (abs(self_pos.y - opp_pos.y));
		frontEval *= (1.0 / ((deltay == 0) ? 1.0 : deltay));
		frontEval = 20.0 * frontEval;

		double deg_of_body = abs(opp2ball_angle.degree());

		double dist_vel = opp_vel.r() * ((opp_cycle < 3 ? 3 : opp_cycle));

		opp_eval = (20 - frontEval) + (5 * (deg_of_body / 180))
				+ ((opp_pos.dist(ball_pos) - dist_vel) > 15 ?
						15 : (opp_pos.dist(ball_pos) - dist_vel));

		if (eval < opp_eval)
			eval = opp_eval;

	}
    return static_cast<int>(eval);

}
int cyrus::blockme(const WorldModel & wm) {

	if (!wm.self().isKickable())
		return 0;
//	cout<<"*************"<<"cycle"<<wm.time().cycle()<<"me:"<<wm.self().unum()<<"************"<<endl;

	vector<double> eval;
	Vector2D ball_pos = wm.ball().pos();
	Vector2D self_pos = wm.self().pos();
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();
	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp)->unum() < 1)
			continue;
		double opp_eval = 0;
		Vector2D opp_pos = (*opp)->pos() + (*opp)->vel();
		Vector2D opp_vel = (*opp)->vel();
		AngleDeg opp_body = (*opp)->body();
		AngleDeg opp2ball_angle = (ball_pos - opp_pos).th();
		int opp_count = (*opp)->posCount();
		int opp_cycle = wm.interceptTable()->opponentReachCycle();

		if (opp_pos.dist(ball_pos) > 6)
			continue;
		if (opp_pos.x < ball_pos.x - 10)
			continue;
		if (opp_pos.dist(Vector2D(52,0))>wm.ball().pos().dist(Vector2D(52,0))+5
				&&opp_pos.x < ball_pos.x)
			continue;
		if (opp_count <= 3 && min((opp2ball_angle - opp_vel.th()).abs(),360-(opp2ball_angle - opp_vel.th()).abs()) > 60
				&& opp_pos.dist(ball_pos) > 5)
			continue;

			if (opp_count <= 3 && min((opp_body - opp2ball_angle).abs(),360-(opp_body- opp2ball_angle).abs()) > 60
					&& opp_pos.dist(ball_pos) > 5)
				continue;

		double opp_eval_dist;
		double opp_eval_body_angle;
		double opp_eval_vel_angle;
		double opp_eval_vel;
		double opp_eval_count;
		double opp_eval_between;
		double opp_eval_ballx;
		double opp_eval_cycle;

		double opp_z_dist = 4.0;
		double opp_z_body_angle = 0.5;
		double opp_z_vel_angle = 0.75;
		double opp_z_vel = 1.0;
		double opp_z_count = 0.5;
		double opp_z_between = 0.25;
		double opp_z_ballx = 0.25;
		double opp_z_cycle = 4.0;
		double sum_z = opp_z_dist + opp_z_body_angle + opp_z_vel_angle
				+ opp_z_vel + opp_z_count + opp_z_between + opp_z_ballx
				+ opp_z_cycle;
		if (opp_pos.dist(ball_pos) < 3)
			opp_eval_dist = 10;
		else if (opp_pos.dist(ball_pos) < 4)
			opp_eval_dist = 8;
		else if (opp_pos.dist(ball_pos) < 5)
			opp_eval_dist = 4;
		else
			opp_eval_dist = ((12 - opp_pos.dist(ball_pos)) / 24 * 10)/2; // dist

		if(opp_eval_dist<0)
			opp_eval_dist=0;

		if ((opp2ball_angle - opp_body).abs() < 15)
			opp_eval_body_angle = 10;
		else
			opp_eval_body_angle = (((opp2ball_angle - opp_body).abs() - 15)
					/ 180 * 10); // body angle
		if(opp_eval_body_angle<0)
			opp_eval_body_angle=0;


        if ((opp2ball_angle - opp_vel.th()).abs() < 80 && opp_vel.r()>0.3){
			if ((opp2ball_angle - opp_vel.th()).abs() < 15)
				opp_eval_vel = 10;
			else
				opp_eval_vel_angle = (((opp2ball_angle - opp_vel.th()).abs()
						- 15) / 180 * 10); // vel angle
        }
		if(opp_eval_vel_angle<0)
			opp_eval_vel_angle=0;

		if ((opp2ball_angle - opp_vel.th()).abs() > 60)
			opp_eval_vel = 0;
		else
			opp_eval_vel = (opp_vel.r() / 1.0 * 10); // vel

		opp_eval_count = bound(0, (*opp)->posCount(), 10); // count

		opp_eval_between = (((opp_pos - self_pos).th()
				- (ball_pos - self_pos).th()).abs() / 180 * 10); // ball between

		opp_eval_ballx = 0;//((105 - (ball_pos.x + 52.5)) / 105 * 10); // ball x

		opp_eval_cycle = (6 - bound(0, opp_cycle, 6)); // cycle intercept
		if(opp_cycle>2)
			opp_eval_cycle/=2.0;
		if(opp_cycle>4)
			opp_eval_cycle/=2.0;


		opp_eval_dist *= opp_z_dist;
		opp_eval_body_angle *= opp_z_body_angle;
		opp_eval_vel_angle *= opp_z_vel_angle;
		opp_eval_vel *= opp_z_vel;
		opp_eval_count *= opp_z_count;
		opp_eval_between *= opp_z_between;
		opp_eval_ballx *= opp_z_ballx;
		opp_eval_cycle *= opp_z_cycle;

//		cout<<"---------------------"
//				<<(*opp)->unum()<<"dist"<<(int)opp_eval_dist
//				<<" body:"<<(int)opp_eval_body_angle
//				<<" velan:"<<(int)opp_eval_vel_angle
//				<<" vel:"<<(int)opp_eval_vel
//				<<" count:"<<(int)opp_eval_count
//				<<" bet:"<<(int)opp_eval_between
//				<<" ballx:"<<(int)opp_eval_ballx
//				<<" cycle:"<<(int)opp_eval_cycle
//				<<endl;

		opp_eval = opp_eval_ballx + opp_eval_between + opp_eval_body_angle
				+ opp_eval_count + opp_eval_dist + opp_eval_vel
				+ opp_eval_vel_angle + opp_eval_cycle;
//		cout<<"---------------------"<<(*opp)->unum()<<":"<<(int)opp_eval<<endl;
		eval.push_back(opp_eval / sum_z);
	}
	double max_eval = 0;
	int max = 0;
    for (size_t i = 0; i < eval.size(); i++) {
		if (max_eval < eval[i]) {
			max_eval = eval[i];
			max = i;
		}
	}
	double max_eval_2 = 0;
    for (size_t i = 0; i < eval.size(); i++) {
		if (i == max)
			continue;
		if (max_eval_2 < eval[i]) {
			max_eval_2 = eval[i];
		}
	}

	double reval = max_eval + max_eval_2 / 4;
	if(reval>10)reval=10;
	//cout<<"****s"<<wm.time().cycle()<<wm.self().unum()<<" "<<wm.ball().pos()<<" "<<reval<<endl;

//	cout<<"----"<<"me:"<<wm.self().unum()<<"-----------------"<<(int)max_eval<<" "<<(int)max_eval_2<<" **"<<(int)(max_eval + max_eval_2 / 2)<<endl;
    return static_cast<int>(reval);
}

int cyrus::block_ev(const PredictState & wm) {

//	if ( !wm.ballHolder()-> )return 0;
//	cout<<"*************"<<"cycle"<<wm.time().cycle()<<"me:"<<wm.self().unum()<<"************"<<endl;

	vector<double> eval;
	Vector2D ball_pos = wm.ball().pos();
	Vector2D self_pos = wm.ballHolder()->pos();
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();
	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp)->unum() < 1)
			continue;
		double opp_eval = 0;
		Vector2D opp_pos = (*opp)->pos() + (*opp)->vel();
		Vector2D opp_vel = (*opp)->vel();
		AngleDeg opp_body = (*opp)->body();
		AngleDeg opp2ball_angle = (ball_pos - opp_pos).th();
		int opp_count = (*opp)->posCount();
		int opp_cycle = opp_pos.dist(ball_pos)/(*opp)->playerTypePtr()->realSpeedMax();

		if (opp_pos.dist(ball_pos) > 6)
			continue;
		if (opp_pos.x < ball_pos.x - 10)
			continue;
		if (opp_pos.dist(Vector2D(52,0))>wm.ball().pos().dist(Vector2D(52,0))+5
				&&opp_pos.x < ball_pos.x)
			continue;
		if (opp_count <= 3 && min((opp2ball_angle - opp_vel.th()).abs(),360-(opp2ball_angle - opp_vel.th()).abs()) > 60
				&& opp_pos.dist(ball_pos) > 5)
			continue;

			if (opp_count <= 3 && min((opp_body - opp2ball_angle).abs(),360-(opp_body- opp2ball_angle).abs()) > 60
					&& opp_pos.dist(ball_pos) > 5)
				continue;
		double opp_eval_dist;
		double opp_eval_body_angle;
		double opp_eval_vel_angle;
		double opp_eval_vel;
		double opp_eval_count;
		double opp_eval_between;
		double opp_eval_ballx;
		double opp_eval_cycle;

		double opp_z_dist = 4.0;
		double opp_z_body_angle = 0.5;
		double opp_z_vel_angle = 0.75;
		double opp_z_vel = 1.0;
		double opp_z_count = 0.5;
		double opp_z_between = 0.25;
		double opp_z_ballx = 0.25;
		double opp_z_cycle = 4.0;
		double sum_z = opp_z_dist + opp_z_body_angle + opp_z_vel_angle
				+ opp_z_vel + opp_z_count + opp_z_between + opp_z_ballx
				+ opp_z_cycle;
		if (opp_pos.dist(ball_pos) < 3)
			opp_eval_dist = 10;
		else if (opp_pos.dist(ball_pos) < 4)
			opp_eval_dist = 8;
		else if (opp_pos.dist(ball_pos) < 5)
			opp_eval_dist = 4;
		else
			opp_eval_dist = ((12 - opp_pos.dist(ball_pos)) / 24 * 10)/2; // dist

		if(opp_eval_dist<0)
			opp_eval_dist=0;

		if ((opp2ball_angle - opp_body).abs() < 15)
			opp_eval_body_angle = 10;
		else
			opp_eval_body_angle = (((opp2ball_angle - opp_body).abs() - 15)
					/ 180 * 10); // body angle
		if(opp_eval_body_angle<0)
			opp_eval_body_angle=0;


		if ((opp2ball_angle - opp_vel.th()).abs() < 80 && opp_vel.r()>0.3)
			if ((opp2ball_angle - opp_vel.th()).abs() < 15)
				opp_eval_vel = 10;
			else
				opp_eval_vel_angle = (((opp2ball_angle - opp_vel.th()).abs()
						- 15) / 180 * 10); // vel angle
		if(opp_eval_vel_angle<0)
			opp_eval_vel_angle=0;

		if ((opp2ball_angle - opp_vel.th()).abs() > 60)
			opp_eval_vel = 0;
		else
			opp_eval_vel = (opp_vel.r() / 1.0 * 10); // vel

		opp_eval_count = bound(0, (*opp)->posCount(), 10); // count

		opp_eval_between = (((opp_pos - self_pos).th()
				- (ball_pos - self_pos).th()).abs() / 180 * 10); // ball between

		opp_eval_ballx = 0;//((105 - (ball_pos.x + 52.5)) / 105 * 10); // ball x

		opp_eval_cycle = (6 - bound(0, opp_cycle, 6)); // cycle intercept
		if(opp_cycle>2)
			opp_eval_cycle/=2.0;
		if(opp_cycle>4)
			opp_eval_cycle/=2.0;

		opp_eval_dist *= opp_z_dist;
		opp_eval_body_angle *= opp_z_body_angle;
		opp_eval_vel_angle *= opp_z_vel_angle;
		opp_eval_vel *= opp_z_vel;
		opp_eval_count *= opp_z_count;
		opp_eval_between *= opp_z_between;
		opp_eval_ballx *= opp_z_ballx;
		opp_eval_cycle *= opp_z_cycle;

//		cout<<"---------------------"
//				<<(*opp)->unum()<<"dist"<<(int)opp_eval_dist
//				<<" body:"<<(int)opp_eval_body_angle
//				<<" velan:"<<(int)opp_eval_vel_angle
//				<<" vel:"<<(int)opp_eval_vel
//				<<" count:"<<(int)opp_eval_count
//				<<" bet:"<<(int)opp_eval_between
//				<<" ballx:"<<(int)opp_eval_ballx
//				<<" cycle:"<<(int)opp_eval_cycle
//				<<endl;

		opp_eval = opp_eval_ballx + opp_eval_between + opp_eval_body_angle
				+ opp_eval_count + opp_eval_dist + opp_eval_vel
				+ opp_eval_vel_angle + opp_eval_cycle;
//		cout<<"---------------------"<<(*opp)->unum()<<":"<<(int)opp_eval<<endl;
		eval.push_back(opp_eval / sum_z);
	}
	double max_eval = 0;
	int max = 0;
	for (int i = 0; i < eval.size(); i++) {
		if (max_eval < eval[i]) {
			max_eval = eval[i];
			max = i;
		}
	}
	double max_eval_2 = 0;
	for (int i = 0; i < eval.size(); i++) {
		if (i == max)
			continue;
		if (max_eval_2 < eval[i]) {
			max_eval_2 = eval[i];
		}
	}
	double reval = max_eval + max_eval_2 / 4;
	if(reval>10)reval=10;
	//cout<<"****"<<wm.currentTime().cycle()<<wm.self().unum()<<" "<<wm.ballHolderUnum()<<" "<<wm.ball().pos()<<" "<<reval<<endl;
//	cout<<"----"<<"me:"<<wm.self().unum()<<"-----------------"<<(int)max_eval<<" "<<(int)max_eval_2<<" **"<<(int)(max_eval + max_eval_2 / 2)<<endl;
	return reval;
}
double cyrus::block_drible(const PredictState & state1) {

	const PlayerPtrCont & opps = state1.opponentsFromSelf();
	//const PredictPlayerObject  &tms = state1.ourPlayers();
	state1.ballHolderUnum(); /////////////////////////
	int i = 0;
	double totalEval = 0;
	double temp;
	vector<PlayerObject *> oppVec; //3 ta opp nazdike ma
	vector<double> distEval; //exp
	double dist[3]; //fasele
	vector<double> angleEval; //anglevalue
	double oppBallAngleDiffVec[3]; //tafazole zavie
	Vector2D faseleVec;
	Vector2D playerVel;
	Vector2D ballPos;
	AngleDeg oppBallAngle;
	AngleDeg oppAngle;
	int index1 = 0;
	int index2 = 0;
	int index3 = 0;
	int index4 = 0;

	if (opps.size() < 1)
		return 0;
	//FASELErrr
	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps.end();
			++opp) {
		if (i == 3) {
			break;
		}

		if ((*opp)->unum() < 1)
			continue;
		/*for(int i = 1 ;i<11 ; i++){
		 if(state1.ourPlayer(i)==NULL)
		 continue;
		 if(state1.ourPlayer(i)->unum() < 0)
		 continue;
		 tmPos = state1.ourPlayer(i)->pos();
		 oppPos = (*opp)->pos();
		 oppTmFasele = sqrt(
		 pow(tmPos.x - oppPos.x, 2) + pow(tmPos.y - oppPos.y, 2));
		 if (oppTmFasele < 4) { //has blocked
		 continue;
		 }
		 }*/
		ballPos = state1.ball().pos();

		faseleVec = ballPos - (*opp)->pos();

		double fasele = faseleVec.r();

		if (fasele > 10)
			continue;

		oppVec.push_back(*opp);


		dist[i] = fasele-(*opp)->posCount();
		if(dist[i] < 0)dist[i] = 0;

		oppBallAngle = faseleVec.th(); //4 cases

		oppAngle = (*opp)->body();

		playerVel = (*opp)->vel();

		double a = (oppAngle - oppBallAngle).abs();
		if (a > 180.0)
			a = 360 - a;

		/*cout << "_block dribble__cycle:" << state1.currentTime().cycle()
		 << "_unum:" << (*opp)->unum() << "_oppvelth:"
		 << (*opp)->vel().th() << "_oppbody:" << oppAngle << "_oppBall:"
		 << oppBallAngle << "_bodyBallDiffangle:" << a << endl;
		 */
		oppBallAngleDiffVec[i] = a;

		i++;

	} //DIST SORT
//	cout << "%%%%%%%%%%cycle:" << state1.currentTime().cycle() << endl;
	double tem = 0.0;
	int index5 = 0;

	if (i > 0) {

		if (i == 1)
			distEval.push_back(distCalc(dist[0]));

	}
	if (i > 1) {
		if (dist[1] < dist[0]) {
			tem = dist[0];
			dist[0] = dist[1];
			dist[1] = tem;
		}
		if (i == 2) {
			distEval.push_back(distCalc(dist[0]));
			distEval.push_back(distCalc(dist[1]));
		}
	}

	if (i > 2) {
		if (dist[2] < dist[1]) {
			tem = dist[1];
			dist[1] = dist[2];
			dist[2] = tem;
			index5 = 1;
		}
		if (index5 == 1) {
			if (dist[1] < dist[0]) {
				tem = dist[0];
				dist[0] = dist[1];
				dist[1] = tem;
			}
		}
		if (index5 == 0) {
			if (dist[2] < dist[0]) {
				tem = dist[0];
				dist[0] = dist[2];
				dist[2] = tem;
			}
		}
		if (i == 3) {
			distEval.push_back(distCalc(dist[0]));
			distEval.push_back(distCalc(dist[1]));
			distEval.push_back(distCalc(dist[2]));
		}
	}
	/*//	if (i > 0) {
	 ////cout << "_BLOCK DRIBBLE__disteval___" << endl;
	 //	for (int j = 0; j < distEval.size(); j++) {
	 ////cout << dist[j] << " => ";
	 ////cout << distEval[j] << " _ ";
	 //}
	 ////cout << "______________end_______________" << endl;
	 //}

	 //DIFF SORT
	 if (i > 0) {

	 }*/
	if (i > 1) {
		if (oppBallAngleDiffVec[1] < oppBallAngleDiffVec[0]) {
			temp = oppBallAngleDiffVec[1];
			index1 = 1;
			oppBallAngleDiffVec[0] = temp;
		}
	}
	if (i > 2) {
		if (oppBallAngleDiffVec[2] < oppBallAngleDiffVec[1]) {
			temp = oppBallAngleDiffVec[2];
			index2 = 1;
			oppBallAngleDiffVec[1] = temp;
		}
		if (index2 == 1) {
			if (oppBallAngleDiffVec[1] < oppBallAngleDiffVec[0]) {
				temp = oppBallAngleDiffVec[1];
				index3 = 1;
				oppBallAngleDiffVec[0] = temp;
			}
		} else if (index2 == 0)
			if (oppBallAngleDiffVec[2] < oppBallAngleDiffVec[0]) {
				temp = oppBallAngleDiffVec[2];
				index4 = 1;
				oppBallAngleDiffVec[0] = temp;
			}
	}

	if (((index1 && index2 && index3) == 1)
			|| (((index1 && index2 && index3) == 0) && (index4 == 1))) {
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		/*if (i > 2)
		 cout << "___ sorted opp vec__1_" << oppBallAngleDiffVec[2] << "_"
		 << oppBallAngleDiffVec[1] << "_" << oppBallAngleDiffVec[0]
		 << endl;*/
		/*if (i > 2)
		 cout << "11ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if ((index1 || index2 || index3) == 0) {
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		/*if (i > 2)
		 cout << "___ sorted opp vec__2_" << oppBallAngleDiffVec[0] << "_"
		 << oppBallAngleDiffVec[1] << "_" << oppBallAngleDiffVec[2]
		 << endl;*/
		//if (i > 2)
		/*cout << "22ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if (index1 == 0 && (index2 == 1) && (index3 == 1)) {
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		/*if (i > 2)

		 cout << "___ sorted opp vec__3_" << oppBallAngleDiffVec[2] << "_"
		 << oppBallAngleDiffVec[1] << "_" << oppBallAngleDiffVec[0]
		 << endl;*/
		/*if (i > 2)
		 cout << "33ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if (index2 == 0 && (index1 == 1) && (index4 == 1)) {
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));

		//if (i > 2)
		/*cout << "___ sorted opp vec__4_" << oppBallAngleDiffVec[2] << "_"
		 << oppBallAngleDiffVec[0] << "_" << oppBallAngleDiffVec[1]
		 << endl;*/
		//if (i > 2)
		/*cout << "44ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if (index2 == 0 && (index1 == 1) && (index4 == 0)) {
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		//if (i > 2)
		/*cout << "___ sorted opp vec__5_" << oppBallAngleDiffVec[1] << "_"
		 << oppBallAngleDiffVec[0] << "_" << oppBallAngleDiffVec[2]
		 << endl;*/
		//if (i > 2)
		/*cout << "55ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if (index3 == 0 && (index2 == 1) && (index1 == 1)) {
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		////cout<<"*********angleeval"<<angleEval[1]<<endl;
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		////cout<<"*********angleeval"<<angleEval[2]<<endl;

		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		/*cout<<"*********angleeval"<<angleEval[0]<<endl;*/
		//if (i > 2)
		/*cout << "__ sorted opp vec__6_" << oppBallAngleDiffVec[1] << "_"
		 << oppBallAngleDiffVec[2] << "_" << oppBallAngleDiffVec[0]
		 << endl;*/
		//if (i > 2)
		/*cout << "66ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}
	if (index1 == 0 && (index2 == 1) && (index3 == 0)) {
		if (i > 0)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[0]->pos(),
							oppBallAngleDiffVec[0], oppVec[0]->body(),
							oppVec[0]->vel(), dist[0]));
		if (i > 2)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[2]->pos(),
							oppBallAngleDiffVec[2], oppVec[2]->body(),
							oppVec[2]->vel(), dist[2]));
		if (i > 1)
			angleEval.push_back(
					angleCalc2(ballPos, oppVec[1]->pos(),
							oppBallAngleDiffVec[1], oppVec[1]->body(),
							oppVec[1]->vel(), dist[1]));
		//if (i > 2)
		/*cout << "__ sorted opp vec__7_" << oppBallAngleDiffVec[0] << "_"
		 << oppBallAngleDiffVec[2] << "_" << oppBallAngleDiffVec[1]
		 << endl;*/
		//if (i > 2)
		/*cout << "77ae" << angleEval[0] << "_"
		 << angleEval[1] << "_" << angleEval[2]
		 << endl;*/
	}

	/*cout << "*******12345*********" << index1 << index2 << index3 << index4
	 << index5 << endl;*/

	double ae1 = 0;
	double ae2 = 0;
	double ae3 = 0;
	double d1 = 0;
	double d2 = 0;
	double d3 = 0;

	if (i > 0) {
		ae1 = angleEval[0];
		d1 = distEval[0];
		////cout<<"^^^^^^^^^ae1"<<ae1<<endl;
	}
	if (i > 1) {
		ae2 = angleEval[1];
		d2 = distEval[1];
		////cout<<"^^^^^^^^^ae2"<<ae2<<endl;
	}
	if (i > 2) {
		ae3 = angleEval[2];
		d3 = distEval[2];
		////cout<<"^^^^^^^^^ae3"<<ae3<<endl;
	}

	double totalDistEval = (3.0 / 4.0) * (d1) + (1.0 / 6.0) * (d2)
			+ (1.0 / 12.0) * (d3);
	/*cout << "____cycle:" << state1.currentTime().cycle() << "____totaldist: "
	 << totalDistEval << endl;*/
	double totalAngleEval = (3.0 / 4.0) * (ae1) + (1.0 / 6.0) * (ae2)
			+ (1.0 / 12.0) * (ae3);
	/*cout << "____cycle:" << state1.currentTime().cycle() << "____totalangle: "
	 << totalAngleEval << endl;*/

	totalEval = totalAngleEval + totalDistEval;

	/*cout << "____cycle:" << state1.currentTime().cycle() << "____totalEval: "
	 << totalEval * 4.0 << endl;*/

	//dist dar renge 0 ta 10 bashad ta ba 1/2 avali va 1/3 dovomi va 1/6 sevomi va miyangin , be 30 beresim
	//angle dar renge 0 ta 5 bashad ta ba dist jam shode va yek adad dar renge 0 ta 15 bedahad
	return totalEval * 4.0;

}

double cyrus::distCalc(double minDist) {	//renge 0 ta 5

	double eval = 0;
	eval = exp(10 / 6.0) / exp(minDist / 6.0) * 10 - 10;
	return eval / 4;

}
double cyrus::angleCalc2(Vector2D ballPos, Vector2D oppPos, double angleDiff,
		AngleDeg playerBody, Vector2D playerVel, double fasele) {

	Vector2D playerBodyvec;
	double y = 0.0;
	double x = 0.0;

	if (playerBody.isRightOf(0))
		y = 1;
	else if (playerBody.isLeftEqualOf(0))
		y = -1;

	double ttt = tan(playerBody.degree());	//shib
	double ccc = cos(playerBody.degree());

	if (ccc > 0)
		x = 1;
	else if (ccc < 0)
		x = -1;

	y *= ttt;

	playerBodyvec = Vector2D(y, x);

	//____________________________________________________________________
	Vector2D velbordar = ballPos - playerVel;
	Vector2D oppBallVec = ballPos - oppPos;	//determine if it is between tow other vectors
	Vector2D bodybordar = ballPos - playerBodyvec;
	//____________________________________________________________________
	Vector2D left, right;
	if (velbordar.th().isRightEqualOf(bodybordar.th())) {
		left = velbordar;
		right = bodybordar;
	} else {
		right = velbordar;
		left = bodybordar;
	}
	//____________________________________________________________________

	double result = (left.y * oppBallVec.x - left.x * oppBallVec.y)
			* (left.y * right.x - left.x * right.y);
	//___________________________________________________________________

	AngleDeg oppBallAngle = oppBallVec.th();

	double a = (playerVel.th() - oppBallAngle).abs();
	double b = (playerBody - oppBallAngle).abs();
	if (a > 180.0)
		a = 360 - a;

	if (b > 180.0)
		b = 360 - b;
	//___________________________________________________________________
	double iftow = 0.2;
	double value = 0;
	double tanasob = 0;
	double playerbody = b;
	double playervel = a;


	if (result < 0)
		value += iftow;
	if (fasele < 2.0)
		value += 9;
	else if (fasele < 4)
		value += 6;
	else if (fasele < 7)
		value += 3;
	else if (fasele < 10)
		value += 1;

	if (playerVel.r() < 0.2)
		value += 0;
	else if (playerVel.r() < 0.8)
		value += 1;
	else if (playerVel.r() < 1.3)
		value += 2;
	else if (playerVel.r() < 1.5)
		value += 3;

	if (playervel < 5)
		value += 3;
	else if (playervel < 15)
		value += 2;
	else if (playervel < 30)
		value += 1;

	if (playerbody < 10)
		value += 1;
	else if (playerbody < 30)
		value += 2;

	if (playerBody == playerVel.th()) {
		value += 2;
	}

	/*if (fasele < 2.0) {
	 if (playerVel.r() > 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value += 17.0;
	 } else if (playerbody < 30.0) {
	 value += 16.0;

	 }
	 } else if (playervel < 15) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 16.0;
	 } else if (playerbody < 30.0) {
	 value = 15.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 15.0;
	 } else if (playerbody < 30.0) {
	 value = 14.0;

	 }

	 }
	 }
	 if (playerVel.r() < 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 16.0;
	 } else if (playerbody < 30.0) {
	 value = 15.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 15.0;
	 } else if (playerbody < 30.0) {
	 value = 14.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 13.0;
	 } else if (playerbody < 30.0) {
	 value = 12.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.8) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 15.0;
	 } else if (playerbody < 30.0) {
	 value = 14.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 14.0;
	 } else if (playerbody < 30.0) {
	 value = 13.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 13.0;
	 } else if (playerbody < 30.0) {
	 value = 12.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.2) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 14.0;
	 } else if (playerbody < 30.0) {
	 value = 13.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 13.0;
	 } else if (playerbody < 30.0) {
	 value = 12.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 12.0;
	 } else if (playerbody < 30.0) {
	 value = 11.0;

	 }

	 }
	 }

	 }*/
	/*if (fasele < 4.0) {
	 if (playerVel.r() > 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 14.0;
	 } else if (playerbody < 30.0) {
	 value = 13.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }

	 if (playerbody < 10.0) {
	 value = 13.0;
	 } else if (playerbody < 30.0) {
	 value = 12.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 12.0;
	 } else if (playerbody < 30.0) {
	 value = 11.0;

	 }

	 }
	 }
	 if (playerVel.r() < 1.3) {
	 if (playervel < 5.0) {

	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 13.0;
	 } else if (playerbody < 30.0) {
	 value = 12.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 12.0;
	 } else if (playerbody < 30.0) {
	 value = 11.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 11.0;
	 } else if (playerbody < 30.0) {
	 value = 10.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.8) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 12.0;
	 } else if (playerbody < 30.0) {
	 value = 11.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 11.0;
	 } else if (playerbody < 30.0) {
	 value = 10.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 10.0;
	 } else if (playerbody < 30.0) {
	 value = 9.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.2) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 11.0;
	 } else if (playerbody < 30.0) {
	 value = 10.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 10.0;
	 } else if (playerbody < 30.0) {
	 value = 9.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 9.0;
	 } else if (playerbody < 30.0) {
	 value = 8.0;

	 }

	 }
	 }

	 }*/
	/*if (fasele < 7.0) {
	 if (playerVel.r() > 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 11.0;
	 } else if (playerbody < 30.0) {
	 value = 10.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 10.0;
	 } else if (playerbody < 30.0) {
	 value = 9.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 9.0;
	 } else if (playerbody < 30.0) {
	 value = 8.0;

	 }

	 }
	 }
	 if (playerVel.r() < 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 10.0;
	 } else if (playerbody < 30.0) {
	 value = 9.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 9.0;
	 } else if (playerbody < 30.0) {
	 value = 8.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 8.0;
	 } else if (playerbody < 30.0) {
	 value = 7.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.8) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 9.0;
	 } else if (playerbody < 30.0) {
	 value = 8.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 8.0;
	 } else if (playerbody < 30.0) {
	 value = 7.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 7.0;
	 } else if (playerbody < 30.0) {
	 value = 6.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.2) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 8.0;
	 } else if (playerbody < 30.0) {
	 value = 7.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 7.0;
	 } else if (10 < playerbody < 30.0) {
	 value = 6.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 6.0;
	 } else if (playerbody < 30.0) {
	 value = 5.0;

	 }

	 }
	 }

	 }*/
	/*if (fasele < 10.0) {
	 if (playerVel.r() > 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 9.0;
	 } else if (playerbody < 30.0) {
	 value = 8.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 8.0;
	 } else if (10 < playerbody < 30.0) {
	 value = 7.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 7.0;
	 } else if (playerbody < 30.0) {
	 value = 6.0;

	 }

	 }
	 }
	 if (playerVel.r() < 1.3) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 8.0;
	 } else if (playerbody < 30.0) {
	 value = 7.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 7.0;
	 } else if (playerbody < 30.0) {
	 value = 6.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 6.0;
	 } else if (10 < playerbody < 30.0) {
	 value = 5.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.8) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 7.0;
	 } else if (playerbody < 30.0) {
	 value = 6.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 6.0;
	 } else if (playerbody < 30.0) {
	 value = 5.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 5.0;
	 } else if (playerbody < 30.0) {
	 value = 4.0;

	 }

	 }
	 }
	 if (playerVel.r() < 0.2) {
	 if (playervel < 5.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 6.0;
	 } else if (playerbody < 30.0) {
	 value = 5.0;

	 }
	 } else if (playervel < 15.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 5.0;
	 } else if (playerbody < 30.0) {
	 value = 4.0;

	 }

	 } else if (playervel < 30.0) {
	 double value = 0.0;
	 if (velbordar * bodybordar < 0.0) {
	 value += iftow;
	 }
	 if (playerbody < 10.0) {
	 value = 4.0;
	 } else if (playerbody < 30.0) {
	 value = 3.0;

	 }

	 }
	 }

	 }
	 */

	tanasob = value * (0.714);
	////cout<<"________value______"<<tanasob<<endl;
	return tanasob;

}

/*double cyrus::angleCalc(AngleDeg angleDiff, AngleDeg playerbody,
 Vector2D playervel, double fasele ) {

 // 1- barresi mizane kam boodan angle dar renge 0 ta 5
 // 2- tasire kam boodane ekhtelafe jahate sorat va body bazikon dar renge  0 ta 5
 // 3- tasire nazdikie bazikone daraye kamtarin angle dar renge  0 ta 5
 // dar nahayat 1/2 (1)+1/3 (2)+1/6 (3) taghsim bar 3

 double value1 = 0;	// dar renge 0 ta 5 yek emtiaz bede
 double value2 = 0;
 double value3 = 0;
 double totalValue = 0;

 if ((angleDiff.isWithin(0, 5))) {
 value1 = 5;
 } else if ((angleDiff.isWithin(5, 15))) {
 value1 = 4;
 }

 ///????????//

 double velR = playervel.r();

 AngleDeg velBodyDiff = playerbody - atan(playervel.x / playervel.y);	//har che kamtar bashad khatarnak tar ast

 if(velR <= 0.2)//min
 if(fasele >= 8 )//min
 if(playervel == playerbody)//max
 if(fasele)





 if((( c.isLeftEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,10) && (angleDiff .isWithin(0,10)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))))
 {
 value2 = 5;
 }
 else if((( c.isLeftEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,10)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))))
 {
 value2 = 4.5;
 }
 else if((( c.isLeftEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,20)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,20)))))
 {
 value2 = 4;
 }
 else if((( c.isLeftEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))))
 {
 value2 = 3.5;
 }

 //_______________________________________________________________________________________________
 else if(( c.isLeftEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,10) && (angleDiff .isWithin(0,10)))

 ||( c.isRightEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,10) && (angleDiff .isWithin(0,10))))
 {

 value2 = 2.5;
 }


 else if(( c.isLeftEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,10)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,10))))
 {

 value2 = 2;
 }

 else if((( c.isLeftEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,20)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,20) && (angleDiff .isWithin(0,20)))))
 {
 value2 = 1.5;
 }

 else if((( c.isLeftEqualOf(playerbody) && oppselfangle.isLeftEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))
 || ( c.isRightEqualOf(playerbody) && oppselfangle.isRightEqualOf(playerbody)

 && (velBodyDiff).isWithin(0,30) && (angleDiff .isWithin(0,30)))))
 {
 value2 = 1;
 }


 if (fasele < 4 && angleDiff.isWithin(0, 10)) {
 value3 = 4.5;
 } else if (fasele < 6 && angleDiff.isWithin(0, 20)) {
 value3 = 4;
 } else if (fasele < 8 && angleDiff.isWithin(0, 30)) {
 value3 = 3.5;
 } else if (fasele < 10 && angleDiff.isWithin(0, 40)) {
 value3 = 3;
 }

 totalValue = value1 * (3.0 / 4.0) + value2 * (1.0 / 6.0)
 + value3 * (1.0 / 12.0);

 return totalValue;
 }*/

double cyrus::mark_pass(const PredictState & state,const WorldModel & wm){
	vector<double> opps_eval;
	int self_cycle = wm.interceptTable()->selfReachCycle();
	int target_unum = state.ballHolderUnum();
	Vector2D frst_ball_pos = wm.ball().inertiaPoint(self_cycle);
	Vector2D trgt_ball_pos = state.ball().pos();
	int action_time = state.spendTime();
	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();
	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp)->unum() < 1)
			continue;
		double opp_eval = 0;
		Vector2D opp_pos = (*opp)->pos();
		Vector2D opp_vel = (*opp)->vel();
		double body = (*opp)->body().degree();
		double trgt_angle = (trgt_ball_pos-opp_pos).th().abs();
		if(trgt_angle>180)trgt_angle=360-trgt_angle;
		double vel_angle = (*opp)->vel().th().degree();
		double vel_dif_2_target = (vel_angle-trgt_angle);
		if(vel_dif_2_target<0)vel_dif_2_target*=(-1.0);

		if(vel_dif_2_target>180)vel_dif_2_target=360-vel_dif_2_target;

		double body_dif_2_target = (body-trgt_angle);
		if(body_dif_2_target<0)body_dif_2_target*=(-1.0);
		if(body_dif_2_target>180)body_dif_2_target=360-vel_dif_2_target;


		double dist = opp_pos.dist(trgt_ball_pos);
		int n_turn = predict_player_turn_cycle((*opp)->playerTypePtr(),(*opp)->body(),(*opp)->vel().r(),dist,(trgt_ball_pos-opp_pos).th(),1,true,opp_pos);
		double bonus_dist = 0;

	    const int pos_count = std::min( 10, // Magic Number
	                                    std::min( (*opp)->seenPosCount(),
	                                              (*opp)->posCount() ) );
	    double thr = 0.9;
	    double max_speed = (*opp)->playerTypePtr()->realSpeedMax() * 0.9; // Magic Number

	    double d = 0.0;
	    double magic=15;
	    if(trgt_ball_pos.x<28)
	    	magic = 20;
	    if(trgt_ball_pos.x<10)
	    	magic = 23;

	    for ( int i = 1; i <= pos_count; ++i ) // start_value==1 to set the initial_value<1
	    {
	        d += max_speed * std::exp( - (i*i) / magic ); // Magic Number
	    }

	    bonus_dist = d;
	    if(d>5)d = 5;
	    double bondist = dist-bonus_dist;
	    if(bondist<0)bondist=0;
		int cycle_2_trgt = cyrus().cycleToReachDist(wm,(*opp)->unum(),false,bondist,false);
		if(cycle_2_trgt==1000)cycle_2_trgt=(*opp)->playerTypePtr()->cyclesToReachDistance(bondist);
		cycle_2_trgt+=n_turn;

		if(dist>10)
			continue;
		if(cycle_2_trgt-action_time>8)
			continue;
		if(dist>8 && (vel_dif_2_target>30 || body_dif_2_target >30))
			continue;

		double dist_too_mate = 1000;
		int mate_mark = 0;
		const PlayerPtrCont & mates = wm.teammatesFromSelf();
		const PlayerPtrCont::const_iterator mates_end = mates.end();
		for (PlayerPtrCont::const_iterator mate = mates.begin(); mate != mates_end;
				++mate) {
			if ((*mate)->unum() < 1)
				continue;
			if((*mate)->pos().dist(opp_pos)<dist_too_mate){
				mate_mark = (*mate)->unum();
				dist_too_mate=(*mate)->pos().dist(opp_pos);
			}
		}
		if(mate_mark != target_unum && dist_too_mate<5 && dist>6){
			continue;
		}

		double cycle_eval = 10-bound(0,cycle_2_trgt,10);
		opp_eval += cycle_eval;

		double dist_eval = 10 - dist;
		if(dist_eval<0)dist_eval= 0;
		opp_eval += dist_eval;

		double vel_eval = 0;
		if(opp_vel.r()>0.2 && vel_dif_2_target < 45 && body_dif_2_target < 45){
			double velr = opp_vel.r();
			double veld = 45 - vel_dif_2_target;
			double bodyd = 45 - body_dif_2_target;
			if(veld>25)veld+=(veld/8.0);
			if(veld>15)veld+=(veld/10.0);
			if(veld>5)veld+=(veld/12.0);

			if(veld>25)veld+=(veld/8.0);
			if(veld>15)veld+=(veld/10.0);
			if(veld>5)veld+=(veld/12.0);

			if(velr>0.8)velr+=(velr/10.0);
			if(velr>0.6)velr+=(velr/12.0);
			if(velr>0.4)velr+=(velr/15.0);
			if(velr>0.2)velr+=(velr/18.0);
			vel_eval = velr*veld*bodyd/160;
		}
		opp_eval+=vel_eval;

		double mark_other_eval = 0;
		if(mate_mark != target_unum && dist_too_mate<5){
			mark_other_eval = dist_too_mate * 2;
		}

		opp_eval-=mark_other_eval;

//		cout<<" "<<cycle_eval<<" "<<dist_eval<<" "<<vel_eval<<" -"<<mark_other_eval<<endl;
		opps_eval.push_back(opp_eval);
	}

	double max_eval = 0;
	int max = 0;
	for (int i = 0; i < opps_eval.size(); i++) {
		if (max_eval < opps_eval[i]) {
			max_eval = opps_eval[i];
			max = i;
		}
	}
	double max_eval_2 = 0;
	for (int i = 0; i < opps_eval.size(); i++) {
		if (i == max)
			continue;
		if (max_eval_2 < opps_eval[i]) {
			max_eval_2 = opps_eval[i];
		}
	}

	return ((max_eval+max_eval_2)/1.0);
}

bool cyrus::amIwin(const WorldModel & wm, int def) {
	if (def < 0)
		def = 0;
	int our_score = (
			wm.ourSide() == LEFT ?
					wm.gameMode().scoreLeft() : wm.gameMode().scoreRight());
	int opp_score = (
			wm.ourSide() == LEFT ?
					wm.gameMode().scoreRight() : wm.gameMode().scoreLeft());
	if (our_score >= opp_score + def)
		return true;
	return false;
}

bool cyrus::amIlost(const WorldModel & wm, int def) {
	if (def < 1)
		def = 1;
	int our_score = (
			wm.ourSide() == LEFT ?
					wm.gameMode().scoreLeft() : wm.gameMode().scoreRight());
	int opp_score = (
			wm.ourSide() == LEFT ?
					wm.gameMode().scoreRight() : wm.gameMode().scoreLeft());
	if (our_score + def <= opp_score)
		return true;
	return false;
}

bool cyrus::isnearEnd(const WorldModel & wm) {
	int time = wm.time().cycle();
	if (abs(3000 - time) < 200 || abs(6000 - time) < 200
			|| abs(7000 - time) < 200 || abs(8000 - time) < 200)
		return true;
	return false;

}

double cyrus::oppDist[12][50] = { 0 };
double cyrus::tmDist[12][50] = { 0 };
bool cyrus::set = false;
int cyrus::cyup = -1;

int cyrus::opptype[12]={0};
int cyrus::matetype[12]={0};
void cyrus::cycleToReachSpeed(const WorldModel & wm) {
	if ( cyup==wm.time().cycle())
		return;

	set = true;
	cyup = wm.time().cycle();

	const PlayerPtrCont & opps = wm.opponentsFromSelf();
	const PlayerPtrCont::const_iterator opps_end = opps.end();
	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp)->unum() < 1) {
			continue;
		}
		if(opptype[(*opp)->unum()]!=(*opp)->playerTypePtr()->id())
			set=false;
	}
	const PlayerPtrCont & tms = wm.teammatesFromSelf();
	const PlayerPtrCont::const_iterator tms_end = tms.end();
	for (PlayerPtrCont::const_iterator tm = tms.begin(); tm != tms_end; ++tm) {
		if ((*tm)->unum() < 1) {
			continue;
		}
		if(opptype[(*tm)->unum()]!=(*tm)->playerTypePtr()->id())
			set=false;
	}

	if(set)
		return;

	for (PlayerPtrCont::const_iterator opp = opps.begin(); opp != opps_end;
			++opp) {
		if ((*opp)->unum() < 1) {
			set = false;
			continue;
		}
		const PlayerType * ptype = (*opp)->playerTypePtr();
		int unum = abs((*opp)->unum());
		opptype[unum]=ptype->id();

		double Speed = (*opp)->vel().r();
		double speed_max = ptype->playerSpeedMax();
		double accel = ServerParam::i().maxDashPower() * ptype->dashPowerRate()
				* ptype->effortMax();
		double reach_dist = 0;
		for (int i = 1; i < 50; i++) {

			if (Speed + accel > speed_max) {
				accel = speed_max - Speed;
			}

			Speed += accel;

			reach_dist += Speed;
			oppDist[unum][i] = reach_dist;
			dlog.addText(Logger::TEAM,
					__FILE__"cyrus new cycle unum:%d,cycle:%d,dist:%f", unum, i,
					oppDist[unum][i]);

			Speed *= ptype->playerDecay();
		}
	}
	for (PlayerPtrCont::const_iterator tm = tms.begin(); tm != tms_end; ++tm) {
    	if((*tm)->unum()<1){
			set = false;
    		continue;
    	}

		const PlayerType * ptype = (*tm)->playerTypePtr();
		const int unum = (*tm)->unum();

		matetype[unum]=ptype->id();

		double Speed = (*tm)->vel().r();
		double speed_max = ptype->playerSpeedMax();
		double accel = ServerParam::i().maxDashPower() * ptype->dashPowerRate()
				* ptype->effortMax();
		double reach_dist = 0;
		for (int i = 1; i < 50; i++) {

			if (Speed + accel > speed_max) {
				accel = speed_max - Speed;
			}

			Speed += accel;

			reach_dist += Speed;
			tmDist[unum][i] = reach_dist;
			dlog.addText(Logger::TEAM,
					__FILE__"cyrus new cycle unum:%d,cycle:%d,dist:%f", unum, i,
					tmDist[unum][i]);

			Speed *= ptype->playerDecay();
		}
	}
///self
	const PlayerType * ptype = &(wm.self().playerType());
	const int unum = wm.self().unum();
	if (unum == 0)
		return;
	double Speed = wm.self().vel().r();
	double speed_max = ptype->playerSpeedMax();
	double accel = ServerParam::i().maxDashPower() * ptype->dashPowerRate()
			* ptype->effortMax();
	double reach_dist = 0;
	for (int i = 1; i < 50; i++) {

		if (Speed + accel > speed_max) {
			accel = speed_max - Speed;
		}

		Speed += accel;

		reach_dist += Speed;
		tmDist[unum][i] = reach_dist;
		dlog.addText(Logger::TEAM,
				__FILE__"cyrus new cycle unum:%d,cycle:%d,dist:%f", unum, i,
				tmDist[unum][i]);

		Speed *= ptype->playerDecay();
	}

}

int cyrus::cycleToReachDist(const WorldModel & wm, int unum, bool tm,
		double dist, bool me) {
	if (unum < 1)
		return dist+5;

	cycleToReachSpeed(wm);


//	return 0;
	if (me) {
		if (tmDist[unum][2] == 0)
			return 1000;
		for (int i = 0; i < 50; i++) {
			if (tmDist[unum][i] > dist)
				return i;
		}
		return 1001;
	}
	if (tm) {
		if (tmDist[unum][2] == 0)
			return 1000;
		if (wm.ourPlayer(unum)!=NULL)
			for (int i = 0; i < 50; i++) {
				if (tmDist[unum][i] > dist)
					return i;
			}

	} else {
		if (oppDist[unum][2] == 0)
			return 1000;
		if (wm.theirPlayer(unum)!=NULL)
			for (int i = 0; i < 50; i++) {
				if (oppDist[unum][i] > dist)
					return i;
			}

	}
	return 1001;
	/*

	 int cycle2speed = 0;
	 for( int i=0 ; i<50 ; i++){
	 if( speed > speedA[i]-0.1 )
	 cycle2speed = i;
	 }
	 double dist2speed = distA[cycle2speed];
	 double max_speed = ptype->playerSpeedMax();
	 int cycle2max_speed = ptype->cyclesToReachMaxSpeed();
	 int cycle_dist = ptype->cyclesToReachDistance(dist);
	 if( cycle_dist > cycle2max_speed ){
	 int cycle_by_max_speed = (int)(max_speed/dist2speed);
	 if ( cycle_by_max_speed != max_speed/dist2speed )cycle_by_max_speed++;
	 dlog.addText(Logger::TEAM,__FILE__"*** ***dist:%.1f,speed:%.1f,cycle2speed:%d,dist2speed:%.1f,maxspeed:%.1f,c2ms:%d,cycleDist:%d,(if),cyclebyMS:%d,cycle2reach:%d"
	 ,dist,speed,cycle2speed,dist2speed,max_speed,cycle2max_speed,cycle_dist,cycle_by_max_speed,cycle_dist - cycle2speed + cycle2max_speed);
	 return cycle_dist - cycle2speed + cycle2max_speed;
	 }else{
	 for( int i=cycle2speed+1 ; i<50 ; i++){
	 if( distA[i] > distA[cycle2speed] - dist2speed ){
	 dlog.addText(Logger::TEAM,__FILE__"*** ***dist:%.1f,speed:%.1f,cycle2speed:%d,dist2speed:%.1f,maxspeed:%.1f,c2ms:%d,cycleDist:%d,(else),cyclebyMS:%d,cycle2reach:%d"
	 ,dist,speed,cycle2speed,dist2speed,max_speed,0,cycle_dist,0,i - cycle2speed);
	 return i - cycle2speed;
	 }
	 }
	 }
	 return cycle_dist;*/
}

int
cyrus::predict_player_turn_cycle( const rcsc::PlayerType * ptype,
		const rcsc::AngleDeg & player_body,
		const double & player_speed,
		const double & target_dist,
		const rcsc::AngleDeg & target_angle,
		const double & dist_thr,
		const bool use_back_dash,
		Vector2D & new_opp_pos)
{
	const ServerParam & SP = ServerParam::i();

	int n_turn = 0;

	double angle_diff = ( target_angle - player_body ).abs();

	if ( use_back_dash
			&& target_dist < 5.0 // Magic Number
			&& angle_diff > 90.0
			&& SP.minDashPower() < -SP.maxDashPower() + 1.0 )
	{
		angle_diff = std::fabs( angle_diff - 180.0 );    // assume backward dash
	}

	double turn_margin = 180.0;
	if ( dist_thr < target_dist )
	{
		turn_margin = std::max( 15.0, // Magic Number
				rcsc::AngleDeg::asin_deg( dist_thr / target_dist ) );
	}

	double speed = player_speed;
	while ( angle_diff > turn_margin )
	{
		angle_diff -= ptype->effectiveTurn( SP.maxMoment(), speed );
		speed *= ptype->playerDecay();
		new_opp_pos += Vector2D::polar2vector(speed,player_body);
		++n_turn;
	}

#ifdef DEBUG_PREDICT_PLAYER_TURN_CYCLE
	dlog.addText( Logger::ANALYZER,
			"(predict_player_turn_cycle) angleDiff=%.3f turnMargin=%.3f nTurn=%d",
			angle_diff);
#endif

	return n_turn;
}


vector<double>
cyrus::chain_player_evaluation(const WorldModel & wm , bool * emptychain){

	vector<double> playersEval= vector<double>(23);

	*emptychain=false;

//	std::ofstream debug("777cyrus eval prints.txt",	std::ios_base::app | std::ios_base::out);

	playersEval[0]=20;//ball



	vector<ActionStatePair> chain = ActionChainGraph::My_result;

	if(chain.empty()){
		*emptychain=true;
		//		debug<<"\n embptyyyyyyyy \n";

		return playersEval;
	}

	for (vector<ActionStatePair>::const_iterator it = chain.begin();it != chain.end(); ++it) {
//		debug<< wm.time().cycle()<<" u "<<wm.self().unum()<<" cat "<<(*it).action().category()<<" ballpos " <<(*it).state().ball().pos()<<" ubh "<< (*it).state().ballHolderUnum()<<"\n";

		//teammates
		int unum = (*it).statePtr()->ballHolderUnum();

		if(unum > 0){
			if((*it).actionPtr()->category()==CooperativeAction::Dribble){//1
				playersEval[unum]+=3;
			}
			if((*it).actionPtr()->category()==CooperativeAction::Pass){//2
				playersEval[unum]+=9;
				if(it+1 != chain.end()){
					int unumNextBallHolder=(*(it+1)).statePtr()->ballHolderUnum();
					if(unumNextBallHolder>0){
						playersEval[unumNextBallHolder]+=7;
						if( it == chain.begin() ) playersEval[unumNextBallHolder]+=3;
//						debug<<" nextpassto "<<unumNextBallHolder<<"\n";
					}
				}

			}
			if((*it).actionPtr()->category()==CooperativeAction::Shoot){//3
				playersEval[unum]+=2;
			}
			if((*it).actionPtr()->category()==CooperativeAction::Clear){
				playersEval[unum]+=1;
			}
			if((*it).actionPtr()->category()==CooperativeAction::Hold){
				playersEval[unum]+=3;
			}
			if((*it).actionPtr()->category()==CooperativeAction::Move){
				playersEval[unum]+=2;
			}
			if((*it).actionPtr()->category()==CooperativeAction::NoAction){
				playersEval[unum]+=0;
			}
			if(!(*it).actionPtr()->M_action_safe){
				playersEval[unum]++;
			}


			////////////////teammate dist to chain
			int tmindistthr=10;
			double tmindist=tmindistthr;

			int unumMinDist=-1;


			const PlayerPtrCont & tmms = wm.teammatesFromBall();
			for (PlayerPtrCont::const_iterator itt = tmms.begin(); itt != tmms.end();
					++itt) {

				double ittmdist = (*itt)->pos().dist((*it).state().ballHolder()->pos());

				if(ittmdist <=tmindist && unum!=(*itt)->unum()){
					tmindist=ittmdist;
					unumMinDist=(*itt)->unum();
				}

			}

			if(unumMinDist>0){
				playersEval[unumMinDist] += (double)((tmindistthr-tmindist))/tmindistthr;
				//				debug<<" umin "<<unumMinDist<<" evmin "<< playersEval[unumMinDist];
			}
			/////////////////////////

		}

		//opponents
		double dist=0;
		int maxdistthr=10;
		if(wm.getOpponentNearestTo((*it).statePtr()->ball().pos(),maxdistthr, &dist)!=NULL){

			int oppUnum=wm.getOpponentNearestTo((*it).statePtr()->ball().pos(),maxdistthr, &dist)->unum();
			if(oppUnum>0 && dist<=maxdistthr){
				playersEval[(oppUnum+11)] += (double)(8*(maxdistthr-dist))/maxdistthr;
			}

			//			debug<<" u "<<unum<<" ev "<< playersEval[unum]<<" ou " <<oppUnum+11 <<" oev "<<playersEval[(oppUnum+11)]<<"\n";
		}
	}


	playersEval[0] *= wm.ball().posCount();

	for(int i=1 ; i <= 11 ; i++){
		if(wm.ourPlayer(i)!=NULL && wm.ourPlayer(i)->posCount()>=0){
			playersEval[i]*= wm.ourPlayer(i)->posCount();
		}
	}

	for(int i=1 ; i <= 11 ; i++){
		if(wm.theirPlayer(i)!=NULL && wm.theirPlayer(i)->posCount()>=0){
			playersEval[i+11]*= wm.theirPlayer(i)->posCount();
		}
	}

//	debug<<" \n";
//	for(int i=0 ; i < 23 ; i++){
//		debug<<" ";
//		if(i==12)debug<<"*";
//		if(i==5)debug<<",";
//		debug<<playersEval[i]<<", ";
//	}
//	debug<<"\n";


//	debug.close();
	return playersEval;
}



int
cyrus::indexOfMaxEvalForView(PlayerAgent * agent, bool * empty){

	const WorldModel & wm = agent->world();
	const ServerParam & sp = ServerParam::i();

	int maxIndex=0;
	int maxEval=0;
//	std::ofstream debug("777cyrus eval prints.txt",	std::ios_base::app | std::ios_base::out);

	vector<double> evals = chain_player_evaluation(wm, empty);

	for( int i = 0 ; i < 23 ; i++ ){

		if(*empty){
			//			debug<<"indexempty\n";
			return 0;
		}

		AngleDeg body = agent->effector().queuedNextSelfBody();
		AngleDeg extra= 0;
		switch ( agent->effector().queuedNextViewWidth().type() ) {
		case ViewWidth::NARROW:
			extra = 25;//60
			break;
		case ViewWidth::NORMAL:
			extra = 55;//120
			break;
		case ViewWidth::WIDE:
			extra = 85;//180
			break;
		default:
			break;
		}

		AngleDeg minAngle = body - 90 - extra;
		AngleDeg maxAngle = body + 90 + extra;

		const double BALL_VIS_DIST
		=  sp.visibleDistance()
		- ( wm.self().vel().r() / wm.self().playerType().playerDecay() ) * 0.1
		- ( wm.ball().vel().r() / ServerParam::i().ballDecay() ) * 0.05
		- ( 0.12 * std::min( 4, wm.ball().posCount() ) )
		- 0.25 ;


		const double VIS_DIST
		= sp.visibleDistance()
		- ( wm.self().vel().r() / wm.self().playerType().playerDecay() ) * 0.1
		- 0.25 ;

//		debug<<"\n "<< sp.visibleDistance()<<" "<<BALL_VIS_DIST<<" "<<VIS_DIST<<" "<<minAngle<<" "<<maxAngle;

		Sector2D ballviewSector= Sector2D(agent->effector().queuedNextSelfPos(),0,/*BALL_VIS_DIST*/30,minAngle,maxAngle);
		Sector2D selfviewSector= Sector2D(agent->effector().queuedNextSelfPos(),0,/*VIS_DIST*/30,minAngle,maxAngle);

		if( i==0
				&& !ballviewSector.contains( agent->effector().queuedNextBallPos()) ){
//			debug<<"\n ballomited";
			continue;
		}


		if( i!=0 && i<=11
				&& agent->world().ourPlayer(i)!=NULL
				&& !selfviewSector.contains( wm.ourPlayer(i)->pos() ) ){
//			debug<<"\n tm "<<i <<" omited";
			continue;
		}

		if( i>11
				&& agent->world().theirPlayer(i-11)!=NULL
				&& !selfviewSector.contains( wm.theirPlayer(i-11)->pos() ) ){
//			debug<<"\n op "<<i-11 <<" omited";
			continue;
		}


		if(evals[i]>maxEval){
			maxEval=evals[i];
			maxIndex=i;
//			debug<<"    "<<maxIndex<<"    ";
		}
	}

//	debug<<"\n "<<wm.time().cycle()<<" selfu "<<wm.self().unum()<<" ulastindex "<< maxIndex<< " \n";
//	debug.close();

	return maxIndex;// 0 for ball, 1-11 tmm , 12-22 opp
}

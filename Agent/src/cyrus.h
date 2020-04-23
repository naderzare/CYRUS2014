/*
 * cyrus.h
 *
 *  Created on: Apr 27, 2013
 *      Author: 007
 */

#ifndef CYRUS_H_
#define CYRUS_H_
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/player_agent.h>
#include <vector>
#include "chain_action/predict_state.h"

using namespace std;

using namespace rcsc;
class cyrus {
public:
	cyrus();
	int cyclesToReachToPoint(const WorldModel& wm ,const AbstractPlayerObject * p,Vector2D target,double distThr);
	int state_confidence(const PredictState & wm);

	int predict_player_turn_cycle( const rcsc::PlayerType * ptype,
			const rcsc::AngleDeg & player_body,
			const double & player_speed,
			const double & target_dist,
			const rcsc::AngleDeg & target_angle,
			const double & dist_thr,
			const bool use_back_dash,
			Vector2D & new_opp_pos);

	double block_drible(const PredictState & state1) ;

	double distCalc(double minDist);
	double angleCalc2(Vector2D ballPos, Vector2D oppPos, double angleDiff,
			AngleDeg playerBody, Vector2D playerVel, double fasele) ;

	double mark_pass(const PredictState & state,const WorldModel & wm);
	int blockme(const WorldModel & wm);
	int block_ev(const PredictState & wm);
	bool amIwin(const WorldModel & wm , int def);
	bool amIlost(const WorldModel & wm , int def);
	bool isnearEnd(const WorldModel & wm);
	void cycleToReachSpeed( const WorldModel & wm );
	int cycleToReachDist( const WorldModel & wm , int unum , bool tm , double dist , bool me = false);
	virtual ~cyrus();
	static double oppDist[12][50];
	static double tmDist[12][50];
	static bool set;
	static int cyup;
	static int opptype[12];
	static int matetype[12];

	std::vector<double> chain_player_evaluation(const WorldModel & wm, bool * emptychain);

	int indexOfMaxEvalForView(PlayerAgent * agent,bool * emptychain );

};

#endif /* CYRUS_H_ */

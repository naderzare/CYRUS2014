// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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

/////////////////////////////////////////////////////////////////////

/*
 * bhv_shoot.h
 *
 *  Created on: Jul 17, 2012
 *      Author: 007
 */



#ifndef BHV_SHOOT_H_
#include <config.h>

#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/bhv_shoot2008.h>

#include <rcsc/common/server_param.h>


#define BHV_SHOOT_H_
#include <rcsc/player/player_agent.h>
#include <string>
#include "bhv_basic_offensive_kick.h"

#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/bhv_shoot2008.h>

#include <iostream>
using namespace std;
using namespace rcsc;


class bhv_shoot {
public:
	static double speed_dist [30][50];
	static std::vector<double> maxSpeedDegree;

	struct view
	{
		AngleDeg neckAngle;
		ViewWidth vw;
		int angleCount;
		view(AngleDeg neckAngle,ViewWidth vw,int angleCount):
			neckAngle(neckAngle),
			vw(vw),
			angleCount(angleCount)
		{}
	};

	enum ShootType
	{
		OneStepShoot,
		KickSmartShoot
	};
	struct Shoot
	{
		rcsc::Vector2D target;
		double ballSpeed;
		ShootType shootType;
		double shootEval;
		bool isGoal;

		Shoot(rcsc::Vector2D target, double ballSpeed, ShootType shootType, bool isGoal,double shootEval)
		: target(target),
		  ballSpeed(ballSpeed),
		  shootType(shootType),
		  shootEval(shootEval),
		  isGoal(isGoal)
		{}
	};
	Vector2D pointCalc(Vector2D first,Vector2D second,double marjin);
	int cyclesToReachToPoint(const WorldModel& wm ,const AbstractPlayerObject * p,Vector2D opprealPos,Vector2D target,double distThr);
	Vector2D leftOrRightOfSector(Line2D l,AngleDeg start,AngleDeg end,Vector2D oppPos1,Vector2D selfPos,int angleCount);
	Vector2D opprealPos(const WorldModel &wm,Vector2D oppPos1,Vector2D target);
	AngleDeg Type2Angle(ViewWidth vw);
	bool canOppInterceptBallMidle(const rcsc::WorldModel & wm, double speed,rcsc::Vector2D target,bool log,int * safe);
	bool createShoot(rcsc::PlayerAgent * agent);
	static std::vector<Shoot> possibleShoot;
	void sortShoot();
	void SetMaxSpeed(rcsc::PlayerAgent *agent);
	double getMaxSpeedOneKick(double angle);
	bool execute( rcsc::PlayerAgent * agent );
	void evaluatorShoot();
	int opp_cycle_get_ball(const WorldModel & wm ,
			const PlayerObject * opp ,
			Vector2D target ,
			double ballSpeed ,
			bool interceptBall,
			int *eval);
	int opp_cycle_go_point(const WorldModel & wm ,
			const PlayerObject * opp ,
			Vector2D target ,
			double ballSpeed ,
			bool interceptBall);
	static void set_speed_dist();
};

#endif /* BHV_SHOOT_H_ */

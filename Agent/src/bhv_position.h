/*
 * bhv_position.h
 *
 *  Created on: Nov 12, 2012
 *      Author: 007
 */

#ifndef BHV_POSITION_H_
#define BHV_POSITION_H_

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <vector>
using namespace std;
using namespace rcsc;
class bhv_position /*: public rcsc::SoccerBehavior */{
public:
		static int last_cycle ;
		static Vector2D target_pos;
	bhv_position(){

	}
	enum posType{
		leadPos,
		throuPos,
		simplePos
	};
//	virtual ~bhv_position();
	struct posfor
	{
		int firstUnum;
		posType firstType;
		double firstmaxD2pos;
		double firstmaxD2tar;

		posfor(int fu=0,posType ft=leadPos , double fd2p=0 , double fd2t=0)
		:firstUnum(fu),firstType(ft),firstmaxD2pos(fd2p),firstmaxD2tar(fd2t)
		{}
	};
	struct passpos
	{
		Vector2D target;
		double ballspeed;
		bool conf;
		double eval;
		double cycleintercept;
		passpos(Vector2D t=Vector2D(100,100),double v=0,bool c=false,double ev=0,double cyc=0)
		:target(t),ballspeed(v),conf(c),eval(ev),cycleintercept(cyc)
		{}
	};
	struct Position
	{
		Vector2D ballpos;
		int holder;
		Vector2D target;
		int cycle2target;
		posType type;
		bool conf;
		bool tmconf;
		double eval;
		vector<passpos> passposvector;
		Position(Vector2D bp=Vector2D(100,100),int tm=0,Vector2D tar=Vector2D(100,100),int d2t=0,posType pt=leadPos,bool c=false,bool tmc=false,
					double ev=0,vector<passpos> ppv=vector<passpos>())
		:ballpos(bp),holder(tm),target(tar),cycle2target(d2t),type(pt),conf(c),tmconf(tmc),eval(ev),passposvector(ppv)
		{}
	};
	bool updatePos(PlayerAgent * agent);
	void leadPosition(PlayerAgent * agent);
	bool canPass2betterMe(rcsc::PlayerAgent *agent,double myEval);
	int self_cycle_get_ball(const WorldModel & wm ,Vector2D ballPos,double ballSpeed, const SelfObject & self ,Vector2D postar, Vector2D target ,Vector2D * dytarget);
	void OppCycleGetBall(rcsc::PlayerAgent *agent,Position pos,passpos pass,int * a,int * b);
	bool excute(PlayerAgent * agent,Position Pos);
	bool canIpos(PlayerAgent * agent);
	int tmDanger(PlayerAgent *agent,int t);
	int lastpos(PlayerAgent * agent);
	double passEval(passpos pass);
	double posEval(const WorldModel & wm,
			Position pos , rcsc::PlayerAgent * agent);
	void simulate_dash(PlayerAgent * agent , int tm ,double maxD2pos,double maxD2tar,std::vector<bhv_position::Position> & posPosition);
	void throuPassFor(PlayerAgent * agent , int tm ,double maxD2pos,double maxD2tar);
	posfor whatTm(PlayerAgent *agent);
	int predict_player_turn_cycle( const rcsc::PlayerType * ptype,
			const AngleDeg & player_body,
			const double & player_speed,
			const double & target_dist,
			const AngleDeg & target_angle,
			const double & dist_thr,
			const bool use_back_dash);
	double dist_near_tm(const WorldModel & wm,
			Vector2D point);
	void lead_pass_simulat(const WorldModel & wm,
			Vector2D passer_pos,
			Vector2D new_self_pos,
			int n_step,
			vector<passpos> & passes);
	int self_cycle_intercept(const WorldModel & wm,
			Vector2D pass_start,
			double pass_speed,
			Vector2D & pass_target,
			double self_body,
			double speed);
	int opps_cycle_intercept(const WorldModel & wm,
			Vector2D pass_start,
			double pass_speed,
			int pass_cycle,
			Vector2D pass_target);
	int opp_cycle_intercept(const WorldModel & wm,
			AbstractPlayerObject * opp,
			Vector2D pass_start,
			double pass_speed,
			int pass_cycle,
			Vector2D pass_target);
	bool check_other_pos(const WorldModel & wm,
			Position & pos,
			bool now = true);
	bool bestpos (const WorldModel & wm,
			bhv_position::Position best,
			bool have_new_pos = true);
	bool do_last_pos(PlayerAgent * agent,
			const WorldModel & wm );
	bool checkRectangle (const WorldModel & wm , int num , int width);
	bool checkDirectPass (const WorldModel & wm );
	bool isHigher (const WorldModel & wm , Vector2D tmPos);
	bool isLower (const WorldModel & wm , Vector2D tmPos);
	bool isFront (const WorldModel & wm , Vector2D tmPos);
	bool isBefore (const WorldModel & wm , Vector2D tmPos);
	Vector2D nearestTowardOppDef (const WorldModel & wm );
	int numOfOppAround (const WorldModel & wm , int num ,int meter);
	double distance2NearestTowardOpp (const WorldModel & wm);
	bool lenght2NearestTowardOppInWidth (const WorldModel & wm , int minLenght , int maxLenght);
	bool IamSurrounded (const WorldModel & wm , int unum , int numOfOpp , int maxLenght , int distFromOut);
	Vector2D target4MiddleOppAndOffside (const WorldModel & wm , double beforeOffside , double uppOfMiddle);
	bool nearestOppIsInOffsideLine (const WorldModel & wm);
	bool ballIsTowardOffsideLine (const WorldModel & wm);
	bool isBeforeOffside (const WorldModel & wm , int num);
	bool IsNearest2Ball(const WorldModel & wm , int num , bool opp);
	bool isNearMe (const WorldModel & wm , int num , bool opp , double maxDist);
	bool widthDifference (const WorldModel & wm , int num , bool opp , double maxwidth);
	bool lenghtDifference (const WorldModel & wm , int num , bool opp , double maxLenght);
	bool state_1 (const WorldModel & wm , PlayerAgent * agent);
	bool state_2 (const WorldModel & wm , PlayerAgent * agent);
	bool state_3 (const WorldModel & wm , PlayerAgent * agent);
	bool state_4 (const WorldModel & wm , PlayerAgent * agent);
	bool state_5 (const WorldModel & wm , PlayerAgent * agent);
	bool state_6 (const WorldModel & wm , PlayerAgent * agent);
	bool state_7 (const WorldModel & wm , PlayerAgent * agent);
	bool state_8 (const WorldModel & wm , PlayerAgent * agent);
	bool state_9 (const WorldModel & wm , PlayerAgent * agent);
	bool state_10 (const WorldModel & wm , PlayerAgent * agent);
	bool state_11 (const WorldModel & wm , PlayerAgent * agent);
	bool state_12 (const WorldModel & wm , PlayerAgent * agent);
	bool state_13 (const WorldModel & wm , PlayerAgent * agent);
	bool state_14 (const WorldModel & wm , PlayerAgent * agent);
	bool state_15 (const WorldModel & wm , PlayerAgent * agent);
	bool state_16 (const WorldModel & wm , PlayerAgent * agent);
	bool state_17 (const WorldModel & wm , PlayerAgent * agent);
	bool state_18 (const WorldModel & wm , PlayerAgent * agent);
	bool state_19 (const WorldModel & wm , PlayerAgent * agent);
	bool state_20 (const WorldModel & wm , PlayerAgent * agent);
	bool state_21 (const WorldModel & wm , PlayerAgent * agent);
	bool state_22 (const WorldModel & wm , PlayerAgent * agent);
	/////////////////////////Ehsan/////////////////////////////
	bool PointofAdjacency2Ball(PlayerAgent * agent,Position Pos);
	bool PointofExistenceInOppDanger(PlayerAgent * agent,Position Pos);
	bool PointofAdjacency2OppGoal(PlayerAgent * agent,Position Pos);
	bool PointofDistance2Me(PlayerAgent * agent,Position Pos);
	double PointofAbsenceofOpps(PlayerAgent * agent,Position Pos);
	bool pointofBeingNearTm (PlayerAgent * agent,Position Pos);
	char actionSafety(rcsc::PlayerAgent * agent , Vector2D first , Vector2D target , char action);
	///////////////////////////////////////////////////////////
};

#endif /* BHV_POSITION_H_ */

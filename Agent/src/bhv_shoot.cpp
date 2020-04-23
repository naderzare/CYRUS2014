// -*-c++-*-

/*
 * bhv_shoot.cpp
 *
 *  Created on: Jul 17, 2012
 *      Author: 007
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "bhv_shoot.h"
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/common/player_type.h>
#include <rcsc/action/bhv_shoot2008.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <iostream>
#include <vector>
#include <rcsc/action/arm_point_to_point.h>
#include <rcsc/player/intercept_table.h>
#include "bhv_basic_offensive_kick.h"
#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_dribble.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_pass.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

// #define DEBUG_PRINT

using namespace std;
using namespace rcsc;

static bhv_shoot::Shoot lastShoot=bhv_shoot::Shoot(Vector2D(52.5,5),1,bhv_shoot::KickSmartShoot,true,0);
std::vector<bhv_shoot::Shoot> bhv_shoot::possibleShoot;
std::vector<double> bhv_shoot::maxSpeedDegree;
static vector<bhv_shoot::view> currentView;

void bhv_shoot::SetMaxSpeed(PlayerAgent *agent){
	const WorldModel & wm=agent->world();
	Vector2D ballvel=wm.ball().vel();
	double kickrate=wm.self().kickRate();
	for(int i=0;i<12;i++){
		maxSpeedDegree.push_back(Body_KickOneStep::get_max_possible_vel(i*30,kickrate,ballvel).r());
	}
}

double bhv_shoot::getMaxSpeedOneKick(double angle){
	if(angle<0)angle+=360;
    size_t minZ=static_cast<size_t>(angle/30);
	double dif=angle-minZ*30;
	if(dif>15)minZ++;
	return maxSpeedDegree[minZ];
}

int bhv_shoot::cyclesToReachToPoint(const WorldModel& wm ,const AbstractPlayerObject * p,Vector2D opprealPos,Vector2D target,double distThr){
	const PlayerType * pt=p->playerTypePtr();
	const ServerParam & SP = ServerParam::i();
	StaminaModel stamina_model;
	stamina_model.init( *pt );
	int n_turn=0;
	Vector2D playerPos=opprealPos;
	Vector2D playerSpeedVec=p->vel();
	Vector2D target_relative = target - playerPos;
	AngleDeg target_angle = target_relative.th();
	AngleDeg player_body=p->body();
	double dash_power = SP.maxDashPower();
	double target_dist=target.dist(playerPos);
	double accel = SP.maxDashPower() * pt->dashPowerRate() * pt->effortMax();
	Vector2D accelVec=Vector2D::polar2vector( accel,target_angle );

	//turn
	double angle_diff = ( target_angle - player_body ).abs();

	if ( target_dist < 5.0 // Magic Number
			&& angle_diff > 90.0
			&& SP.minDashPower() < -SP.maxDashPower() + 1.0 )
	{
		angle_diff = std::fabs( angle_diff - 180.0 );    // assume backward dash
	}

	//double turn_margin = 180.0;
	if ( 0.5 < target_dist )
	{
		//turn_margin = std::max( 15.0, // Magic Number
		rcsc::AngleDeg::asin_deg( 0.5 / target_dist ) ;
	}
	if(playerPos.dist(target)<distThr)return 1;
	for ( int counter = n_turn; counter <= 50; ++counter )
	{
		if ( (playerSpeedVec + accelVec).r() > pt->playerSpeedMax() ){
			playerSpeedVec=Vector2D::polar2vector(pt->playerSpeedMax(),target_angle);
			dash_power = std::min( SP.maxDashPower(),
					accel / ( pt->dashPowerRate() * stamina_model.effort() ) );
		}else{
			playerSpeedVec+=accelVec;
		}



		playerPos+=playerSpeedVec;

		if(playerPos.dist(target)<distThr)return counter;

		playerSpeedVec *= pt->playerDecay();
		stamina_model.simulateDash(*pt,dash_power);
		if ( stamina_model.stamina() <= 0.0 )
		{
			return 1000;
		}
	}
    return 1000;
}

Vector2D bhv_shoot::leftOrRightOfSector(Line2D l,AngleDeg start,AngleDeg end,Vector2D oppPos1,Vector2D selfPos,int angleCount){
	Line2D ll=Line2D(selfPos,end);
	Vector2D intersect=l.intersection(ll);
    int meterToPos=static_cast<int>(oppPos1.dist2(intersect));
	if(angleCount>=meterToPos) return intersect;
	else if(angleCount<meterToPos){
		Line2D lll=Line2D(selfPos,start);
		return l.intersection(lll);
	}
    return Vector2D::INVALIDATED;
}

AngleDeg bhv_shoot::Type2Angle(ViewWidth vw){
	AngleDeg angle;
	if(vw.type()==ViewWidth::NARROW)angle=30;
	else if(vw.type()==ViewWidth::NORMAL)angle=45;
	else if(vw.type()==ViewWidth::WIDE)angle=90;
	return angle;
}

Vector2D bhv_shoot::opprealPos(const WorldModel &wm,Vector2D oppPos1,Vector2D ballPos){
	Vector2D min=Vector2D(10000,10000);
	AngleDeg start;
	AngleDeg end;
	Vector2D oppPos2;
	Line2D l=Line2D(oppPos1,ballPos);
	Vector2D selfPos=wm.self().pos();
    for(size_t i=0;i<currentView.size();i++){
		oppPos2=Vector2D::polar2vector(currentView[i].angleCount,(ballPos-oppPos1).th());//ba farz inke dar har cycle 1 meter jelo rafte bashad
		AngleDeg first=(oppPos1-selfPos).th();
		AngleDeg second=(oppPos2-selfPos).th();
		if(!currentView[i].neckAngle.isWithin(first,second))continue;
		AngleDeg a=Type2Angle(currentView[i].vw);
		end=currentView[i].neckAngle+a;
		start=currentView[i].neckAngle-a;
		Vector2D oppR=leftOrRightOfSector(l,start,end,oppPos1,selfPos,currentView[i].angleCount);
		if(oppR==oppPos1)return oppPos1;
		else if((sqrt(pow(min.x-oppPos1.x,2)+pow(min.y-oppPos1.y,2)))>(sqrt(pow(oppR.x-oppPos1.x,2)+pow(oppR.y-oppPos1.y,2)))){
			min=oppR;
		}
	}
	return min;
}

Vector2D bhv_shoot::pointCalc(Vector2D first,Vector2D second,double marjin){
	Line2D l=Line2D(first,second);
	double xl=l.getX(5);
	double a=5/xl;
	AngleDeg shib=atan(a);
	Vector2D intersect=Vector2D::polar2vector(marjin,shib);
	first+=intersect;
	return first;
}

int bhv_shoot::opp_cycle_get_ball(const WorldModel & wm , const PlayerObject * opp , Vector2D target , double ballSpeed , bool interceptBall,int * eval){

	const ServerParam & SP = ServerParam::i();
    static const Rect2D penalty_area( Vector2D( SP.theirPenaltyAreaLineX(),
                                                -SP.penaltyAreaHalfWidth() ),
                                      Size2D( SP.penaltyAreaLength(),
                                              SP.penaltyAreaWidth() ) );
    const PlayerType * ptype = opp->playerTypePtr();
	int cycle=0;
	Vector2D oppPos=opp->inertiaPoint(1);//makan cycle baad opp
	Vector2D ballPos=( wm.self().isKickable()
			? wm.ball().pos()
					: wm.ball().pos() + wm.ball().vel() );
	//baraye sector
	AngleDeg a=(target-ballPos).th();
	a+=180;
	AngleDeg left=a-70;
	AngleDeg right=a+70;
	const Sector2D sector( ballPos,0,100,left,right );
	if (  sector.contains(oppPos)){
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"***************opp %d is behind me",(*opp).unum());
#endif
		return 1001;
	}

#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"***************opp %d start of shoot simulate",(*opp).unum());
#endif

	cycle++;
	ballPos+=Vector2D::polar2vector(ballSpeed,(target-wm.ball().pos()).th());
	ballSpeed*=0.96;

	//toop cycle ..
	while( ballSpeed > 0.1 && cycle < 30 && ballPos.absX() < 52.8 && ballPos.absY() < 34.0 )
	{
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"********************cycle=%d-ballpos=(%1.1f,%1.1f)-ballspeed=%1.1f",cycle,ballPos.x,ballPos.y,ballSpeed);
#endif
		if(oppPos.dist(ballPos)>cycle+2+opp->posCount()){
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"*************************opp can not intercept");
#endif
			//update
			cycle++;
			ballPos+=Vector2D::polar2vector(ballSpeed,(target-wm.ball().pos()).th());
			ballSpeed*=0.96;
			continue;
		}
		int dely=0;

		dely++;//for view
		Vector2D oppRealPos=oppPos;//opprealPos(wm,opp->pos(),ballPos);
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"*************************oppPos=(%f,%f)",opp->pos().x,opp->pos().y);
		dlog.addText(Logger::TEAM,__FILE__"*************************oprPos=(%f,%f)",oppRealPos.x,oppRealPos.y);
#endif
		double dist_opp2ball=oppRealPos.dist(ballPos);
		double opp_kick_area=ptype->kickableArea();
		if(opp->goalie())opp_kick_area=(penalty_area.contains( ballPos ) ? SP.catchableArea() : ptype->kickableArea());
		if(opp->goalie())
			if((opp->body()-(target-oppPos).th()).abs()<30)
				opp_kick_area=SP.tackleDist();
		{
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"*************************od=%f odpc=%f oprd=%f",oppdist2ball,oppdist2ballWposcount,opprealdist2ball);
#endif
		}
		//		Vector2D balltarget=pointCalc(oppRealPos,ballPos,opp_kick_area);
		int cycle_opp2ball=cyclesToReachToPoint(wm,opp,oppRealPos,ballPos,opp_kick_area);//////change
		//mohasebe delay
		cycle_opp2ball+=dely;
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"*************************cycleOpp2ball:%d",cycle_opp2ball);
#endif
		if(opp->goalie()){
			if(cycle_opp2ball-1-cycle<(*eval)){
				(*eval)=cycle_opp2ball-1-cycle;
			}
		}
		else{
			if(cycle_opp2ball-cycle<(*eval)){
				(*eval)=cycle_opp2ball-cycle;
			}
		}
		double distErr=ballPos.dist(wm.ball().inertiaPoint(1))*tan(3);
		int opp_bouns=bound( 0, opp->posCount(), 1 );
		if(opp->goalie())
			opp_bouns=bound( 0, opp->posCount(), 5 );
		if(dist_opp2ball-opp_kick_area-distErr<=0.0
				||cycle_opp2ball<=cycle){
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"*************************opp intercept ball");
#endif
			return cycle;
		}
		//update
		cycle++;
		ballPos+=Vector2D::polar2vector(ballSpeed,(target-wm.ball().pos()).th());
		ballSpeed*=0.96;
	}

	if(ballPos.x>52.7){
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"***************opp can not intercept ball");
#endif
		return 1001;
	}
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"***************ball can not recive gol");
#endif
	return 1;
}

bool bhv_shoot::canOppInterceptBallMidle(const rcsc::WorldModel & wm, double speed,rcsc::Vector2D target,bool log,int * safe){
	int eval=1001;

	const PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
	for ( PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
			o != o_end;
			++o ){
    	if((*o)->unum()<1)continue;

		if ( (*o)->isTackling() ) continue;
        if ( (*o)->pos().x < 15 ) continue;
        if ( (*o)->pos().absY() > 18 ) continue;
        if ( ( (target-wm.ball().pos()).th() - (*o)->angleFromSelf() ).abs() > 90.0 ) continue;
        if( !(*o)->goalie() ){
            if ( (*o)->posCount() > 10 ) continue;
            if ( (*o)->isGhost() && (*o)->posCount() > 5 ) continue;
        }
		int opp_intercept_cycle=opp_cycle_get_ball(wm,(*o),target,speed,true,&eval);
		if(opp_intercept_cycle!=1001){
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"***************opp %d intercept ball in %d cycle",(*o)->unum(),opp_intercept_cycle);
#endif
			return true;
		}
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"***************opp %d not intercept ball",(*o)->unum());
#endif
	}
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"***************shoot eval=%1.1f",eval);
#endif
	(*safe)=eval;
	return false;
}

bool bhv_shoot::createShoot(rcsc::PlayerAgent * agent){
	const WorldModel &wm=agent->world();
	SetMaxSpeed(agent);
	possibleShoot.clear();

	double PS=wm.self().vel().r();
	double DA=abs((wm.self().body()-(Vector2D(52.5,0)-wm.ball().pos()).th()).degree());
	double BS=3;

	AngleDeg ErrAngle=2;
	ErrAngle*=pow(PS,1.18);
	ErrAngle*=pow(DA/180,0.75);
	ErrAngle*=(BS/3);
	ErrAngle+=2.9;
	bool dir2goal = true ;
	double myDir2Goal = wm.self().body().degree();
	if(myDir2Goal>90.0 && myDir2Goal<270.0) dir2goal = false ;
	if(!dir2goal){
		ErrAngle=4;
	}
	//////////////////////////////////////////////
	AngleDeg ball2left=(Vector2D(52.5,-7)-wm.ball().pos()).th();
	AngleDeg ball2Maxleft=ball2left+ErrAngle;
	AngleDeg ball2right=(Vector2D(52.5,7)-wm.ball().pos()).th();
	AngleDeg ball2Maxright=ball2right-ErrAngle;
	Vector2D ballpos=( wm.self().isKickable()
			? wm.ball().pos()
					: wm.ball().pos() + wm.ball().vel() );
	//momkene intersection irad dashte bashe , yani hamishe oon chizi ke mikhaym ro behemoon nade !!!
	Vector2D Maxleft=Line2D(ballpos,ball2Maxleft).intersection(Line2D(Vector2D(52.5,7),Vector2D(52.5,-7)));
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"*****MAxleft:(%f,%f)",Maxleft.x,Maxleft.y);
#endif
	Vector2D Maxright=Line2D(ballpos,ball2Maxright).intersection(Line2D(Vector2D(52.5,7),Vector2D(52.5,-7)));
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"*****MAxright:(%f,%f)",Maxright.x,Maxright.y);
#endif
	if(Maxright.y<=Maxleft.y){
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"*****(Maxright.y<=Maxleft.y)****************");
#endif
		return false;
	}
	// ehtemalan eshtebah bashad---> nist meske :D ->>> hala khabari nist :D
	if(Maxright.y<-7||Maxleft.y>7){
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"*****(Maxright.y<-7||Maxleft.y>7)**************");
#endif
		return false;
	}
	double oppDist=wm.interceptTable()->opponentReachCycle();
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"*****start for just Left & Right");
#endif
	for(int a=0;a<=1;a++){

		Vector2D target=(a==0?Maxleft:Maxright);
		double oneStepBallSpeed=getMaxSpeedOneKick((target-ballpos).dir().degree())+0.1;
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f)",target.x,target.y);
#endif

		bool checklowspeed=true;
		if(oppDist>2){
			int eval=0;
			double BallSpeed=2.9;
			ShootType ShType=(BallSpeed<oneStepBallSpeed?OneStepShoot:KickSmartShoot);
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
			bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
			checklowspeed=targetISsafe;
			if(targetISsafe){
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f-eval=%d is ok",target.x,target.y,BallSpeed,eval);
#endif
				possibleShoot.push_back(Shoot(target,BallSpeed,ShType,true,eval));
			}else{
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f is not ok",target.x,target.y,BallSpeed);
#endif
			}
		}
		if(checklowspeed && oppDist>1)
			if(oppDist<3){
				int eval=0;
				double BallSpeed=2.5;
				ShootType ShType=(BallSpeed<oneStepBallSpeed?OneStepShoot:KickSmartShoot);
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
				bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
				if(targetISsafe){
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f-eval=%d is ok",target.x,target.y,BallSpeed,eval);
#endif
					possibleShoot.push_back(Shoot(target,BallSpeed,ShType,true,eval));
				}else{
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f is not ok",target.x,target.y,BallSpeed);
#endif
				}
			}
		if(checklowspeed)
		{
			int eval=0;
			double BallSpeed=oneStepBallSpeed;
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
			bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
			if(targetISsafe){
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f-eval=%d is ok",target.x,target.y,BallSpeed,eval);
#endif
				possibleShoot.push_back(Shoot(target,BallSpeed,OneStepShoot,true,eval));
			}else{
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"***************target=(%1.1f,%1.1f)-speed=%1.1f is not ok",target.x,target.y,BallSpeed);
#endif
			}
		}
	}

	if(possibleShoot.empty()){
#ifdef DEBUG_PRINT
		dlog.addText(Logger::TEAM,__FILE__"*****start for for all target");
#endif
		for(double y=Maxleft.y+(Maxright.y-Maxleft.y)/18 ; y<=Maxright.y-(Maxright.y-Maxleft.y)/18 ; y+=(Maxright.y-Maxleft.y)/18){
			Vector2D target=Vector2D(52.5,y);
			double oneStepBallSpeed=getMaxSpeedOneKick((target-ballpos).dir().degree())+0.1;
#ifdef DEBUG_PRINT
			dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f)",target.x,target.y);
#endif
			bool checklowspeed=true;
			if(oppDist>2){
				int eval=0;
				double BallSpeed=2.9;
				ShootType ShType=(BallSpeed<oneStepBallSpeed?OneStepShoot:KickSmartShoot);
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
				bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
				checklowspeed=targetISsafe;
				if(targetISsafe){
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f-eval=%d is ok",BallSpeed,eval);
#endif
					possibleShoot.push_back(Shoot(target,BallSpeed,ShType,true,eval));
				}else{
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f is not ok",BallSpeed);
#endif
				}
			}
			if(checklowspeed && oppDist>1){
				int eval=0;
				double BallSpeed=2.5;
				ShootType ShType=(BallSpeed<oneStepBallSpeed?OneStepShoot:KickSmartShoot);
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
				bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
				if(targetISsafe){
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f-eval=%d is ok",BallSpeed,eval);
#endif
					possibleShoot.push_back(Shoot(target,BallSpeed,ShType,true,eval));
				}else{
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f is not ok",BallSpeed);
#endif
				}
			}
			if(checklowspeed){
				int eval=0;
				double BallSpeed=getMaxSpeedOneKick((target-ballpos).dir().degree())+0.1;
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"**********target:(%1.1f,%1.1f) with speed=%1.1f",target.x,target.y,BallSpeed);
#endif
				bool targetISsafe=!canOppInterceptBallMidle(agent->world(),BallSpeed,target,true,&eval);
				if(targetISsafe){
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f-eval=%d is ok",BallSpeed,eval);
#endif
					possibleShoot.push_back(Shoot(target,BallSpeed,OneStepShoot,true,eval));
				}else{
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"***************speed=%1.1f is not ok",BallSpeed);
#endif
				}
			}
		}
	}
	if(!possibleShoot.empty()){
		sortShoot();
		evaluatorShoot();
		if(oppDist<3){
            size_t i=0;
			while(i<possibleShoot.size()){
				if(possibleShoot[i].shootType==OneStepShoot){
					Shoot best=possibleShoot[i];
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"*****oop near best one kick shoot target=(%1.1f,%1.1f)-speed=%1.1f-eval=%1.1f",best.target.x,best.target.y,best.ballSpeed,best.shootEval);
#endif
					if(wm.existKickableOpponent())
						if(Body_KickOneStep(best.target,best.ballSpeed,true).execute(agent)){
							//Arm_PointToPoint(best.target).execute(agent);
#ifdef DEBUG_PRINT
							dlog.addText(Logger::TEAM,__FILE__"*****best shoot with one kick execute");
#endif
							return true;
						}
					if(Body_SmartKick(best.target,best.ballSpeed,best.ballSpeed*0.97,1).execute(agent)){
						//Arm_PointToPoint(best.target).execute(agent);
#ifdef DEBUG_PRINT
						dlog.addText(Logger::TEAM,__FILE__"*****best shoot with smart kick execute");
#endif
						return true;
					}
				}
				i++;
			}
		}
		else if(oppDist>2){
            size_t i=0;
			while(i<possibleShoot.size()&&i<10){
				Shoot best=possibleShoot[i];
#ifdef DEBUG_PRINT
				dlog.addText(Logger::TEAM,__FILE__"*****best smart kick shoot target=(%1.1f,%1.1f)-speed=%1.1f-eval=%1.1f",best.target.x,best.target.y,best.ballSpeed,best.shootEval);
#endif
				if(best.shootType==OneStepShoot){
					if(Body_SmartKick(best.target,best.ballSpeed,best.ballSpeed*0.97,1).execute(agent)){
						//Arm_PointToPoint(best.target).execute(agent);
#ifdef DEBUG_PRINT
						dlog.addText(Logger::TEAM,__FILE__"*****best shoot with smart kick execute");
#endif
						return true;
					}
				}
				if(Body_SmartKick(best.target,best.ballSpeed,best.ballSpeed*0.97,3).execute(agent)){
					//Arm_PointToPoint(best.target).execute(agent);
#ifdef DEBUG_PRINT
					dlog.addText(Logger::TEAM,__FILE__"*****best shoot with smart kick  execute");
#endif
					return true;
				}
				i++;
			}
		}
	}
	return false;
}
void bhv_shoot::evaluatorShoot(){
	for(uint i = 0; i < possibleShoot.size(); i++)
	{
		if(possibleShoot[i].shootEval>2)
			possibleShoot[i].shootEval=3;
		if(possibleShoot[i].shootType==OneStepShoot)
			possibleShoot[i].shootEval*=2;
		if(possibleShoot[i].target.absY()>6)
			possibleShoot[i].shootEval--;
	}
}
void bhv_shoot::sortShoot(){
	for(uint i = 0; i < possibleShoot.size(); i++)
	{
		for(uint j = i+1; j < possibleShoot.size(); j++)
		{
			if(possibleShoot[j].shootEval > possibleShoot[i].shootEval)
			{
				Shoot temp = possibleShoot[i];
				possibleShoot[i] = possibleShoot[j];
				possibleShoot[j] = temp;
			}
		}
	}
#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"possibleShoot.front:(%f,%f)",possibleShoot.front().target.x,possibleShoot.front().target.y);
#endif
}

bool bhv_shoot::execute(rcsc::PlayerAgent * agent){

	const WorldModel & wm=agent->world();

    for(size_t i=0;i<10&&i<currentView.size();i++){
		currentView[i].angleCount++;
	}
	currentView.push_back(view(wm.self().neck(),wm.self().viewWidth(),0));


#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"*****************Shoot start**************");
#endif
	if(wm.gameMode().type()!=GameMode::PlayOn||!wm.self().isKickable())return false;
	if(wm.teammatesFromBall().size()>0)
		if(wm.teammatesFromBall().front()->isKickable()&&wm.teammatesFromBall().front()->unum()>wm.self().unum())return true;
	if(agent->world().ball().pos().x>30&&agent->world().ball().pos().absY()<20)
		if(createShoot(agent)){
			return true;
		}

#ifdef DEBUG_PRINT
	dlog.addText(Logger::TEAM,__FILE__"*****************Shoot end**************");
#endif

	return false;
}

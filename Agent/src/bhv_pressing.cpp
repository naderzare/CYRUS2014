
#include "bhv_pressing.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>


#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"
#include <rcsc/action/arm_point_to_point.h>
#include <iostream>
using namespace std;
using namespace rcsc;



 bool Bhv_Press::execute(rcsc::PlayerAgent * agent)

 {
	const WorldModel & wm =agent->world();
	int unum=wm.self().unum();
	int tmCycle=wm.interceptTable()->teammateReachCycle();
	int oppCycle=wm.interceptTable()->opponentReachCycle();
	if(tmCycle<=oppCycle
			|| wm.self().stamina()<6000)
		return false;

    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.85, 90.0 ).execute( agent ) )
            return true;

        if ( ! wm.existKickableTeammate()
          && ! wm.existKickableOpponent()
          && ( wm.interceptTable()->selfReachCycle () <= oppCycle + 2 || wm.interceptTable()->selfReachCycle () < 3 ) 
          && wm.self().pos().x < wm.ball().pos().x
          &&wm.interceptTable()->selfReachCycle ()< wm.interceptTable()->teammateReachCycle())
         {
            Body_Intercept().execute( agent );
            //Arm_PointToPoint(Vector2D(0,0)).execute(agent);
//            cout<<"#########intercept\n";
            agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
            return true;
         }

	if ( ( oppCycle < min ( tmCycle , wm.interceptTable()->selfReachCycle () ) ) 
          || ( oppCycle <= 3 )                
          || ( wm.existKickableOpponent() )         )
             {    

		if(!isOffensePress(agent,unum))
                    return false;

		int nearestToball=0;
		double ball_dist=wm.self().inertiaPoint(1).dist(wm.ball().inertiaPoint(1));

		const PlayerPtrCont & tms = wm.teammatesFromSelf();
		const PlayerPtrCont::const_iterator tms_end = tms.end();
		for ( PlayerPtrCont::const_iterator it = tms.begin();
			  it != tms_end;
			  ++it )
		{
	    	if((*it)->unum()<1)continue;

			if ( (*it)->unum() == wm.self().unum() ) continue;
                        if ( (*it)->posCount() > 10            ) continue;
                        if ( (*it)->pos().x < wm.ourDefenseLineX() + 7.0 )

			int itunum=(*it)->unum();
			if (  (*it)->inertiaPoint(1).dist( wm.ball().inertiaPoint(1) ) < ball_dist - 2 //Cheack put depth for it TODO
	                    && isOffensePress(agent,(*it)->unum()))
			{
                           if ( ( (wm.self().inertiaPoint(1).y*(*it)->inertiaPoint(1).y) > 0 ) 
                             || ( (wm.self().inertiaPoint(1).y*(*it)->inertiaPoint(1).y) < 0 
                                  && (*it)->inertiaPoint(1).dist( wm.ball().inertiaPoint(1) ) < ball_dist - 5 ) )
				nearestToball++;
				return false;
			}

		}
		if ( nearestToball == 0 )// change it
                {
			if( dopress (agent)  )
                          return true;
		}
	}

	return false;

}
bool Bhv_Press::dopress ( PlayerAgent * agent )
{
  const WorldModel & wm =agent->world();
  const rcsc::PlayerObject * opponent = wm.opponentsFromBall().front();
  if ( !opponent )  return false;

  rcsc::Vector2D target_point = opponent->pos();
  
  if ( std::fabs( target_point.y - wm.self().pos().y ) > 1.0 )
    {
        rcsc::Line2D opp_move_line( opponent->pos(),
                                    opponent->pos() + rcsc::Vector2D( -3.0, 0.0 ) );//change
        rcsc::Ray2D my_move_ray( wm.self().pos(),
                                 wm.self().body() );
        rcsc::Vector2D intersection = my_move_ray.intersection( opp_move_line );
        if ( intersection.isValid()
             && intersection.x < target_point.x
             && wm.self().pos().dist( intersection ) < target_point.dist( intersection ) * 1.1 )
        {
            target_point.x = intersection.x;
        }
        else
        {
            target_point.x -= 5.0;
            if ( target_point.x > wm.self().pos().x
                 && wm.self().pos().x > Strategy::i().getPosition( wm.self().unum() ).x - 2.0 )
            {
                target_point.x = wm.self().pos().x;
            }
        }
    }
//   cout<<"##########press\n";
   bool recover_mode = true;
   if ( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.48 )
    {
        recover_mode = true;
    }
    if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.75 )
    {
        recover_mode = false;
    }

    double dash_power;
    if ( recover_mode )
    {
        dash_power = wm.self().playerType().staminaIncMax();
        dash_power *= wm.self().recovery();
        dash_power *= 0.8;
    }
    else
    {
        dash_power = rcsc::ServerParam::i().maxPower();
    }

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;
    if (  rcsc::Body_GoToPoint( target_point, dist_thr, dash_power
                                 ).execute( agent ) ){
    	//Arm_PointToPoint(target_point).execute(agent);
    }else
    {
        rcsc::Body_TurnToPoint( target_point ).execute( agent );
    }

    if ( wm.existKickableOpponent() )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }


    return true;


}
bool Bhv_Press::isOffensePress(PlayerAgent * agent,int num)
{
	const WorldModel & wm=agent->world();
	int oppCycle=wm.interceptTable()->opponentReachCycle();
	if ( num <= 6 ) 
           return false; //change

	if (   wm.ourPlayer(num)->pos().dist( wm.ball().pos() ) > 25 
            || Strategy::i().getPosition(num).dist ( wm.ball().pos() ) > 20 )
           return false;

	if ( num == 11 && wm.ball().inertiaPoint(oppCycle).x < -10 )
	   return false;

	else if (   num == 10 && (wm.ball().inertiaPoint(oppCycle).x < -15
	        /*|| wm.ball().inertiaPoint(oppCycle).y < 10*/ ) )
           return false;

	else if (   num == 9 && (wm.ball().inertiaPoint(oppCycle).x < -15
	        /*|| wm.ball().inertiaPoint(oppCycle).y > -10*/) )
	   return false;

	else if (   num == 8 && (wm.ball().inertiaPoint(oppCycle).x < -20
	        /*|| wm.ball().inertiaPoint(oppCycle).y < 0 */ ) )
           return false;

	else if (  num == 7 && (wm.ball().inertiaPoint(oppCycle).x < -20
	        /*|| wm.ball().inertiaPoint(oppCycle).y > 0 */ ) )
		return false;

	return true;


}


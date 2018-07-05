
////////////////////////////////////////////M.Kh////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_defense_mark_move.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"

#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_scan_field.h>

#include <rcsc/geom/ray_2d.h>
#include <rcsc/geom/line_2d.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/player_object.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <rcsc/player/player_object.h>

#include "neck_offensive_intercept_neck.h"
#include <rcsc/action/arm_point_to_point.h>
#include <iostream>
#include <rcsc/math_util.h>

#include "strategy.h"

using namespace std;
using namespace rcsc;



//////////////////////////////////////////////////////////////////////////////////////



bool
Defense_MarkMove::Stop_by_mark( rcsc::PlayerAgent * agent )
{

    rcsc::dlog.addText( rcsc::Logger::TEAM,
                        __FILE__": Bhv_BasicMove" );

    // tackle

    //    Bhv_MiracleBlock().execute( agent );

    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.7, 70.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();

    // check ball owner

    //int self_min = wm.interceptTable()->selfReachCycle();
    //int mate_min = wm.interceptTable()->teammateReachCycle();
    //int opp_min = wm.interceptTable()->opponentReachCycle();

    /*    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min < mate_min + 3
                   && self_min < opp_min + 4 )
              )
         )
    {
        rcsc::dlog.addText( rcsc::Logger::TEAM,
                            __FILE__": (execute) intercept" );

        rcsc::Body_Intercept().execute( agent );
        if ( wm.ball().distFromSelf()
             < rcsc::ServerParam::i().visibleDistance() )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );;
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }
        return true;
    }*/

    // go to home position


    rcsc::Vector2D  my_targrt(0,0);
    rcsc::Vector2D target_point = home , base = wm.ball().pos();
    vector<const rcsc::AbstractPlayerObject * > target;


    get_mark_pos(agent , &target);


    if (target.size()>0){
        my_targrt = target[0]->pos();


    }

    const rcsc::PlayerPtrCont & tems = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator t_end = tems.end();
    uint index = 0;
    for(int i = 0; i<6 ;i++)
        for ( rcsc::PlayerPtrCont::const_iterator tem = tems.begin(); tem != t_end ; ++tem )
        {
            if((*tem)->unum()<1)continue;

            if ( (*tem)->goalie() )                   continue;
            if ( (*tem)->isGhost()  )                 continue;
            if ( (*tem)->unum() == wm.self().unum() ) continue;
            if ( (*tem)->posCount() > 10 )            continue;

            if ((*tem)->pos().dist(my_targrt) < wm.self().pos().dist(my_targrt) -2 &&  index + 1 < target.size() && ((*tem)->unum() == 6 ||(*tem)->unum() == 7 || (*tem)->unum() == 8))
            {
                if (wm.ball().pos().x > -40 && wm.self().pos().x > -40&& (wm.self().unum()==2 ||wm.self().unum()==3 ||wm.self().unum()==5
                                                                          ||wm.self().unum()==4||wm.self().unum()==6 ))
                {
                    my_targrt = target[index+1]->pos();
                    index ++;
                }
            }
        }

    if (!(my_targrt.dist(rcsc::Vector2D (0,0))<2 ))
    {
        target_point = (base + (my_targrt))/2;
        target_point = (target_point+(my_targrt) )/2;
        target_point = ( target_point+(my_targrt) )/2;
        //target_point = getManToManMarkMove ( my_targrt , agent );
        //target_point.x -= 4;
    }

    if (wm.ball().pos().x > -40 && wm.self().pos().x > -40 && (wm.self().unum()==2 ||wm.self().unum()==3
                                                               ||wm.self().unum()==5 ||wm.self().unum()==4 ||wm.self().unum()==6))
        if (target_point.dist(home) > 10 )
            target_point = home;

    const double dash_power =getDashPower(agent , target_point);

    double dist_thr = 1.0;
    agent->debugClient().addMessage( " BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );


    if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power
                                 ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 10.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }

    return true;


}


/////////////////////////////////////////////////////////////////////////////////////////////



bool
Defense_MarkMove::mark_move( rcsc::PlayerAgent * agent )
{
    rcsc::dlog.addText( rcsc::Logger::TEAM,
                        __FILE__": Bhv_markMove" );

    // tackle
    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.7 , 50.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();

    // check ball owner

    //int self_min = wm.interceptTable()->selfReachCycle();
    //int mate_min = wm.interceptTable()->teammateReachCycle();
    //int opp_min = wm.interceptTable()->opponentReachCycle();

    /*    if ( ! wm.existKickableTeammate()
         && ( self_min <= 4
              || ( self_min <= mate_min-1
                   && self_min < opp_min + 3 )
              )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        if(wm.self().unum()==2||wm.self().unum()==3||wm.self().unum()==6
                        ||wm.self().unum()==4||wm.self().unum()==5)
        {
                if(wm.ball().pos().x<-10||self_min<opp_min){
                        Body_Intercept().execute( agent );
                                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

                                return true;
                }
        }else{
                        Body_Intercept().execute( agent );
                        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

                        return true;
        }
    }*/

    // go to mark position
    static rcsc::Vector2D  my_targrt(0,0);
    rcsc::Vector2D target_point = home , base (-50 , 0);

    vector<const rcsc::AbstractPlayerObject * > target;


    get_mark_pos(agent , &target);


    if (target.size()>0){
        my_targrt = target[0]->pos();


    }
    else
        return false;


    const rcsc::PlayerPtrCont & tems = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator t_end = tems.end();


    for ( rcsc::PlayerPtrCont::const_iterator tem = tems.begin(); tem != t_end ; ++tem )
    {
        if((*tem)->unum()<1)continue;

        if ( (*tem)->goalie() )                   continue;
        if ( (*tem)->isGhost()  )                 continue;
        if ( (*tem)->unum() == wm.self().unum() ) continue;
        if ( (*tem)->posCount() > 10 )            continue;
        if ( (*tem)->pos().dist(my_targrt) < wm.self().pos().dist(my_targrt) - 2 && ((*tem)->unum() == 2 || (*tem)->unum() == 3 ||(*tem)->unum() == 4 || (*tem)->unum() == 5||(*tem)->unum() == 6))
        {
            if (target.size()>1)
                my_targrt = target[1]->pos();
            else
                my_targrt = Vector2D(0,0);
        }

    }

    for ( rcsc::PlayerPtrCont::const_iterator tem = tems.begin(); tem != t_end ; ++tem )
    {
        if((*tem)->unum()<1)continue;

        if ( (*tem)->goalie() )                   continue;
        if ( (*tem)->isGhost()  )                 continue;
        if ( (*tem)->unum() == wm.self().unum() ) continue;
        if ( (*tem)->posCount() > 10 )            continue;
        if ((*tem)->pos().dist(my_targrt) < wm.self().pos().dist(my_targrt) -2 && ((*tem)->unum() == 2 || (*tem)->unum() == 3 ||(*tem)->unum() == 4 || (*tem)->unum() == 5|| (*tem)->unum() == 6))
        {
            if (target.size()>2)
                my_targrt = target[2]->pos();
            else
                my_targrt = Vector2D(0,0);
        }

    }



    if (!(my_targrt.dist(rcsc::Vector2D (0,0))<2 ))
    {
        /*target_point = (base + (my_targrt))/2;
      target_point = (target_point+(my_targrt) )/2;
      target_point = ( target_point+(my_targrt) )/2;*/
        target_point = getManToManMarkMove ( my_targrt , agent );
        //target_point = my_targrt;
        //target_point.x-=2;
    }
    else
        return false;


    if (my_targrt.dist(home) > 7 )
        return false;


    /*	for (int i = 0 ; i<4 ; i++)
    {
        if (wm.teammate(i+2) && (wm.teammate(i+2)->pos().x > target_point.x - 2.0))
            target_point.x = home.x ;

        if (wm.teammate(i+2) && (wm.teammate(i+2)->pos().x < target_point.x + 2.0) && wm.ball().pos().x < wm.teammate(i+2)->pos().x + 14)
            target_point.x = wm.teammate(i+2)->pos().x ;



    }*/



    double dash_power;
    if(wm.ball().vel().x > 0.0 && wm.ball().pos().x > wm.ourDefenseLineX() + 20 )
        dash_power=(rcsc::ServerParam::i().maxDashPower()/3);
    else
        dash_power = rcsc::ServerParam::i().maxDashPower();

    if ( wm.self().pos().dist( target_point ) < 7 )
        dash_power = rcsc::ServerParam::i().maxDashPower();

    // double dist_thr =  1.0;

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    rcsc::Vector2D turn_point = (target_point + wm.ball().pos())/2;

    agent->debugClient().addMessage( " BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );


    agent->doPointto ( target_point.x , target_point.y );
    if(target_point.dist(Strategy::i().getPosition( wm.self().unum()))>10){
        return false;
    }
    if ( ! rcsc::Body_GoToPoint( target_point, 0.5, dash_power
                                 ).execute( agent ) )
    {
        rcsc::Body_TurnToPoint(turn_point , 2).execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_ScanField() );
    }

    return true;
}



//////////////////////////////////////////////////////////////////////////////////////////////



double
Defense_MarkMove::getDashPower( rcsc::PlayerAgent * agent,
                                const rcsc::Vector2D & target_point )
{

    static bool s_recover_mode = false;

    const rcsc::WorldModel & wm = agent->world();


    if ( wm.self().pos().dist( target_point ) < 7 )
    {
        return rcsc::ServerParam::i().maxDashPower();
    }
    /*--------------------------------------------------------*/
    // check recover

    if ( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.5 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.7 )
    {
        s_recover_mode = false;
    }


    double dash_power =  rcsc::ServerParam::i().maxDashPower();

    if(wm.ball().vel().x > 0.0 && wm.ball().pos().x > wm.ourDefenseLineX() + 20 )
        dash_power=(rcsc::ServerParam::i().maxDashPower()/3);
    else
        dash_power = rcsc::ServerParam::i().maxDashPower();

    return dash_power;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////









void
Defense_MarkMove::get_mark_pos( rcsc::PlayerAgent * agent , vector<const rcsc::AbstractPlayerObject * > *mark_table)
{

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont & tems = wm.teammatesFromSelf();
    vector<rcsc::PlayerObject * > attacet_opponents;
    vector<rcsc::PlayerObject * > defender_teammates;
    vector<rcsc::PlayerObject * > dist_to_self_dangerous;
    vector<rcsc::PlayerObject * > dist_to_my_home_dangerous;
    vector<rcsc::PlayerObject * > dist_to_tem_dangerous;
    vector<rcsc::PlayerObject * > dist_to_our_goal_dangerous;

    for (uint i = 0 ; i < opps.size() ; i++)
        if ( opps[i]->pos().x < 0.0 && !opps[i]->isGhost () && opps[i]->posCount() < 10 )
        {
            attacet_opponents.push_back(opps[i]);
            dist_to_my_home_dangerous.push_back(opps[i]);
        }


    for (uint i = 0 ; i < opps.size() ; i++)
        if ( opps[i]->pos().x < 20.0  && !opps[i]->isGhost () && opps[i]->posCount() < 10 )
            dist_to_our_goal_dangerous.push_back(opps[i]);


    for (uint i = 0 ; i < tems.size() ; i++)
        if ( tems[i]->unum() == 2 || tems[i]->unum() == 3 ||
             tems[i]->unum() == 4 || tems[i]->unum() == 5 ||
             tems[i]->unum() == 6 || tems[i]->unum() == 7 ||
             tems[i]->unum() == 8 )
            if ( !tems[i]->isGhost () && tems[i]->posCount() < 10 )

                defender_teammates.push_back(tems[i] );


    for ( size_t i = attacet_opponents.size() ; i > 0 ; i-- )
        dist_to_self_dangerous.push_back(attacet_opponents[i-1]);


    for (uint i = 0 ; i < dist_to_my_home_dangerous.size() ; i++)
        for (uint j = 0 ; j < dist_to_my_home_dangerous.size() - 1 ; j++)
        {
            double opp1danger = dist_to_my_home_dangerous[j]->pos().dist(home);
            double  opp2danger  = dist_to_my_home_dangerous[j+1]->pos().dist(home);

            if (opp1danger < opp2danger)
            {
                rcsc::PlayerObject* temp = dist_to_my_home_dangerous[j + 1];
                dist_to_my_home_dangerous[j + 1] = dist_to_my_home_dangerous[j];
                dist_to_my_home_dangerous[j] = temp;
            }
        }


    for ( uint i = 0 ; i < dist_to_our_goal_dangerous.size() ; i++ )
        for (uint j = 0 ; j < dist_to_our_goal_dangerous.size() - 1 ; j++)
        {
            double opp1danger = min (
                        min ( dist_to_our_goal_dangerous[j]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y) ) ,
                              dist_to_our_goal_dangerous[j]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y)) )
                        ,

                        dist_to_our_goal_dangerous[j]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y)));


            double  opp2danger  = min (
                        min ( dist_to_our_goal_dangerous[j + 1]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y)),
                        dist_to_our_goal_dangerous[j + 1]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y)))
                    ,
                    dist_to_our_goal_dangerous[j + 1]->pos().dist(Vector2D(-52.5, dist_to_our_goal_dangerous[j]->pos().y)));

            if (opp1danger < opp2danger)
            {
                rcsc::PlayerObject* temp = dist_to_our_goal_dangerous[j + 1];
                dist_to_our_goal_dangerous[j + 1] = dist_to_our_goal_dangerous[j];
                dist_to_our_goal_dangerous[j] = temp;
            }
        }



    double min_dist = 1000.0 ;
    double  for_sort  [11]= {1000.0};

    for (uint i = 0 ; i < attacet_opponents.size() ; i++)
    {
        for (uint j = 0 ; j < defender_teammates.size()  ; j++)
            if (attacet_opponents[i]->pos().dist(defender_teammates[j]->pos()) < min_dist)
                min_dist = attacet_opponents[i]->pos().dist(defender_teammates[j]->pos());

        for_sort[i] = min_dist;

        min_dist = 1000.0 ;

    }

    min_dist = 1000.0 ;
    size_t temp = 20;
    for (size_t i = 0 ; i < attacet_opponents.size() ; i++)
    {
        for(size_t j=0 ; j < 11 ; j ++)
        {
            if (for_sort[j] > 900.0 || j > attacet_opponents.size() )
                continue;
            if (for_sort[j] < min_dist)
            {
                min_dist = for_sort[j];
                temp = j;
            }
        }

        for_sort[temp] = 1000.0;
        if (temp < attacet_opponents.size())
            dist_to_tem_dangerous.push_back(attacet_opponents[temp]);
        min_dist = 1000.0 ;

    }



    int opp_weigths[12] = {0};


    vector<rcsc::PlayerObject * > templatew = dist_to_our_goal_dangerous;


    for (uint i = 0 ; i < templatew.size() ; i++)
        if (templatew[i]->unum() != -1)
        {
            opp_weigths[templatew[i]->unum()] += i ;
        }
    //	std::cout<<wm.self().unum()<<"->";
    for (uint i = 0 ; i < dist_to_my_home_dangerous.size() ; i++)
        if (dist_to_my_home_dangerous[i]->unum() != -1)
        {
            opp_weigths[dist_to_my_home_dangerous[i]->unum()] += i ;
        }


    for (uint i = 0 ; i < dist_to_tem_dangerous.size() ; i++)
    {
        if (dist_to_tem_dangerous[i]->unum() != -1)
            opp_weigths[dist_to_tem_dangerous[i]->unum()] += i ;
    }






    temp = 20;
    int max_weight = 0;
    for(size_t i=1 ; i < 12 ; i ++)
    {
        for(size_t j=1 ; j < 12 ; j ++)
        {
            if (opp_weigths[j] == 0)  continue;
            if (opp_weigths[j] > max_weight)
            {
                max_weight = opp_weigths[j];
                temp = j;
            }
        }
        if (wm.theirPlayer(static_cast<int>(temp)))
        {
            mark_table->push_back( wm.theirPlayer(static_cast<int>(temp)));
        }
        opp_weigths[temp] = 0;
        max_weight = 0;

    }




}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

rcsc::Vector2D
Defense_MarkMove::get_mark_pos_defmid( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D  my_targrt(0,0);
    size_t i = 0 ;
    if (wm.opponentsFromSelf()[0])
        my_targrt =wm.opponentsFromSelf()[i]->pos();

    for (uint i = 0 ; i<2 && i<wm.opponentsFromSelf().size() + 1; i++)
    {
        if (wm.ourPlayer(6) && (wm.ourPlayer(6)->pos().dist(wm.opponentsFromSelf()[i]->pos()) <
                                wm.self().pos().dist(wm.opponentsFromSelf()[i]->pos())))
            if (wm.opponentsFromSelf()[i+1])
                my_targrt = wm.opponentsFromSelf()[i+1]->pos();
    }

    return my_targrt;

}

//////////////////////////////////////////////////////////////////////////////////////



bool 
Defense_MarkMove::ManToManMark ( rcsc::PlayerAgent * agent )
{
    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.85, 60.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();


    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->teammateReachCycle();

    if ( ( ! wm.existKickableTeammate() && self_min < 3 )
         || ( self_min < mate_min  && self_min <= opp_min  ) )
    {
        rcsc::Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    if ( ! wm.existKickableOpponent()
         && ! wm.existKickableTeammate()
         && self_min < mate_min
         && self_min < opp_min )
    {
        rcsc::Body_Intercept().execute( agent );
        //Arm_PointToPoint(Vector2D(0,0)).execute(agent);

        if ( wm.ball().distFromSelf()
             < rcsc::ServerParam::i().visibleDistance() )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }

        return true;
    }



    double dist_opp_to_home = 200.0;
    const rcsc::PlayerObject * opp
            = wm.getOpponentNearestTo( home, 1, &dist_opp_to_home );

    /////////////
    /*

       const rcsc::PlayerObject * p = static_cast< rcsc::PlayerObject * >( 0 );
       double min_dist2 = 40000.0;

       const rcsc::PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end()  ;
       for ( rcsc::PlayerPtrCont::const_iterator it  = wm.opponentsFromSelf().begin();
          it != end;
          ++it )
       {
         if ( (*it)->posCount() > 2 )
          {
            continue;
          }
         if ( (*it)->isKickable( ) )
          {
            continue;
          }

         double d2 = (*it)->pos().dist2(home);
         if ( d2 < min_dist2 )
          {
            p = *it;
            min_dist2 = d2;
          }
       }

       if ( p )
       {
        dist_opp_to_home = std::sqrt( min_dist2 );
       }

      const rcsc::PlayerObject * opp = p;
*/

    //////////////

    {
        const double dist_opp_to_ball = ( opp ? opp->distFromBall() : 100.0 );
        if ( wm.existKickableTeammate()
             && dist_opp_to_ball > 2.5 )
        {
            return false;
        }
    }

    rcsc::Vector2D nearest_opp_pos( rcsc::Vector2D::INVALIDATED );

    if ( opp )
    {
        nearest_opp_pos = opp->pos();
    }

    if ( !( nearest_opp_pos.isValid()
            && ( dist_opp_to_home < 7.0
                 || home.x > nearest_opp_pos.x ) ) )
    {
        return false;
    }

    {
        bool opp_is_not_danger
                = ( nearest_opp_pos.x > -35.0 || nearest_opp_pos.absY() > 17.0 );
        const double dist_opp_to_home2 = dist_opp_to_home * dist_opp_to_home;
        const rcsc::PlayerPtrCont::const_iterator end = wm.teammatesFromSelf().end();
        for ( rcsc::PlayerPtrCont::const_iterator
              it = wm.teammatesFromSelf().begin();
              it != end;
              ++it )
        {
            if((*it)->unum()<1)continue;

            if ( opp_is_not_danger
                 && (*it)->pos().x < wm.ourDefenseLineX() + 5.0 )
            {
                continue;
            }

            if ( (*it)->pos().dist2( nearest_opp_pos ) < dist_opp_to_home2 )
            {
                return false;
            }
        }
    }

    rcsc::AngleDeg mark_angle = ( wm.ball().pos() - nearest_opp_pos ).th();
    rcsc::Vector2D mark_point
            = nearest_opp_pos + rcsc::Vector2D::polar2vector( 0.2, mark_angle );
    mark_point.x -= 0.1;

    if ( mark_point.x < wm.self().pos().x - 1.5 )
    {
        mark_point.y = wm.self().pos().y;
    }


    double dash_power = rcsc::ServerParam::i().maxPower() * 0.9;
    double x_diff = mark_point.x - wm.self().pos().x;

    if ( x_diff > 20.0 )
    {
        dash_power *= 0.5;
    }
    else if ( x_diff > 10.0 )
    {
        dash_power *= 0.5;
    }
    else
    {
        dash_power *= 0.7;
    }

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;
    if(mark_point.dist(Strategy::i().getPosition( wm.self().unum()))>10){
        return false;
    }
    if (  rcsc::Body_GoToPoint( mark_point, dist_thr, dash_power ).execute( agent ) ){
        //Arm_PointToPoint(mark_point).execute(agent);
    }else
    {
        rcsc::Body_TurnToBall().execute( agent );
    }
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );

    return true;

}

bool
Defense_MarkMove::PassCutMark ( PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( /*wm.ball().pos().x < -14.0 ||*/ wm.existKickableTeammate () || wm.self().unum() > 8 )
    {
        return false;
    }
    if ( ! wm.existKickableOpponent()
         && ! wm.existKickableTeammate()
         && wm.interceptTable()->selfReachCycle() < wm.interceptTable()->teammateReachCycle()
         && wm.interceptTable()->selfReachCycle() < wm.interceptTable()->opponentReachCycle() )
    {
        rcsc::Body_Intercept().execute( agent );
        //Arm_PointToPoint(Vector2D(0,0)).execute(agent);
        if ( wm.ball().distFromSelf()
             < rcsc::ServerParam::i().visibleDistance() )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }

        return true;
    }

    if ( std::min ( wm.interceptTable()->selfReachCycle() ,  wm.interceptTable()->teammateReachCycle() )
         <   wm.interceptTable()->opponentReachCycle() )
    {
        return false;
    }

    const rcsc::AbstractPlayerObject * mark_target = getMarkTarget( wm );

    if ( ! mark_target )
    {
        return false;
    }

    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const rcsc::AbstractPlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();

    if ( mark_target == fastest_opp )
    {
        return false;
    }

    if ( mark_target->isGhost() )
    {
        if ( wm.ball().distFromSelf() < 20.0
             && ( wm.existKickableOpponent()
                  || ( opp_min <= 3
                       && fastest_opp->distFromBall() < 3.0 )
                  )
             )
        {
            return false;
        }

        if ( mark_target->ghostCount() >= 2 )
        {
            return false;
        }

        rcsc::Bhv_BodyNeckToPoint( mark_target->pos() ).execute( agent );
        return true;
    }

    if ( mark_target->posCount() >= 5 )
    {
        return rcsc::Bhv_NeckBodyToPoint( mark_target->pos() ).execute( agent );
    }

    rcsc::Vector2D target_point = getMarkPosition( wm, mark_target );
    if(target_point.dist(Strategy::i().getPosition( wm.self().unum()))>10){
        return false;
    }
    if ( ! target_point.isValid() )
    {
        return false;
    }

    double dash_power = getDashPower( agent , target_point);
    double dist_thr = mark_target->pos().dist( target_point ) * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    if ( rcsc::Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) ){
        //Arm_PointToPoint(target_point).execute(agent);
    }else
    {
        AngleDeg body_angle = ( wm.ball().pos().y < wm.self().pos().y
                                ? -90.0
                                : 90.0 );
        rcsc::Body_TurnToAngle( body_angle ).execute( agent );
    }

    if ( mark_target->posCount() >= 3 )
    {
        rcsc::Vector2D face_point = mark_target->pos() + mark_target->vel();
        rcsc::Vector2D next_self_pos = wm.self().pos() + wm.self().vel();
        rcsc::AngleDeg neck_angle = ( face_point - next_self_pos ).th();
        neck_angle -= wm.self().body();


        double view_half_width = agent->effector().queuedNextViewWidth().width();
        double neck_min = ServerParam::i().minNeckAngle() - ( view_half_width - 20.0 );
        double neck_max = ServerParam::i().maxNeckAngle() + ( view_half_width - 20.0 );

        if ( neck_min < neck_angle.degree()
             && neck_angle.degree() < neck_max )
        {
            neck_angle = rcsc::ServerParam::i().normalizeNeckAngle( neck_angle.degree() );
        }
        else
        {
            rcsc::Bhv_NeckBodyToPoint( face_point ).execute( agent );
        }
    }
    else
    {
        int count_thr = -1;
        switch ( agent->effector().queuedNextViewWidth().type() ) {
        case ViewWidth::NARROW:
            count_thr = 1;
            break;
        case ViewWidth::NORMAL:
            count_thr = 2;
            break;
        case ViewWidth::WIDE:
            count_thr = 3;
            break;
        default:
            break;
        }

        agent->setNeckAction( new Neck_TurnToBallOrScan( count_thr ) );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
const
rcsc::AbstractPlayerObject *
Defense_MarkMove::getMarkTarget( const rcsc::WorldModel & wm )
{
    const rcsc::Vector2D home_pos = home;
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );

    const rcsc::PlayerObject * nearest_attacker = static_cast< const PlayerObject * >( 0 );
    const rcsc::PlayerObject * outside_attacker = static_cast< const PlayerObject * >( 0 );
    double min_dist2 = 100000.0;

    const rcsc::PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin();
          it != end;
          ++it )
    {
        if((*it)->unum()<1)continue;

        bool same_side = false;
        if ( position_type == Position_Center
             || ( position_type == Position_Left
                  && (*it)->pos().y < 0.0 )
             || ( position_type == Position_Right
                  && (*it)->pos().y > 0.0 )
             )
        {
            same_side = true;
        }

        if ( same_side
             && (*it)->pos().x < wm.ourDefenseLineX() + 17.0
             && (*it)->pos().absY() > home_pos.absY() - 5.0 )
        {
            outside_attacker = *it;
        }

        if ( (*it)->pos().x > wm.ourDefenseLineX() + 10.0 )
        {
            if ( (*it)->pos().x > home_pos.x ) continue;
        }
        if ( (*it)->pos().x > home_pos.x + 10.0 ) continue;
        if ( (*it)->pos().x < home_pos.x - 20.0 ) continue;
        if ( std::fabs( (*it)->pos().y - home_pos.y ) > 20.0 ) continue;

        double d2 = (*it)->pos().dist( home_pos );
        if ( d2 < min_dist2 )
        {
            min_dist2 = d2;
            nearest_attacker = (*it);
        }
    }

    if ( outside_attacker
         && nearest_attacker != outside_attacker )
    {
        return static_cast< rcsc::PlayerObject * >( 0 );
    }

    if ( nearest_attacker )
    {
        return nearest_attacker;
    }

    return static_cast< rcsc::PlayerObject * >( 0 );

}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Vector2D
Defense_MarkMove::getMarkPosition( const rcsc::WorldModel & wm,
                                   const AbstractPlayerObject * target_player )
{
    if ( ! target_player )
    {
        return rcsc::Vector2D::INVALIDATED;
    }

    rcsc::Vector2D target_point = rcsc::Vector2D::INVALIDATED;

    const rcsc::Vector2D home_pos = home;//Strategy::i().getPosition( wm.self().unum() );

    if ( home_pos.x - target_player->pos().x > 5.0 )
    {
        target_point
                = target_player->pos()
                + ( wm.ball().pos() - target_player->pos() ).setLengthVector( 1.0 );

    }
    else if ( wm.ball().pos().x - target_player->pos().x > 15.0 )
    {
        target_point
                = target_player->pos()
                + ( wm.ball().pos() - target_player->pos() ).setLengthVector( 1.0 );
    }
    else
    {
        const Vector2D goal_pos( -50.0, 0.0 );

        target_point
                = target_player->pos()
                + ( goal_pos - target_player->pos() ).setLengthVector( 1.0 );
    }

    return target_point;
}


bool
Defense_MarkMove::danger_mark ( rcsc::PlayerAgent * agent )
{

    rcsc::dlog.addText( rcsc::Logger::TEAM,
                        __FILE__": Bhv_BasicMove" );

    // tackle

    //    Bhv_MiracleBlock().execute( agent );

    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.8, 90.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();

    // check ball owner

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ( ! wm.existKickableTeammate() && self_min < 4 )
         || ( self_min < mate_min  ) )
    {
        if( wm.self().unum()==2||wm.self().unum()==3||wm.self().unum()==7||wm.self().unum()==8
                || wm.self().unum()==4||wm.self().unum()==5||wm.self().unum()==6)
        {
            if(wm.ball().pos().x<-10||self_min<opp_min){
                //Arm_PointToPoint(Vector2D(0,0)).execute(agent);
                Body_Intercept().execute( agent );
                agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
                return true;
            }
        }
        else{
            Body_Intercept().execute( agent );
            //Arm_PointToPoint(Vector2D(0,0)).execute(agent);
            agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
            return true;
        }
    }


    // go to home position


    rcsc::Vector2D  my_targrt(0,0);
    rcsc::Vector2D target_point = home , base = wm.ball().pos();
    vector<const rcsc::AbstractPlayerObject * > target;


    get_mark_pos(agent , &target);


    if (target.size()>0){
        my_targrt = target[0]->pos();


    }
    else
        return false;

    const rcsc::PlayerPtrCont & tems = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator t_end = tems.end();
    uint index = 0;
    bool flag1 = false , flag2 = false;
    //for(int i = 0; i<8 ;i++)
    for ( rcsc::PlayerPtrCont::const_iterator tem = tems.begin(); tem != t_end ; ++tem )
    {
        if((*tem)->unum()<1)continue;

        if ( (*tem)->goalie() )                   continue;
        if ( (*tem)->isGhost()  )                 continue;
        if ( (*tem)->unum() == wm.self().unum() ) continue;
        if ( (*tem)->posCount() > 10 )            continue;


        if ((*tem)->pos().dist(my_targrt) < wm.self().pos().dist(my_targrt) -4 &&  index + 1 < target.size() )
        {
            if (!flag1)
            {
                if (wm.ball().pos().x > -40 && wm.self().pos().x > -40)
                    flag1 = true;
            }
            else
            {
                if (!flag2)
                {
                    if (wm.ball().pos().x > -40 && wm.self().pos().x > -40)
                        flag2 = true;
                }
            }
            if (flag1 && flag2 &&  index + 1 < target.size())
            {
                my_targrt = target[index+1]->pos();
                index ++;
            }
        }
    }


    if (!(my_targrt.dist(rcsc::Vector2D (0,0))<2 ))
    {

        target_point = (base + (my_targrt))/2;
        target_point = (target_point+(my_targrt) )/2;
        target_point = (target_point+(my_targrt) )/2;
        target_point = (target_point+(my_targrt) )/2;

        target_point = getManToManMarkMove ( my_targrt , agent );

        target_point.x -= 4;
    }
    else
        return false;

    if (wm.ball().pos().x > -40 && wm.self().pos().x > -40 )
    {
        if (target_point.dist(home) > 5 )
            return false;
    }
    else
        if (target_point.dist(home) > 4 )
            return false;

    const double dash_power =getDashPower(agent , target_point);

    double dist_thr = 1.0;
    agent->debugClient().addMessage( " BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );

    agent->doPointto ( target_point.x , target_point.y );
    //std::cout<<"\n\n\n MAAAAAAAAAAAAAAAAAAAAAARK ";

    if (  rcsc::Body_GoToPoint( target_point, dist_thr, dash_power
                                ).execute( agent ) ){
        //Arm_PointToPoint(target_point).execute(agent);
    }else
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 10.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }

    return true;


}

bool Defense_MarkMove::cross_mark ( rcsc::PlayerAgent * agent )
{
    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.7, 70.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();

    // check ball owner

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min < mate_min + 3
                   && self_min < opp_min + 4 )
              )
         )
    {
        rcsc::dlog.addText( rcsc::Logger::TEAM,
                            __FILE__": (execute) intercept" );
        rcsc::Body_Intercept().execute( agent );
        if ( wm.ball().distFromSelf()
             < rcsc::ServerParam::i().visibleDistance() )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );;
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }
        return true;
    }

    rcsc::Vector2D target_point = home;
    const rcsc::Vector2D HOME = home;
    double dist;
    const rcsc::PlayerObject * opp = wm.getOpponentNearestTo ( HOME , 10 , &dist );
    if ( opp->pos().isValid () )
        target_point = opp->pos( );
    else
        return false;

    target_point = getManToManMarkMove ( target_point , agent );

    const double dash_power =getDashPower(agent , target_point);

    double dist_thr = 1.0;
    agent->debugClient().addMessage( " BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );

    if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent() && wm.ball().distFromSelf() < 10.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }

    return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////



bool
Defense_MarkMove::basicMove( rcsc::PlayerAgent * agent )
{
    rcsc::dlog.addText( rcsc::Logger::TEAM,
                        __FILE__": BasicMove" );

    // tackle
    if((agent->world().ball().pos().x<-10)||agent->world().interceptTable()->selfReachCycle()>1)
        if ( Bhv_BasicTackle( 0.5, 60.0 ).execute( agent ) )
        {
            return true;
        }

    const rcsc::WorldModel & wm = agent->world();

    // check ball owner

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min < mate_min + 3
                   && self_min < opp_min + 4 )
              )
         )
    {
        rcsc::dlog.addText( rcsc::Logger::TEAM,
                            __FILE__": (execute) intercept" );
        rcsc::Body_Intercept().execute( agent );
        if ( wm.ball().distFromSelf()
             < rcsc::ServerParam::i().visibleDistance() )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );;
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }
        return true;
    }

    // go to home position

    const double dash_power = getDashPower( agent, home );
    //  const double dash_power =rcsc::ServerParam::i().maxDashPower();
    Vector2D target_point = home;
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    agent->debugClient().addMessage( " BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );


    if ( ! rcsc::Body_GoToPoint( target_point , dist_thr, dash_power
                                 ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }

    return true;
}

/*bool Defense_MarkMove::PassCutMove ( rcsc::PlayerAgent * agent , rcsc::Vector2D & target )
{

}*/

rcsc::Vector2D Defense_MarkMove::getManToManMarkMove ( rcsc::Vector2D tmp , rcsc::PlayerAgent * agent )
{

    rcsc::Vector2D target ;
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D posAgent = agent->world().self().pos();
    rcsc::Vector2D posBall = agent->world().ball().pos();
    rcsc::Vector2D center ( -52.5 , 0 );

    double MARK_DIST = 7.0;
    if ( Strategy::get_ball_area( tmp  ) == Strategy::BA_Danger ) MARK_DIST = 1.5;

    {
        target = tmp;
        target += rcsc::Vector2D::polar2vector( MARK_DIST , ( center-target ).th() );
        if ( Strategy::get_ball_area( tmp  ) == Strategy::BA_Danger )
        {
            target = tmp;
            double th = ( center-target ).th().degree ( );
            th = ( th + (wm.ball().pos()-target).th().degree() ) / 2;
            th = ( th + ( center-target ).th().degree ( ) ) / 2;
            th = ( th + ( center-target ).th().degree ( ) ) / 2;
            target += rcsc::Vector2D::polar2vector( MARK_DIST , AngleDeg ( th ) );
            //target += rcsc::Vector2D::polar2vector( MARK_DIST , (wm.ball().pos()-tmp).th() );
        }
        /*if(  target.x > posBall.x + 20 )
     {
      return false;
     }

    if( Strategy::get_ball_area( target ) == Strategy::BA_ShootChance )     return false;
    else if( target.dist( posBall ) < 5.0 )                                 return false;*/
    }
    if ( tmp.x > -33 && tmp.x < -15 )
    {

        if ( !wm.existKickableOpponent ( ) && !wm.existKickableTeammate ( ) )
        {
            //if ( PassCutMove ( agent , target ) )
            //   return target;

            rcsc::Ray2D  moveRay  ( tmp , ( center-target ).th() );
            rcsc::Line2D passLine ( posBall , wm.ball().vel().th() );
            rcsc::Vector2D tmpTarget = moveRay.intersection (passLine);
            if ( tmpTarget.isValid() )
            {
                if ( tmpTarget.dist(tmp) < 4 )
                    target = tmpTarget+rcsc::Vector2D::polar2vector( MARK_DIST , ( tmpTarget-tmp ).th() );
                else
                    target = tmpTarget;
            }
        }

    }
    return target;



}

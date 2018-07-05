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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"

#include "sample_field_evaluator.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/common/audio_memory.h>
#include "chain_action/action_chain_graph.h"

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"
#include "bhv_defense_mark_move.h"
#include "bhv_cyrus_block.h"
#include "bhv_cyrus_mark.h"

#include "bhv_pressing.h"
#include "bhv_position.h"
#include "cyrus.h"
int Bhv_BasicMove::lastCycle;
Vector2D Bhv_BasicMove::lastTarget;

int Bhv_BasicMove::sums=0;
int Bhv_BasicMove::sumto=0;
int Bhv_BasicMove::numSums=0;
int Bhv_BasicMove::numSumto=0;


using namespace rcsc;
using namespace std;
/*-------------------------------------------------------------------*/
/*!

 */
bool Bhv_BasicMove::execute(PlayerAgent * agent) {

    dlog.addText(Logger::TEAM, __FILE__": Bhv_BasicMove");
    const WorldModel & wm = agent->world();


    vector<ActionStatePair> chain = ActionChainGraph::My_result;

    for (vector<ActionStatePair>::const_iterator it = chain.begin();it != chain.end(); ++it) {

        //teammates
        int unum = (*it).statePtr()->ballHolderUnum();

        if(unum > 0 ){

            if((*it).actionPtr()->category()==CooperativeAction::Pass){//2
                if( wm.self().unum()==5 && wm.ourPlayer(unum)!=NULL && wm.ourPlayer(unum)->posCount()!=1000 ){
                    sums+=wm.ourPlayer(unum)->posCount();
                    numSums++;
                }
                if(it+1 != chain.end()){
                    int unumNextBallHolder=(*(it+1)).statePtr()->ballHolderUnum();
                    if(unumNextBallHolder>0 && wm.ourPlayer(unumNextBallHolder)!=NULL && wm.ourPlayer(unumNextBallHolder)->posCount()!=1000){
                        if(wm.self().unum()==5){
                            sumto+=wm.ourPlayer(unumNextBallHolder)->posCount();
                            numSumto++;
                        }

                    }
                }

            }

        }
    }

    //	if(wm.self().unum()==5)	std::cout<<wm.time().cycle()<<" u "<<wm.self().unum()<<" n "<< numSums<<" s "<<sums<<" nt "<<numSumto<<"  st  "<<sumto<<" \n ";

    bool empty;
    int viewUnum=cyrus().indexOfMaxEvalForView(agent, &empty);

    Vector2D viewPos=wm.ball().pos();
    if( viewUnum == 0 ){
        viewPos=wm.ball().pos();
    }
    else if( viewUnum <= 11 ){
        if(wm.ourPlayer(viewUnum)!=NULL)
            viewPos=wm.ourPlayer(viewUnum)->pos();
    }
    else if( viewUnum > 11 ){
        if(wm.theirPlayer(viewUnum-11)!=NULL)
            viewPos=wm.theirPlayer(viewUnum-11)->pos();
    }
    if(!empty)Arm_PointToPoint(viewPos).execute(agent);

    bool vvv = true;
    if(!empty){
        vvv=false;
        agent->setNeckAction(new Neck_TurnToPoint(viewPos));
    }


    if (bhv_position::last_cycle > 0)
        bhv_position::last_cycle--;

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const int our_min = std::min(self_min, mate_min);
    //////say msg//////

    //
    // say recovery msg to other player for dont lead pass 2 me
    //
    if (wm.self().stamina() < 3500
            && wm.time().cycle() % 5 == wm.self().unum() % 5
            && mate_min < opp_min + 1) {
        agent->addSayMessage(new StaminaMessage(wm.self().stamina()));
        //		//Arm_PointToPoint(Vector2D(50, 30)).execute(agent);
    }

    if (self_min < mate_min && self_min <= opp_min
            && !wm.existKickableTeammate()) {
        doIntercept(agent);

        if (wm.self().pos().x > 35 && wm.self().pos().absY() < 10) {
            if(wm.theirPlayer(wm.theirGoalieUnum())->posCount()>1)
                agent->setNeckAction(new Neck_TurnToGoalieOrScan());
            else
                agent->setNeckAction(new Neck_TurnToBallOrScan());
        } else if (wm.existKickableOpponent()
                   && wm.ball().distFromSelf() < 18.0) {
            if(vvv)agent->setNeckAction(new Neck_TurnToBall());
        } else {
            if(vvv)agent->setNeckAction(new Neck_TurnToBallOrScan());
        }
        return true;
    }
    Vector2D ball = wm.ball().pos();
    Vector2D me = wm.self().pos();
    Vector2D target_point = Strategy::i().getPosition(wm.self().unum());

    if ((self_min < mate_min && self_min <= opp_min)) {
        doIntercept(agent);
        if(vvv)agent->setNeckAction(new Neck_TurnToBallOrScan());
        return true;
    }

    if (((wm.ball().pos().x < -10) || self_min > 1)
            || ServerParam::i().theirPenaltyArea().contains(wm.ball().pos()))

        if (mate_min > opp_min && !wm.existKickableTeammate()
                && wm.ball().pos().x < -10)
            if (Bhv_BasicTackle(0.85, 80.0).execute(agent))
                return true;

    //-----------------------------------------------
    // tackle

    Vector2D next_ball = wm.ball().inertiaPoint(
                wm.interceptTable()->opponentReachCycle());
    Vector2D my_pos = wm.self().pos() + wm.self().pos();
    if (next_ball.x > 25 && wm.self().unum() > 8) {
        if (opp_min < std::min(self_min, mate_min) - 3) {
            if (Bhv_BasicTackle(0.8, 80.0).execute(agent)) {
                return true;
            }
        }
    } else if (wm.self().unum() < 6) {
        if ((next_ball.x - my_pos.x) < 0 || (opp_min - our_min) > 2
                || ball.x > -10.0) {
            if (Bhv_BasicTackle(0.8, 80.0).execute(agent)) {
                return true;
            }
        } else {
            if (Bhv_BasicTackle(1.0, 80.0).execute(agent)) {
                return true;
            }
        }

    }

    else {
        if (Bhv_BasicTackle(0.8, 80.0).execute(agent)) {
            return true;
        }
    }
    // turn for tackle
    {
        rcsc::Vector2D nextBall = ball + wm.ball().vel();

        if (wm.ball().vel().r() > 1.8
                && nextBall.dist(me + wm.self().vel()) < 1.9
                && wm.lastKickerSide() != wm.ourSide()
                && nextBall.dist(me + wm.self().vel())
                > wm.self().playerType().kickableArea()) {
            rcsc::Body_TurnToPoint(nextBall).execute(agent);
            agent->setNeckAction(new Neck_TurnToBall());
            return true;
        }
    }
    /*--------------------------------------------------------*/
    bool def_plan_A = false;
    bool def_plan_B = true;
    bool def_plan_C = false;
    bool def_plan_D = false;

    int defthr = 0;
    if (wm.ball().pos().x < 10 && wm.self().unum() < 7)
        defthr = 1;
    if (opp_min <= self_min + defthr && opp_min <= mate_min + defthr/*true*/) {
        // def plan A, new mark and block
        if (def_plan_A) {
        }
        //def plan B: just new block
        else if (def_plan_B /*&& wm.time().cycle() < 3000*/) {
            /*cout << "****" << wm.time().cycle() << "****" << "def plan b"
             << endl;*/
            int blocker = 0;
            if (!(opp_min + 1 > our_min && wm.ball().pos().x > 10
                  && wm.self().unum() > 8)
                    && ((opp_min <= self_min && opp_min <= mate_min)
                        || (opp_min <= our_min - 2 && wm.self().unum() < 6
                            && wm.ball().inertiaPoint(opp_min).x < 0))) {
                blocker = bhv_cyrus_block().decision(agent,
                                                     vector<int>(12, 0));
                if (blocker == wm.self().unum()) {
                    return true;
                } else {
                    if (ball.x > wm.self().pos().x && danger_move(agent))
                        return true;
                    vector<int> tmwork(12, 0);
                    tmwork[static_cast<size_t>(blocker)] = 1;
                    tmwork[2] = 1;
                    tmwork[3] = 1;
                    tmwork[4] = 1;
                    tmwork[5] = 1;
                    if (bhv_cyrus_mark().execute(agent, tmwork))
                        return true;
                    else if (danger_move(agent))
                        return true;
                    else {
                        Vector2D home = Strategy::i().getPosition(
                                    wm.self().unum());
                        double dist_thr = wm.ball().pos().dist(home) / 20;
                        Body_GoToPoint2010(home, dist_thr, 100, 1.5, 100, false,
                                           15).execute(agent);
                    }
                }

            }

            if (danger_move(agent))
                return true;
        }

        //def plan C, old mark and new block
        else if (def_plan_C /*&& wm.time().cycle() > 3000*/) {
        }

        //def plan D, old mark and block
        else if (def_plan_D) {
        }
    }
    const double dash_power = Strategy::get_normal_dash_power(wm);

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if (dist_thr < 1.0)
        dist_thr = 1.0;

    if (bhv_position().updatePos(agent)) {
        if (wm.self().pos().x > 35 && wm.self().pos().absY() < 12) {
            agent->setNeckAction(new Neck_TurnToGoalieOrScan());
        } else
            agent->setNeckAction(new Neck_TurnToBall());
        return true;
    }

    dlog.addText(Logger::TEAM,
                 __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                 target_point.x, target_point.y, dist_thr);

    agent->debugClient().addMessage("BasicMove%.0f", dash_power);
    agent->debugClient().setTarget(target_point);
    agent->debugClient().addCircle(target_point, dist_thr);

    if (!Body_GoToPoint(target_point, dist_thr, dash_power).execute(agent)) {
        Body_TurnToBall().execute(agent);
    }

    if(vvv){
        if (wm.self().pos().x > 35 && wm.self().pos().absY() < 10) {
            agent->setNeckAction(new Neck_TurnToGoalieOrScan());
        } else if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0) {
            agent->setNeckAction(new Neck_TurnToBall());
        } else {
            agent->setNeckAction(new Neck_TurnToBallOrScan());
        }
    }

    return true;
}

bool Bhv_BasicMove::doIntercept(PlayerAgent * agent) {
    if (Body_Intercept2009(false).execute(agent)) {
        agent->setNeckAction(new Neck_TurnToBallOrScan());
        return true;
    }
    return false;
}

bool Bhv_BasicMove::UltimateGap(PlayerAgent * agent) {

    return false;

}

bool Bhv_BasicMove::unMark(rcsc::PlayerAgent * agent) {
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    if (wm.self().isKickable()) {
        return false;
    }

    if (! (wm.interceptTable()->teammateReachCycle()
            > wm.interceptTable()->opponentReachCycle())) {
        return false;
    }

    if (wm.self().pos().dist(wm.ball().pos()) > 50) {
        return false;
    }

    const PlayerObject * opp = wm.getOpponentNearestToSelf(10, false);
    if (!opp) {
        return false;
    }
    if (opp->distFromSelf() < 5) {

        Vector2D target = lastTarget;
        if (wm.time().cycle() - lastCycle > 4
                || lastTarget.x > wm.offsideLineX()) {

            //			std::cout<<wm.time()<<"unmark"<<endl;
            //			std::cout<<"** num "<<wm.self().unum()<<endl;

            ////////*******************//////// find good targets

            double magValue = wm.self().playerType().playerSpeedMax();
            double magValue1 = magValue * wm.self().playerType().playerDecay();
            double magvalue2 = magValue1 * wm.self().playerType().playerDecay();

            magValue = magValue + magValue1 + magvalue2;
            double magValue2 = opp->distFromSelf();

            double radius = std::max(magValue, magValue2);

            vector<Vector2D> targets;
            vector<double> values;
            double dist_to_go = radius;

            Vector2D addTo1 = Vector2D::polar2vector(radius,
                                                     (wm.self().pos() - opp->pos()).dir());

            /*	targets[0] =*/
            Vector2D target0 = Vector2D(wm.self().pos() + addTo1);
            dist_to_go = std::max(radius, wm.self().pos().dist(target0));
            target0 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target0 - wm.self().pos()).dir());
            //		targets.push_back(target0);

            ////////////////////////// baine toop va bazikon harif vaysa

            Vector2D addTo2 = Vector2D::polar2vector(radius,
                                                     (wm.ball().pos() - opp->pos()).dir());
            Vector2D target1 = Vector2D(opp->pos() + addTo2);
            dist_to_go = std::max(radius, wm.self().pos().dist(target1));
            target1 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target1 - wm.self().pos()).dir());
            /*targets[1]=*/
            targets.push_back(target1);

            Line2D oppBall = Line2D(opp->pos(), wm.ball().pos());
            Line2D oppBallAmood = oppBall.perpendicular(opp->pos());

            Circle2D oppCircle = Circle2D(opp->pos(), radius);

            Vector2D target2;
            Vector2D target3;
            oppCircle.intersection(oppBallAmood, &target2, &target3);

            dist_to_go = std::max(radius, wm.self().pos().dist(target2));
            target2 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target2 - wm.self().pos()).dir());
            targets.push_back(target2);

            dist_to_go = std::max(radius, wm.self().pos().dist(target3));
            target3 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target3 - wm.self().pos()).dir());
            targets.push_back(target3);

            //////////////////////////2 ta ba zavie 45 daraje ham ezafe kon

            dist_to_go = radius; //std::max(dist_to_go,wm.self().pos().dist(target4));
            Vector2D target4 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target1 - wm.self().pos()).dir() + 45.0);
            targets.push_back(target4);

            dist_to_go = radius; //std::max(dist_to_go,wm.self().pos().dist(target5));
            Vector2D target5 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target1 - wm.self().pos()).dir() - 45.0);
            targets.push_back(target5);

            ////////////2 ta 90 daraje badan harif

            Vector2D target6 = opp->pos()
                    + Vector2D::polar2vector(radius, opp->body() + 90.0);
            dist_to_go = std::max(radius, wm.self().pos().dist(target6));
            target6 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target6 - wm.self().pos()).dir());
            targets.push_back(target6);

            Vector2D target7 = opp->pos()
                    + Vector2D::polar2vector(radius, opp->body() - 90.0);
            dist_to_go = std::max(radius, wm.self().pos().dist(target7));
            target7 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target7 - wm.self().pos()).dir());
            targets.push_back(target7);

            ///////////////////////////// 2 ta 90 daraje toop

            Vector2D target8 = wm.self().pos()
                    + Vector2D::polar2vector(radius,
                                             (wm.self().pos() - wm.ball().pos()).dir() + 90.0);
            dist_to_go = std::max(radius, wm.self().pos().dist(target8));
            target8 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target8 - wm.self().pos()).dir());
            targets.push_back(target8);

            Vector2D target9 = wm.self().pos()
                    + Vector2D::polar2vector(radius,
                                             (wm.self().pos() - wm.ball().pos()).dir() - 90.0);
            dist_to_go = std::max(radius, wm.self().pos().dist(target9));
            target9 = wm.self().pos()
                    + Vector2D::polar2vector(dist_to_go,
                                             (target9 - wm.self().pos()).dir());
            targets.push_back(target9);

            ////////*******************//////// add 16 target around self

            for (int a = 0; a < 16; ++a) {
                AngleDeg dash_angle = wm.self().body() + (22.5 * a);
                Vector2D tempTarget = wm.self().pos()
                        + Vector2D::polar2vector(radius, dash_angle);
                targets.push_back(tempTarget);
            }

            ////////*******************//////// count evaluation for targets

            for (size_t i = 0; i < targets.size(); i++) {

                double tmpValue = 0;
                /*if(targets[i].dist(wm.ball().pos())< opp->pos().dist(wm.ball().pos()))
                 {
                 tmpValue+=10;
                 }*/
                if ((((targets[i] - opp->pos()).dir() - opp->body()).abs())
                        > 70.0) {
                    tmpValue += 20;
                }

                Vector2D middleToBall = (targets[i] + wm.ball().pos()) / 2;
                Circle2D circle(middleToBall, middleToBall.dist(targets[i]));
                double oppminDist = 40;

                Triangle2D goalTargetTriangle = Triangle2D(targets[i],
                                                           Vector2D(SP.pitchHalfLength(), SP.goalHalfWidth()),
                                                           Vector2D(SP.pitchHalfLength(), -SP.goalHalfWidth()));
                int oppCount = 0;

                if (!wm.opponents().empty()) {
                    const PlayerPtrCont::const_iterator o_end =
                            wm.opponentsFromSelf().end();
                    for (PlayerPtrCont::const_iterator o =
                         wm.opponentsFromSelf().begin(); o != o_end; ++o) {
                        if (circle.contains((*o)->pos())
                                && targets[i].dist((*o)->pos()) < oppminDist) {
                            oppminDist = targets[i].dist((*o)->pos());
                        }
                        if (goalTargetTriangle.contains((*o)->pos())) {
                            oppCount++;
                        }
                    }
                }

                tmpValue = +(oppminDist / 3);
                tmpValue = -(oppCount * 3);

                //////////////////////////////////////////////////////////////
                Line2D meBall = Line2D(wm.self().pos(), wm.ball().pos());

                bool successMe = true;
                bool successBall = true;
                const PlayerObject * oppBall = wm.getOpponentNearestToBall(10,
                                                                           true);
                const PlayerObject * oppMe = wm.getOpponentNearestToSelf(10,
                                                                         true);
                Vector2D middleballme((wm.self().pos() + wm.ball().pos()) / 2);
                Circle2D meBallcircle(middleballme,
                                      middleballme.dist(wm.ball().pos()));
                if (oppBall) {
                    Line2D tO(targets[i], oppBall->pos());
                    if (meBallcircle.contains(tO.intersection(meBall))) {
                        successBall = false;
                    }
                }
                if (oppMe) {
                    Line2D tO(targets[i], oppMe->pos());
                    if (meBallcircle.contains(tO.intersection(meBall))) {
                        successMe = false;
                    }
                }

                if (successBall && successMe) {
                    tmpValue += 20;
                }

                /////////////////////////////////////////////////

                double tmp = 0;

                tmpValue += SampleFieldEvaluator().evaluator(targets[i]) / 10;

                if (!wm.getOpponentNearestTo(targets[i], 30, &tmp)) {
                    tmp = 0;
                }
                /*values[i]=*/
                values.push_back(
                            tmpValue + tmp - targets[i].dist(wm.ball().pos()));

            }

            ////////*******************//////// find best target

            target = wm.self().pos();
            double value = 0;
            //std::cout<<targets[0]<<" 0:"<< values[0]<<endl;
            for (size_t i = 0; i < targets.size(); ++i) {
                //std::cout<<targets[i]<<" "<<i<<":"<< values[i]<<endl;
                ////////*******************//////// if dist to opp will be less dont check

                double nowValue;
                double targetValue;
                if (!wm.getOpponentNearestTo(wm.self().pos(), 30, &nowValue))
                    nowValue = 0;
                if (!wm.getOpponentNearestTo(targets[i], 30, &targetValue))
                    targetValue = 0;
                if (targetValue < nowValue) {
                    continue;
                }
                if (Strategy::i().getPosition(wm.self().unum()).dist(targets[i])
                        > 5) {
                    continue;
                }
                if (targets[i].x > wm.offsideLineX()) {
                    continue;
                }
                if ((predict_player_turn_cycle(agent, wm.self().body(),
                                               wm.self().vel().r(), wm.self().pos().dist(targets[i]),
                                               (targets[i] - wm.self().pos()).dir(), 0.5,
                                               true/*,	opp->pos()*/)) > 1) {
                    continue;

                }
                ////////*******************//////// now compare
                if (values[i] > value) {
                    value = values[i];
                    target = targets[i];
                }
            }

            if (value == 0) {
                return false;
            }
            ////////*******************//////// move to target

            lastTarget = target;
            lastCycle = static_cast<int>(wm.time().cycle());

        }
        //		//Arm_PointToPoint(target).execute(agent);
        Body_GoToPoint(target, 0.5, ServerParam::i().maxDashPower(),
                       ServerParam::i().maxDashPower(), 4, true, 5.0).execute(agent);

        agent->setNeckAction(new Neck_TurnToBallOrScan());

        //		std::cout<<"#############final target  "<<target<<endl;
        return true;

    }

    return false;
}

int Bhv_BasicMove::predict_player_turn_cycle(
        PlayerAgent * agent,				//me
        const AngleDeg & player_body, double player_speed, double target_dist,
        AngleDeg target_angle, double dist_thr, bool use_back_dash
        /*const Vector2D & new_opp_pos*/) {
    const WorldModel & wm = agent->world();
    const ServerParam & SP = ServerParam::i();

    int n_turn = 0;

    double angle_diff = (target_angle - player_body).abs();

    if (use_back_dash && target_dist < 5.0 // Magic Number
            && angle_diff > 90.0 && SP.minDashPower() < -SP.maxDashPower() + 1.0) {
        angle_diff = std::fabs(angle_diff - 180.0);  // assume backward dash
    }

    double turn_margin = 180.0;
    if (dist_thr < target_dist) {
        turn_margin = std::max(15.0, // Magic Number
                               rcsc::AngleDeg::asin_deg(dist_thr / target_dist));
    }

    double speed = player_speed;
    while (angle_diff > turn_margin) {
        angle_diff -= wm.self().playerType().effectiveTurn(SP.maxMoment(),
                                                           speed);
        speed *= wm.self().playerType().playerDecay();
        //new_opp_pos += Vector2D::polar2vector(speed,player_body);
        ++n_turn;
    }

    return n_turn;
}

bool Bhv_BasicMove::danger_move(PlayerAgent * agent) {
    const WorldModel & wm = agent->world();
    Vector2D ball = wm.ball().inertiaPoint(
                wm.interceptTable()->opponentReachCycle());
    Vector2D me = wm.self().pos();
    int my_unum = wm.self().unum();
    Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    /*if (ball.x < -36.0 && ball.absY() < 18 && me.dist(home_pos) < 10.0
     && opp_min < mate_min && my_unum < 9) {
     std::vector<int> tmwork(12, 0);
     if (bhv_cyrus_block().decision(agent, tmwork) == wm.self().unum())
     return true;

     }*/

    // offside trap 2012
    {
        int num = wm.self().unum();
        double x_thr = 2.5;

        if (num > 6)
            x_thr = 4.0;
        if (num > 8)
            x_thr = 8.0;

        if (me.x < -37.0 && opp_min < mate_min
                && (home_pos.x > -36.3
                    || wm.ball().inertiaPoint(opp_min).x > -36.3
                    || (wm.ball().vel().x > 1
                        && wm.ball().inertiaPoint(opp_min).x > -36.0))
                && wm.ourDefenseLineX() > me.x - x_thr) {
            Vector2D move = Vector2D(me.x + 15, me.y);

            if ((num == 2 || num == 3) /*&& me.absY() > 10.0*/) // new B4 IO2013
                move = home_pos;
            //        move_y = rcsc::sign(home_pos.y)*10.0;

            if ((num == 4 || num == 5) && me.absY() < 7.5)
                move.y = rcsc::sign(home_pos.y) * 7.5;

            Body_GoToPoint(move, 0.5, ServerParam::i().maxDashPower()).execute(
                        agent);

            if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 12.0)
                agent->setNeckAction(new Neck_TurnToBall());
            else
                agent->setNeckAction(new Neck_TurnToBallOrScan());
            return true;
        }

    }

    if (ball.x > -36.0 && me.x < -40.0 && home_pos.x > -40.0) {
        Body_GoToPoint(Vector2D(me.x + 10.0, me.y), 0.3,
                       wm.self().getSafetyDashPower(ServerParam::i().maxDashPower()),
                       100).execute(agent);

        if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0)
            agent->setNeckAction(new Neck_TurnToBall());
        else
            agent->setNeckAction(new Neck_TurnToBallOrScan());
        return true;
    }

    if (my_unum < 6 && opp_min < mate_min && opp_min < 8
            && wm.ourDefenseLineX() < home_pos.x - 1.0 && ball.x < 0.0
            && ball.x > -35.0) {
        home_pos.x = std::max(-35.0, wm.ourDefenseLineX());

    }

    if (my_unum < 6 && opp_min < mate_min && opp_min < 15
            && me.x > home_pos.x + 2.5 && wm.ourDefenseLineX() < me.x - 2.0
            && // 2013 -2.0 bud AmirZ!
            ball.x < 10.0 && home_pos.x > -33.0
            && fabs(me.absY() - home_pos.absY()) < 4.0) {
        home_pos.x = me.x - 15;
        home_pos.y = me.y;

    } else if (wm.self().pos().x < wm.ourDefenseLineX()
               && wm.self().unum() > 5) {
        Body_GoToPoint(Vector2D(wm.ourDefenseLineX() + 2, home_pos.y), 0.3,
                       wm.self().getSafetyDashPower(ServerParam::i().maxDashPower()),
                       100).execute(agent);
        return true;
    }
    if (my_unum == 6 && opp_min < mate_min && opp_min < 15
            && me.x > home_pos.x + 3 && wm.ourDefenseLineX() < me.x - 2.0
            && ball.x < 20.0 && home_pos.x > -33.0
            && fabs(me.absY() - home_pos.absY()) < 3.0) {
        home_pos.x = me.x - 15;
        home_pos.y = me.y;

    }

    if (my_unum < 9 && my_unum > 6 && opp_min < mate_min && opp_min < 15
            && me.x > home_pos.x + 4 && wm.ourDefenseLineX() < me.x - 4.0
            && ball.x < 25.0 && home_pos.x > -33.0
            && fabs(me.absY() - home_pos.absY()) < 10.0) {
        home_pos.x = me.x - 15;
        home_pos.y = me.y;

    }

    if (ball.x < -36.0 && me.dist(home_pos) > 4.0 && opp_min < mate_min
            && my_unum < 9) {
        Body_GoToPoint(home_pos, 0.4, ServerParam::i().maxDashPower()).execute(
                    agent);
        if (wm.existKickableOpponent() && wm.ball().distFromSelf() < 18.0)
            agent->setNeckAction(new Neck_TurnToBall());
        else
            agent->setNeckAction(new Neck_TurnToBallOrScan());
        return true;

    }
    return false;
}


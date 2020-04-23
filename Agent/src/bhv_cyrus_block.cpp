#include "bhv_cyrus_block.h"
#include "bhv_cyrus_mark.h"
#include "bhv_basic_tackle.h"
#include "strategy.h"
#include "field_analyzer.h"
#include "cyrus.h"

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_and_player.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_clear_ball.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/common/audio_memory.h>

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
// set values for fields
bhv_block::bhv_block(int blocker_unum, int opponent_unum, rcsc::AngleDeg dir,
        double speed, rcsc::Vector2D start_drible_position, int block_time,
        rcsc::Vector2D target_position) {
    this->blocker_unum = blocker_unum;
    this->opponent_unum = opponent_unum;
    this->dir = dir;
    this->speed = speed;
    this->start_drible_position = start_drible_position;
    this->block_time = block_time;
    this->target_position = target_position;

}
bhv_block::bhv_block() {
    this->blocker_unum = 0;
    this->opponent_unum = 0;
    this->dir = 0;
    this->speed = 0;
    this->start_drible_position = Vector2D(0, 0);
    this->block_time = 1000;
    this->target_position = Vector2D(-52, 0);

}
rcsc::AngleDeg bhv_cyrus_block::get_dir(rcsc::PlayerAgent * agent,
        rcsc::Vector2D start_pos) {
    /*bool log = false;

     rcsc::AngleDeg dir = 0;
     rcsc::Vector2D self_pos = agent->world().self().pos();
     rcsc::Vector2D goal_pos = rcsc::Vector2D(-50, 0);

     if (start_pos.x > -25.0)
     dir = 180.0;

     else if (start_pos.absY() < 10.0)
     dir = (goal_pos - start_pos).dir().degree();

     else if (start_pos.x > -37.0 && start_pos.absY() > 20.0) {
     if (start_pos.y > 0)
     dir = 190.0;
     else
     dir = 170.0;
     } else
     dir = (goal_pos - start_pos).dir().degree();

     //oneone
     if (start_pos.x > -35 && start_pos.absY() < 25) {
     rcsc::Sector2D oneone = rcsc::Sector2D(start_pos, 1.0,
     goal_pos.dist(start_pos), (goal_pos - start_pos).th() - 30,
     (goal_pos - start_pos).th() + 30);
     if (!agent->world().existTeammateIn(oneone, 1, false)
     && !oneone.contains(self_pos)) {
     //			//cout << "      is one one" << endl;
     if (start_pos.y > 0)
     dir = (Vector2D(-45, 5) - start_pos).dir().degree();
     else
     dir = (Vector2D(-45, -5) - start_pos).dir().degree();
     }
     }
     return dir;*/
    const rcsc::WorldModel & wm = agent->world();
    const ServerParam & sp = ServerParam::i();

    static const int VALID_PLAYER_THRESHOLD = 8;
    double maxEval = 0;

    /*std::ofstream debug("777cyrus block prints.txt",
            std::ios_base::app | std::ios_base::out);*/

    AngleDeg finaldirection = AngleDeg(180);
    Vector2D maxTarget = Vector2D(0, 0);
    Rect2D fieldRect = Rect2D(
            Vector2D(-sp.pitchHalfLength(), -sp.pitchHalfWidth()),
            Vector2D(sp.pitchHalfLength(), sp.pitchHalfWidth()));

    if (wm.self().unum() == 5) {
    } //debug << "*************************************\n";

    if (wm.self().unum() == 5) {
    } //debug << tdir << "\n";
    if (wm.self().unum() == 5) {
    } //debug << wm.time().cycle() << "\n";
    for (double dir = -170; dir <= 180; dir += 10) {
        int dist = 5;

        Vector2D target = start_pos + Vector2D::polar2vector(5, dir);

        if (!fieldRect.contains(target))
            continue;

        double point = -(target.x);

        point += std::max(0.0, 40.0 - sp.ourTeamGoalPos().dist(target));

        // add bonus for goal, free situation near offside line
        if (opp_can_shoot_from(false, target, wm.ourPlayers(),
                VALID_PLAYER_THRESHOLD)) { // chon baraye teame harif dar nazar migirim opp ha ma hastim
            point += 1.0e+6;
        }

        if (wm.self().unum() == 5) {
        } //debug << " point " << point;
        if (point >= maxEval) {
            if (wm.self().unum() == 5) {
            } //debug << " m ";
            maxEval = point;
            finaldirection = AngleDeg(dir);
            maxTarget = Vector2D(target.x, target.y);
        }
    }

    for (double dir = finaldirection.degree() - 10;
            dir <= finaldirection.degree() + 10; dir += 2) {
        int dist = 5;

        Vector2D target = start_pos + Vector2D::polar2vector(5, dir);

        if (!fieldRect.contains(target))
            continue;

        double point = -(target.x);

        point += std::max(0.0, 40.0 - sp.ourTeamGoalPos().dist(target));

        // add bonus for goal, free situation near offside line
        if (opp_can_shoot_from(false, target, wm.ourPlayers(),
                VALID_PLAYER_THRESHOLD)) { // chon baraye teame harif dar nazar migirim opp ha ma hastim
            point += 1.0e+6;
        }

        if (wm.self().unum() == 5) {
        } //debug << " point " << point;
        if (point > maxEval
                || (point == maxEval && fabs(dir) > finaldirection.degree())) {

            maxEval = point;
            finaldirection = AngleDeg(dir);
            maxTarget = Vector2D(target.x, target.y);
        }
    }

    /*debug.close();*/
    //	Arm_PointToPoint(maxTarget).execute(agent);
    return finaldirection.degree();
}
double bhv_cyrus_block::get_speed(const rcsc::WorldModel & wm, AngleDeg dir,
        Vector2D ball) {
    rcsc::Sector2D drible_sector = Sector2D(ball, 0, 20, dir - 50, dir + 50);
    Vector2D self_pos = wm.self().pos();
    if (!wm.existTeammateIn(drible_sector, 0.5, false)
            && !drible_sector.contains(self_pos)) {
        return 0.95;
    }
    drible_sector = Sector2D(ball, 0, 20, dir - 30, dir + 30);
    if (!wm.existTeammateIn(drible_sector, 0.5, false)
            && !drible_sector.contains(self_pos)) {
        return 0.8;
    }
    drible_sector = Sector2D(ball, 0, 20, dir - 20, dir + 20);
    if (!wm.existTeammateIn(drible_sector, 0.5, false)
            && !drible_sector.contains(self_pos)) {
        return 0.7;
    }
    drible_sector = Sector2D(ball, 0, 10, dir - 10, dir + 10);
    if (!wm.existTeammateIn(drible_sector, 0.5, false)
            && !drible_sector.contains(self_pos)) {
        return 0.6;
    }
    return 0.8;
}
vector<bhv_block> bhv_cyrus_block::get_block_vector(rcsc::PlayerAgent * agent) {
    bool log = false;
    vector<bhv_block> block_vec(11, bhv_block());
    const rcsc::WorldModel & wm = agent->world();

    const PlayerObject * nearest_opp = wm.interceptTable()->fastestOpponent();
    int oppCycles = 0;
    Vector2D start_pos = wm.ball().pos();

//	if (!((wm.self().pos().x - opp_pos.x) < 5
//			&& (wm.self().pos().x - opp_pos.x) > 0
//			&& abs(wm.ball().pos().y - wm.self().pos().y) < 8
//			&& (ball_pos.x - wm.self().pos().x) > 0
//			&& opp_with_ball->pos().dist(ball_pos) > 3)
//			|| (!((wm.self().pos().x - opp_pos.x) < 1
//					&& (wm.self().pos().x - opp_pos.x) > -5
//					&& abs(wm.ball().pos().y - wm.self().pos().y) < 8
//					&& abs(wm.ball().pos().y - wm.self().pos().y) > 3
//					&& (ball_pos.x - wm.self().pos().x) > 0)
//					&& opp_with_ball->pos().dist(ball_pos) > 3)) {
//		while (true) {
//			double opp2ball_dist = opp_pos.dist(ball_pos);
//			opp2ball_dist -= opp_with_ball->playerTypePtr()->kickableArea();
//			opp2ball_dist -= opp_with_ball->playerTypePtr()->playerSize();
//			int cycle_opp2ball =
//					opp_with_ball->playerTypePtr()->cyclesToReachDistance(
//							opp2ball_dist);
//			if (cycle_opp2ball <= oppCycles) {
//				break;
//			}
//			start_pos += ball_vel;
//			ball_vel *= ServerParam::i().ballDecay();
//			oppCycles++;
//		}
//	} else {
    oppCycles = wm.interceptTable()->opponentReachCycle();
    start_pos = wm.ball().inertiaPoint(oppCycles);
//	}
    rcsc::AngleDeg dir = bhv_cyrus_block::get_dir(agent, start_pos);
    double drible_speed = get_speed(wm, dir, start_pos);
    drible_speed =
            wm.interceptTable()->fastestOpponent()->vel().r()
                    * wm.interceptTable()->fastestOpponent()->playerTypePtr()->playerDecay();

    bool stamina_req[12] = { false };
    if (wm.audioMemory().staminaTime().cycle() > wm.time().cycle() - 10) // Magic Number
            {
        for (std::vector<AudioMemory::Stamina>::const_iterator it =
                wm.audioMemory().stamina().begin();
                it != wm.audioMemory().stamina().end(); ++it) {
            if (it->rate_ * 8000 < 3500 && it->sender_ > 0) {
                stamina_req[it->sender_] = true;
            }
        }
    }

    for (int i = 1; i < 40; i++) {
        /*if (log)
            cout << "   " << i << " " << start_pos << " " << drible_speed << " "
                    << dir << endl;*/
        if (start_pos.x < -54 || start_pos.absY() > 36 || start_pos.x < -54)
            break;
        const PlayerPtrCont & tmms = wm.teammatesFromBall();
        const PlayerPtrCont::const_iterator tmms_end = tmms.end();
        for (PlayerPtrCont::const_iterator it = tmms.begin(); it != tmms_end;
                ++it) {
            if ((*it)->unum() < 1)
                continue;
            double deflinX = wm.ourDefensePlayerLineX();
            if ((*it)->goalie() || (*it)->unum() == 1 || (*it)->unum() == -1)
                continue;
            if ((*it)->distFromBall() > 30)
                continue;
            if ((*it)->unum() < 6 && start_pos.x > wm.ourDefenseLineX() + 15
                    && start_pos.x > -35)
                continue;
            if ((*it)->unum() < 6 && wm.ball().inertiaPoint(oppCycles).x < 15
                    && wm.ball().inertiaPoint(oppCycles).x > -25 && deflinX < 10
                    && deflinX > -25 && start_pos.x > (*it)->pos().x + 10)
                continue;
            if (wm.ball().inertiaPoint(oppCycles).x > 10 && (*it)->unum() > 6
                    && start_pos.dist((*it)->pos()) > 20)
                continue;
            if (stamina_req[(*it)->unum()] && start_pos.dist((*it)->pos()) > 4)
                continue;
            if ((*it)->isTackling())
                continue;
            bhv_block tmp = block_vec[(*it)->unum() - 1];

            if ((*it)->playerTypePtr()->cyclesToReachDistance(
                    (*it)->pos().dist(start_pos)) < i + oppCycles
                    && tmp.block_time == 1000) {
                block_vec[(*it)->unum() - 1] = bhv_block((*it)->unum(),
                        nearest_opp->unum(), dir, drible_speed,
                        wm.ball().inertiaPoint(oppCycles), i + oppCycles,
                        start_pos);
                /*if (log)
                    //cout << "      " << (*it)->unum() << " block " << endl;
        */	}
        }
        if (i % 5 == 0)
            continue;
        start_pos += Vector2D::polar2vector(drible_speed, dir);
        dir = bhv_cyrus_block::get_dir(agent, start_pos);
        drible_speed = bhv_cyrus_block::get_speed(wm, dir,
                wm.ball().inertiaPoint(oppCycles + i));
    }

    if (wm.self().distFromBall() > 30)
        return block_vec;
    start_pos = wm.ball().inertiaPoint(oppCycles);
    dir = bhv_cyrus_block::get_dir(agent, start_pos);
    drible_speed = bhv_cyrus_block::get_speed(wm, dir,
            wm.ball().inertiaPoint(oppCycles));
    drible_speed =
            wm.interceptTable()->fastestOpponent()->vel().r()
                    * wm.interceptTable()->fastestOpponent()->playerTypePtr()->playerDecay();
    for (int i = 1; i < 40; i++) {
        /*if (log)
            //cout << "   " << i << " " << start_pos << " " << drible_speed << " "
                    << dir << endl;*/
        double deflinX = wm.ourDefensePlayerLineX();

        if (start_pos.x < -54 || start_pos.absY() > 36 || start_pos.x < -54)
            break;
        const SelfObject & se = wm.self();
        if (se.unum() < 6 && start_pos.x > wm.ourDefenseLineX() + 15
                && start_pos.x > -35)
            continue;
        if (se.unum() < 6 && wm.ball().inertiaPoint(oppCycles).x < 15
                && wm.ball().inertiaPoint(oppCycles).x > -25 && deflinX < 10
                && deflinX > -25 && start_pos.x > se.pos().x + 10)
            continue;
        if (wm.ball().inertiaPoint(oppCycles).x > 10 && se.unum() > 6
                && start_pos.dist(se.pos()) > 20)
            continue;
        if (se.stamina() < 2500 && start_pos.dist(se.pos()) > 4)
            break;

        bhv_block tmp = block_vec[se.unum() - 1];

        if (se.playerTypePtr()->cyclesToReachDistance(se.pos().dist(start_pos))
                <= i + oppCycles && tmp.block_time == 1000) {
            block_vec[se.unum() - 1] = bhv_block(se.unum(), nearest_opp->unum(),
                    dir, drible_speed, wm.ball().inertiaPoint(oppCycles + i),
                    i + oppCycles, start_pos);
            if (log)
                //cout << "      " << se.unum() << " block " << endl;
            break;

        }
        if (i % 5 == 0)
            continue;
        start_pos += Vector2D::polar2vector(
                i == 1 ? drible_speed / 2 : drible_speed, dir);
        dir = bhv_cyrus_block::get_dir(agent, start_pos);
        drible_speed = bhv_cyrus_block::get_speed(wm, dir,
                wm.ball().inertiaPoint(oppCycles + i));
    }

    return block_vec;
}
/*
 vector<bhv_block> bhv_cyrus_block::get_block_vector(rcsc::PlayerAgent * agent) {
 bool log = false;
 vector<bhv_block> block_vec(11, bhv_block());
 const rcsc::WorldModel & wm = agent->world();

 int opp_number;
 const PlayerPtrCont & opps = wm.opponentsFromBall();
 const PlayerObject * nearest_opp = wm.interceptTable()->fastestOpponent();
 //(opps.empty() ? static_cast<PlayerObject *>(0) : opps.front());
 int oppCycles = wm.interceptTable()->opponentReachCycle();
 Vector2D start_pos = wm.ball().inertiaPoint(oppCycles);
 rcsc::AngleDeg dir = bhv_cyrus_block::get_dir(agent, start_pos);
 double drible_speed = get_speed(wm, dir, start_pos);
 drible_speed =
 wm.interceptTable()->fastestOpponent()->vel().r()
 * wm.interceptTable()->fastestOpponent()->playerTypePtr()->playerDecay();
 const bhv_block tmp = bhv_block();
 for (int i = 0; i < 40; i++) {
 if (log)
 //cout << "   " << i << " " << start_pos << " " << drible_speed << " "
 << dir << endl;
 if (start_pos.x < -54 || start_pos.absY() > 36)
 break;
 const PlayerPtrCont & tmms = wm.teammatesFromBall();
 const PlayerPtrCont::const_iterator tmms_end = tmms.end();
 for (PlayerPtrCont::const_iterator it = tmms.begin(); it != tmms_end;
 ++it) {

 double deflinX = wm.ourDefensePlayerLineX();
 if ((*it)->goalie() || (*it)->unum() == 1 || (*it)->unum() == -1)
 continue;
 if ((*it)->distFromBall() > 30)
 continue;
 if ((*it)->unum() < 6 && start_pos.x > wm.ourDefenseLineX() + 15
 && start_pos.x > -35)
 continue;
 if ((*it)->unum() < 6 && wm.ball().inertiaPoint(oppCycles).x < 15
 && wm.ball().inertiaPoint(oppCycles).x > -25 && deflinX < 10
 && deflinX > -25 && start_pos.x > (*it)->pos().x + 10)
 continue;

 bhv_block tmp = block_vec[(*it)->unum() - 1];

 if ((*it)->playerTypePtr()->cyclesToReachDistance(
 (*it)->pos().dist(start_pos)) < (i + oppCycles)
 && tmp.block_time == 1000) {
 block_vec[(*it)->unum() - 1] = bhv_block((*it)->unum(),
 nearest_opp->unum(), dir, drible_speed,
 wm.ball().inertiaPoint(oppCycles), (i + oppCycles),
 start_pos);
 if (log)
 //cout << "      " << (*it)->unum() << " block " << endl;
 }
 }

 start_pos += Vector2D::polar2vector(drible_speed, dir);
 dir = bhv_cyrus_block::get_dir(agent, start_pos);
 drible_speed = bhv_cyrus_block::get_speed(wm, dir,
 wm.ball().inertiaPoint(oppCycles + i));
 }

 if (wm.self().distFromBall() > 30)
 return block_vec;
 start_pos = wm.ball().inertiaPoint(oppCycles);
 dir = bhv_cyrus_block::get_dir(agent, start_pos);
 drible_speed = bhv_cyrus_block::get_speed(wm, dir,
 wm.ball().inertiaPoint(oppCycles));
 drible_speed =
 wm.interceptTable()->fastestOpponent()->vel().r()
 * wm.interceptTable()->fastestOpponent()->playerTypePtr()->playerDecay();
 for (int i = 0; i < 40; i++) {
 if (log)
 //cout << "   " << i << " " << start_pos << " " << drible_speed << " "
 << dir << endl;

 if (start_pos.x < -54 || start_pos.absY() > 36)
 break;
 const SelfObject & se = wm.self();
 if (se.unum() < 6 && start_pos.x > wm.ourDefenseLineX() + 15
 && start_pos.x > -35)
 continue;

 bhv_block tmp = block_vec[se.unum() - 1];

 if (se.playerTypePtr()->cyclesToReachDistance(se.pos().dist(start_pos))
 <= i + oppCycles && tmp.block_time == 1000) {
 block_vec[se.unum() - 1] = bhv_block(se.unum(), nearest_opp->unum(),
 dir, drible_speed, wm.ball().inertiaPoint(oppCycles),
 (i + oppCycles), start_pos);
 if (log)
 //cout << "      " << se.unum() << " block " << endl;
 break;

 }

 start_pos += Vector2D::polar2vector(drible_speed, dir);
 dir = bhv_cyrus_block::get_dir(agent, start_pos);
 drible_speed = bhv_cyrus_block::get_speed(wm, dir,
 wm.ball().inertiaPoint(oppCycles + i));
 }

 return block_vec;
 }
 */

int bhv_cyrus_block::decision(rcsc::PlayerAgent * agent, vector<int> tmwork) {
    bool log = false;
    const rcsc::WorldModel & wm = agent->world();
    /*if (log)
        //cout << "**" << wm.time().cycle() << " block start for unum "
                << wm.self().unum() << endl;*/
    vector<bhv_block> block_vector = get_block_vector(agent);

    int min_reach_time = 10000;
    int best_bolcker_unum = 0;
    int index_of_best = 0;
    for (int j = 10; j > 0; j--) {
        bhv_block tmp = block_vector[j];
        /*if (log)
            //cout << wm.time().cycle() << " " << j + 1 << "block in"
                    << tmp.target_position << " time " << tmp.block_time
                    << endl;*/

        double tm_dist =
                wm.ourPlayer(tmp.blocker_unum) != NULL
                /*&& (!(wm.ball().pos().x < -32
                 && wm.ball().pos().absY() < 22))*/?
                        (tmp.target_position.dist(
                                Strategy::i().getPosition(tmp.blocker_unum))
                                + (tmp.target_position.dist(
                                        wm.ourPlayer(tmp.blocker_unum)->pos())
                                        / (!(wm.ball().inertiaPoint(
                                                wm.interceptTable()->opponentReachCycle()).x
                                                < -32
                                                && wm.ball().inertiaPoint(
                                                        wm.interceptTable()->opponentReachCycle()).absY()
                                                        < 22)) ? 2 : (1))) / 2 :
                        0;
        if ((tmp.block_time + tm_dist) < min_reach_time
                && tmwork[tmp.blocker_unum] != 1) {
            min_reach_time = tmp.block_time + tm_dist;
            best_bolcker_unum = tmp.blocker_unum;
            index_of_best = j;

        }

    }

    if (best_bolcker_unum != wm.self().unum())
        return best_bolcker_unum;
    bhv_block tmp = block_vector[index_of_best];
    execute(agent, tmp);
    return wm.self().unum();
    /*double time_intercept = 3.5;
     if (wm.self().unum() <= 6
     && (tmp.target_position.dist(Vector2D(-52, 0)) < 25
     || tmp.target_position.x - wm.ourDefenseLineX() < 15))
     time_intercept = 1.5;
     if (tmp.block_time > time_intercept) {
     //Arm_PointToPoint(tmp.target_position).execute(agent);
     agent->setNeckAction(
     new Neck_TurnToBallAndPlayer(wm.theirPlayer(tmp.opponent_unum),
     2));

     Body_GoToPoint2010(tmp.target_position, 0.5, 100, 1.1, tmp.block_time,
     false, 25).execute(agent);
     if (log)
     //cout << wm.time().cycle() << "go to point" << tmp.blocker_unum
     << "opp" << tmp.opponent_unum << " in "
     << tmp.target_position.x << "," << tmp.target_position.y
     << endl;

     } else {
     //Arm_PointToPoint(Vector2D(0, 0)).execute(agent);
     agent->setNeckAction(new Neck_TurnToBall());
     Body_Intercept2009(false).execute(agent);
     if (log)
     //cout << wm.time().cycle() << "intercept" << endl;
     }

     return tmp.blocker_unum;*/
}
bool bhv_cyrus_block::execute(rcsc::PlayerAgent * agent, bhv_block tmp) {

    const WorldModel & wm = agent->world();
    /*//cout << wm.time().cycle() << "*********************************for"
            << wm.self().unum() << endl;*/
    const PlayerObject * opp_with_ball = wm.interceptTable()->fastestOpponent();
    const BallObject & ball = wm.ball();
    Vector2D ball_pos = ball.pos();
    Vector2D ball_vel = ball.vel();
    Vector2D opp_pos = opp_with_ball->pos();
    int cycle_intercept = 0;
    int cycle_first_intercept = 0;
    if (!((wm.self().pos().x - opp_pos.x) < 5
            && (wm.self().pos().x - opp_pos.x) > 0
            && abs(wm.ball().pos().y - wm.self().pos().y) < 8
            && (ball_pos.x - wm.self().pos().x) > 0
            && opp_with_ball->pos().dist(ball_pos) > 3)
            || (!((wm.self().pos().x - opp_pos.x) < 1
                    && (wm.self().pos().x - opp_pos.x) > -5
                    && abs(wm.ball().pos().y - wm.self().pos().y) < 8
                    && abs(wm.ball().pos().y - wm.self().pos().y) > 3
                    && (ball_pos.x - wm.self().pos().x) > 0)
                    && opp_with_ball->pos().dist(ball_pos) > 3)) {
        while (true) {
            double opp2ball_dist = opp_pos.dist(ball_pos);
            opp2ball_dist -= opp_with_ball->playerTypePtr()->kickableArea();
            opp2ball_dist -= opp_with_ball->playerTypePtr()->playerSize();
            int cycle_opp2ball =
                    opp_with_ball->playerTypePtr()->cyclesToReachDistance(
                            opp2ball_dist);
            if (cycle_opp2ball <= cycle_intercept) {
                break;
            }
            ball_pos += ball_vel;
            ball_vel *= ServerParam::i().ballDecay();
            cycle_intercept++;
        }
    } else {
        cycle_intercept = wm.interceptTable()->opponentReachCycle();
        ball_pos = wm.ball().inertiaPoint(cycle_intercept);
    }
    cycle_first_intercept = cycle_intercept;
    /*//cout << "   opp" << opp_with_ball->unum() << " " << "intercept in"
            << cycle_intercept << "cycle in" << ball_pos << endl;*/
    AngleDeg opp_body = opp_with_ball->body();
    Vector2D opp_vel = opp_with_ball->vel();
    Vector2D self_pos = wm.self().pos();

    if (ball_pos.dist(opp_pos) > 2) {
        opp_body = (ball_pos - opp_pos).th();
        opp_vel = Vector2D::polar2vector(
                opp_with_ball->playerTypePtr()->playerSpeedMax(),
                (ball_pos - opp_pos).th());
    }
    opp_pos = ball_pos;
    //cout << "   opp vel after intercept" << opp_vel.r() << endl;
    //cout << "   after intercept " << "opppos " << opp_pos << " oppbody"
            /*<< opp_body << " oppvel" << opp_vel.r() << " oppvelr"
            << opp_vel.th() << endl;*/

    { //kick
        opp_pos += opp_vel;
        opp_vel *= opp_with_ball->playerTypePtr()->playerDecay();
        cycle_intercept++; //for kick
    }

    //cout << "   after     kick " << "opppos " << opp_pos << " oppbody"
            /*<< opp_body << " oppvel" << opp_vel.r() << " oppvelr"
            << opp_vel.th() << endl;*/
    AngleDeg drible_dir = 180;
    //cout << " start drible simulate-----------------" << endl;
    while (true) {
        double self2opp_dist = self_pos.dist(opp_pos) - 0.5;
        int self2opp_cycle = wm.self().playerTypePtr()->cyclesToReachDistance(
                self2opp_dist);

        //cout << "   cycle :" << cycle_intercept << "   cycle too opp :"
                /*<< self2opp_cycle << endl;*/
        if (self2opp_cycle <= cycle_intercept) {
            break;
        }
        if (cycle_intercept % 5 != 0 || cycle_intercept == 0)
            drible_dir = get_dir(agent, opp_pos);

        AngleDeg angle_dif =
                (drible_dir - opp_body).abs() > 180 ?
                        360 - (drible_dir - opp_body).abs() :
                        (drible_dir - opp_body).abs();
        //cout << "      drible dir: " << drible_dir << "oppbody" << opp_body
                /*<< "angledif" << angle_dif << endl;*/

        if (angle_dif.degree() > 20) {
            opp_pos += opp_vel;
            opp_vel *= opp_with_ball->playerTypePtr()->playerDecay();
            opp_body = drible_dir;
            cycle_intercept++;
            /*//cout << "   after     trun " << "opppos " << opp_pos << " oppbody"
                    << opp_body << " oppvel" << opp_vel.r() << " oppvelr"
                    << opp_vel.th() << endl;*/
        } else if (cycle_intercept % 5 == 0) {
            { //kick
                opp_pos += opp_vel;
                opp_vel *= opp_with_ball->playerTypePtr()->playerDecay();
                cycle_intercept++; //for kick
            }

        } else {
            double max_dashpower = ServerParam::i().maxDashPower();
            double dashpower_rate =
                    opp_with_ball->playerTypePtr()->dashPowerRate();
            double effort = opp_with_ball->playerTypePtr()->effortMax();
            double opp_decay = opp_with_ball->playerTypePtr()->playerDecay();
            Vector2D dash_accel = Vector2D::polar2vector(
                    max_dashpower * dashpower_rate * effort, drible_dir);
            opp_vel += dash_accel;
            if (opp_vel.r() > opp_with_ball->playerTypePtr()->realSpeedMax())
                opp_vel = Vector2D::polar2vector(
                        opp_with_ball->playerTypePtr()->realSpeedMax(),
                        opp_vel.th());
            opp_pos += opp_vel;
            double movedist = opp_vel.r();
            opp_vel *= opp_decay;

            cycle_intercept++;
    /*		//cout << "   after     move " << movedist << " opppos " << opp_pos
                    << " oppbody" << opp_body << " oppvel" << opp_vel.r()
                    << " oppvelr" << opp_vel.th() << endl;*/
        }
    }
    double time_intercept = 2.5;
    if (wm.self().unum() <= 6
            && (opp_pos.dist(Vector2D(-52, 0)) < 25
                    || opp_pos.x - wm.ourDefenseLineX() < 15))
        time_intercept = 1.5;
    if (cycle_intercept < 2) {
        agent->setNeckAction(new Neck_TurnToBall());
        //cout << "   INTERCEPT" << endl;
        Body_Intercept2009(false).execute(agent);
//		//Arm_PointToPoint(ball.pos()).execute(agent);
        return true;
    }
    if (blockMove(agent, opp_pos)) {
        Arm_PointToPoint(opp_pos).execute(agent);
        agent->setNeckAction(new Neck_TurnToBallAndPlayer(opp_with_ball, 1));
        return true;
    }
    /*double angle_thr = 20;
     if (angle_thr > 20)
     angle_thr = 20;
     if(opp_pos.dist(self_pos)<4){
     AngleDeg dif = (wm.self().body()-(opp_pos-self_pos).th());
     if(dif.degree()>180)dif=360-dif;
     if(dif.degree()<60){
     //Arm_PointToPoint(opp_pos).execute(agent);
     agent->doDash(100,(opp_pos-self_pos).th()-wm.self().body());
     agent->setNeckAction(new Neck_TurnToBall());
     return true;
     }
     else if (dif.degree()>120){
     //Arm_PointToPoint(opp_pos).execute(agent);
     agent->doDash(-200,-((opp_pos-self_pos).th()-wm.self().body()));
     agent->setNeckAction(new Neck_TurnToBall());
     return true;
     }
     }
     AngleDeg dif = ((opp_pos - self_pos).th() - wm.self().body()).abs();
     if (dif.degree() > 180)
     dif = 360 - dif;
     if (dif.degree() < 15) {
     //cout << "dif" << dif.degree() << "body" << wm.self().body() << "target"
     << (opp_pos - self_pos).th() << endl;
     //cout << "   doodash" << opp_pos << endl;
     agent->doDash(200, (opp_pos - self_pos).th() - wm.self().body());
     //Arm_PointToPoint(opp_pos).execute(agent);
     agent->setNeckAction(new Neck_TurnToBall());
     return true;
     }
     if (Body_GoToPoint2010(opp_pos, 0.5, 100, 1.5, 5, false, angle_thr).execute(
     agent)) {
     //cout << "   gotopoint" << opp_pos << endl;
     //Arm_PointToPoint(opp_pos).execute(agent);
     agent->setNeckAction(new Neck_TurnToBall());
     return true;
     }*/
    return false;
}
bool bhv_cyrus_block::blockMove(rcsc::PlayerAgent * agent, Vector2D target) {

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
        //cout << "A" << endl;
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
            //cout << "B" << endl;
            return agent->doDash(-200, move_body_dir);

        } else {
            double move_body_dir = move_dir - body_dir;
            if (move_body_dir > 180)
                move_body_dir = -(360 - move_body_dir);
            if (move_body_dir < -180)
                move_body_dir = (360 + move_body_dir);
            //cout << "C" << endl;
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
                //cout << "D" << endl;
            }
        } else {
            if (self_vel_r > 0.3 && abs(self_vel_dir - target_dir) < 10
                    && abs(body_dir - target_dir) < 15) {
                double move_body_dir = move_dir - body_dir;
                if (move_body_dir > 180)
                    move_body_dir = -(360 - move_body_dir);
                if (move_body_dir < -180)
                    move_body_dir = (360 + move_body_dir);
                //cout << "E" << endl;
                return agent->doDash(100, move_body_dir);
            } else if (abs(self_vel_dir - target_dir) < 10
                    && abs(body_dir - target_dir) < 10) {
                double move_body_dir = move_dir - body_dir;
                if (move_body_dir > 180)
                    move_body_dir = -(360 - move_body_dir);
                if (move_body_dir < -180)
                    move_body_dir = (360 + move_body_dir);
                //cout << "F" << endl;
                return agent->doDash(100, move_body_dir);
            }
        }
    }

    if (target_dist < 10) {
        //cout << "G" << endl;
        return Body_GoToPoint2010(target, 1, 100, 1, 1, false, 20).execute(
                agent);
    } else {
        //cout << "H" << endl;
        return Body_GoToPoint2010(target, 1, 100, 1, 1, false, 15).execute(
                agent);
    }
}
namespace {

struct Player {
    const AbstractPlayerObject * player_;
    AngleDeg angle_from_pos_;
    double hide_angle_;

    Player(const AbstractPlayerObject * player, const Vector2D & pos) :
            player_(player), angle_from_pos_(), hide_angle_(0.0) {
        Vector2D inertia_pos = player->inertiaFinalPoint();
        double control_dist = (
                player->goalie() ?
                        ServerParam::i().catchAreaLength() :
                        player->playerTypePtr()->kickableArea());
        double hide_angle_radian = std::asin(
                std::min(control_dist / inertia_pos.dist(pos), 1.0));

        angle_from_pos_ = (inertia_pos - pos).th();
        hide_angle_ = hide_angle_radian * AngleDeg::RAD2DEG;
    }

    struct Compare {
        bool operator()(const Player & lhs, const Player & rhs) const {
            return lhs.angle_from_pos_.degree() < rhs.angle_from_pos_.degree();
        }
    };
};

}
bool bhv_cyrus_block::opp_can_shoot_from(const bool is_self,
        const Vector2D & pos, const AbstractPlayerCont & myteammates,
        const int valid_opponent_threshold) {
    static const double SHOOT_DIST_THR2 = std::pow(17.0, 2);
//static const double SHOOT_ANGLE_THRESHOLD = 20.0;
    static const double SHOOT_ANGLE_THRESHOLD = 15.0;

    static const double OPPONENT_DIST_THR2 = std::pow(20.0, 2);

    if (ServerParam::i().ourTeamGoalPos().dist2(pos) > SHOOT_DIST_THR2) {
        return false;
    }

#ifdef DEBUG_CAN_SHOOT_FROM
    dlog.addText( Logger::SHOOT,
            "===== "__FILE__": (can_shoot_from) pos=(%.1f %.1f) ===== ",
            pos.x, pos.y );
#endif

    const Vector2D goal_minus(-ServerParam::i().pitchHalfLength(),
            -ServerParam::i().goalHalfWidth() + 0.5);
    const Vector2D goal_plus(-ServerParam::i().pitchHalfLength(),
            +ServerParam::i().goalHalfWidth() - 0.5);

    const AngleDeg goal_minus_angle = (goal_minus - pos).th();
    const AngleDeg goal_plus_angle = (goal_plus - pos).th();

//
// create opponent list
//

    std::vector<Player> opponent_candidates;
    opponent_candidates.reserve(myteammates.size());

    const AbstractPlayerCont::const_iterator o_end = myteammates.end();
    for (AbstractPlayerCont::const_iterator o = myteammates.begin(); o != o_end;
            ++o) {
        if ((*o)->posCount() > valid_opponent_threshold) {
            continue;
        }

        if ((*o)->pos().dist2(pos) > OPPONENT_DIST_THR2) {
            continue;
        }

        opponent_candidates.push_back(Player(*o, pos));
#ifdef DEBUG_CAN_SHOOT_FROM
        dlog.addText( Logger::SHOOT,
                "(can_shoot_from) (opponent:%d) pos=(%.1f %.1f) angleFromPos=%.1f hideAngle=%.1f",
                opponent_candidates.back().player_->unum(),
                opponent_candidates.back().player_->pos().x,
                opponent_candidates.back().player_->pos().y,
                opponent_candidates.back().angle_from_pos_.degree(),
                opponent_candidates.back().hide_angle_ );
#endif
    }

//
// TODO: improve the search algorithm (e.g. consider only angle width between opponents)
//
// std::sort( opponent_candidates.begin(), opponent_candidates.end(),
//            Opponent::Compare() );

    const double angle_width = (goal_plus_angle - goal_minus_angle).abs();
    const double angle_step = std::max(2.0, angle_width / 10.0);

    const std::vector<Player>::const_iterator end = opponent_candidates.end();

    double max_angle_diff = 0.0;

    for (double a = 0.0; a < angle_width + 0.001; a += angle_step) {
        const AngleDeg shoot_angle = goal_minus_angle + a;

        double min_angle_diff = 180.0;
        for (std::vector<Player>::const_iterator o =
                opponent_candidates.begin(); o != end; ++o) {
            double angle_diff = (o->angle_from_pos_ - shoot_angle).abs();

#ifdef DEBUG_CAN_SHOOT_FROM
            dlog.addText( Logger::SHOOT,
                    "(can_shoot_from) __ opp=%d rawAngleDiff=%.1f -> %.1f",
                    o->player_->unum(),
                    angle_diff, angle_diff - o->hide_angle_*0.5 );
#endif
            if (is_self) {
                angle_diff -= o->hide_angle_;
            } else {
                angle_diff -= o->hide_angle_ * 0.5;
            }

            if (angle_diff < min_angle_diff) {
                min_angle_diff = angle_diff;

                if (min_angle_diff < SHOOT_ANGLE_THRESHOLD) {
                    break;
                }
            }
        }

        if (min_angle_diff > max_angle_diff) {
            max_angle_diff = min_angle_diff;
        }

#ifdef DEBUG_CAN_SHOOT_FROM
        dlog.addText( Logger::SHOOT,
                "(can_shoot_from) shootAngle=%.1f minAngleDiff=%.1f",
                shoot_angle.degree(),
                min_angle_diff );
#endif
    }

#ifdef DEBUG_CAN_SHOOT_FROM
    dlog.addText( Logger::SHOOT,
            "(can_shoot_from) maxAngleDiff=%.1f",
            max_angle_diff );
#endif

    return max_angle_diff >= SHOOT_ANGLE_THRESHOLD;
}


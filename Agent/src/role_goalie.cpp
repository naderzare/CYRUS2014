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

#include "role_goalie.h"

#include "bhv_goalie_basic_move.h"
#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_free_kick.h"
#include "chain_action/bhv_chain_action.h"
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/body_clear_ball.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/world_model.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>

using namespace rcsc;

const std::string RoleGoalie::NAME("Goalie");
RoleGoalie::lastKicker RoleGoalie::lastKickerteam = non;

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg(&RoleGoalie::create,
		RoleGoalie::name());
}

/*-------------------------------------------------------------------*/
/*!

 */
bool RoleGoalie::execute(PlayerAgent * agent) {

	const WorldModel & wm = agent->world();

	static const Rect2D our_penalty(
			Vector2D(-ServerParam::i().pitchHalfLength(),
					-ServerParam::i().penaltyAreaHalfWidth() + 1.0),
			Size2D(ServerParam::i().penaltyAreaLength() - 1.0,
					ServerParam::i().penaltyAreaWidth() - 2.0));


	static bool kicking = false;

	//////////////////////////////////////////////////////////////
	// play_on play

	static bool isDanger = false;
	static int dangerCycle = 0;

	rcsc::Vector2D ball = wm.ball().pos();

	if (wm.lastKickerSide() == wm.ourSide()) {
		isDanger = true;
        dangerCycle = static_cast<int>(wm.time().cycle());
	}

	if (isDanger) {
		if (wm.gameMode().type() != rcsc::GameMode::PlayOn)
			isDanger = false;

		if (wm.time().cycle() - dangerCycle > 33)
			isDanger = false;

		if (wm.existKickableOpponent() || wm.existKickableTeammate())
			isDanger = false;

	}

	if (agent->world().time().cycle()
			> agent->world().self().catchTime().cycle()
					+ ServerParam::i().catchBanCycle()
			&& agent->world().ball().distFromSelf()
					< ServerParam::i().catchableArea() - 0.05
			&& our_penalty.contains(agent->world().ball().pos())) {
		Vector2D nearestTmmPos = wm.teammatesFromSelf().front()->pos();

		if (isDanger) {
		}

		if (our_penalty.contains(agent->world().ball().pos())
				&& (nearestTmmPos.dist(wm.ball().pos()) > 2.5
						|| wm.existKickableOpponent()) && !kicking
				&& (!isDanger
						|| (wm.ball().vel().r() > 2.5
								&& ball.dist(wm.self().pos()) > 0.45))) {
			kicking = false;
			isDanger = false;
			agent->doCatch();
			agent->setNeckAction(new Neck_TurnToBall());
		} else {
			kicking = true;
			isDanger = false;
			doKick(agent);
		}
	} else if (wm.self().isKickable()) {
		kicking = true;
		isDanger = false;
		doKick(agent);
	} else {
		kicking = false;
		doMove(agent);
	}

	return true;
}
/*-------------------------------------------------------------------*/
/*!

 */
void RoleGoalie::updateLastKicker(rcsc::PlayerAgent * agent) {
	if (agent->world().opponentsFromBall().size() > 0)
		if (agent->world().opponentsFromBall().front()->kicked()) {
			lastKickerteam = opp;
		}
	if (agent->world().teammatesFromBall().size() > 0)
		if (agent->world().teammatesFromBall().front()->kicked()) {
			lastKickerteam = tmm;
		}
}
void RoleGoalie::doKick(PlayerAgent * agent) {
	if (Bhv_ChainAction().execute(agent)) {
		dlog.addText(Logger::TEAM, __FILE__": (execute) do chain action");
		agent->debugClient().addMessage("ChainAction");
		return;
	}
	Body_ClearBall().execute(agent);
	agent->setNeckAction(new Neck_ScanField());
}

/*-------------------------------------------------------------------*/
/*!

 */
void RoleGoalie::doMove(PlayerAgent * agent) {
	if (Bhv_GoalieChaseBall::is_ball_chase_situation(agent)) {
		Bhv_GoalieChaseBall().execute(agent);
	} else {
		Bhv_GoalieBasicMove().execute(agent);
	}
}

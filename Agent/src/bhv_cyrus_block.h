/*
 *Copyright:

 Copyright (C) Amir Tavafi

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/player_agent.h>
#include <iostream>
#include <vector>

namespace rcsc {
class WorldModel;
}
class bhv_block {
public:
	int blocker_unum;
	int opponent_unum;
	rcsc::AngleDeg dir;
	double speed;
	rcsc::Vector2D start_drible_position;
	int block_time;
	rcsc::Vector2D target_position;
	bhv_block(int blocker_unum, int opponent_unum, rcsc::AngleDeg dir,
			double speed, rcsc::Vector2D start_drible_position, int block_time,
			rcsc::Vector2D target_position);
	bhv_block();
};
class bhv_cyrus_block/* :public rcsc::SoccerBehavior*/{

private:

public:
	rcsc::AngleDeg get_dir(rcsc::PlayerAgent * agent, rcsc::Vector2D start_pos);
	double get_speed(const rcsc::WorldModel & wm, rcsc::AngleDeg dir,
			rcsc::Vector2D ball);
	std::vector<bhv_block> get_block_vector(rcsc::PlayerAgent * agent);

	int decision(rcsc::PlayerAgent * agent, std::vector<int> tmwork);
	bool execute(rcsc::PlayerAgent * agent, bhv_block tmp);
	rcsc::Vector2D getOppTarget(rcsc::PlayerAgent* agent, rcsc::Vector2D ball);

	rcsc::Vector2D getTarget(rcsc::PlayerAgent * agent);
	bool blockMove(rcsc::PlayerAgent * agent, rcsc::Vector2D target);
	bool opp_can_shoot_from(const bool is_self, const rcsc::Vector2D & pos,
			const rcsc::AbstractPlayerCont & myteammates,
			const int valid_opponent_threshold);

};

/*
 * bhv_cyrus_mark.h
 *
 *  Created on: Mar 17, 2014
 *      Author: nkpo
 */

/* BHV_CYRUS_MARK_H_ */


/////////////////////////////////////////////////////////////////////
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/player_agent.h>
#include <iostream>
#include <vector>

namespace rcsc {
class WorldModel;
}

class bhv_cyrus_mark/*: public rcsc::SoccerBehavior */{

private:

public:
	std::vector<double> set_opp_evaluation(rcsc::PlayerAgent * agent);
	std::vector<std::vector<double> > get_weight(rcsc::PlayerAgent * agent,
			std::vector<double> & oppev);
	std::vector<std::vector<double> > get_danger_opp(rcsc::PlayerAgent * agent,
			std::vector<std::vector<double> > & weights);
	int get_best_opp(std::vector<double> & oppev);
	int get_best_tmm(int opp_unum,
			std::vector<std::vector<double> > & danger_opp,
			std::vector<std::vector<double> > & weights, rcsc::Vector2D ball,std::vector<int>tmwork);
	rcsc::Vector2D get_pass_cut_mark_point(rcsc::PlayerAgent * agent,
			rcsc::Vector2D opp_pos);
	bool execute(rcsc::PlayerAgent * agent, std::vector<int> tmwork);
	bool mark_move(rcsc::PlayerAgent * agent, rcsc::Vector2D target);

};

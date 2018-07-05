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

#ifndef BHV_BASIC_MOVE_H
#define BHV_BASIC_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_BasicMove
    : public rcsc::SoccerBehavior {
public:
    Bhv_BasicMove()
      { }

    static int sums;
    static int sumto;
    static int numSums;
    static int numSumto;


    bool execute( rcsc::PlayerAgent * agent );
    bool danger_move(rcsc::PlayerAgent * agent) ;
private:
    static int lastCycle;
    static rcsc::Vector2D lastTarget;
    double getDashPower( const rcsc::PlayerAgent * agent );
    bool doIntercept(rcsc::PlayerAgent * agent);
    bool UltimateGap(rcsc::PlayerAgent * agent);
    bool unMark( rcsc::PlayerAgent * agent);

    int predict_player_turn_cycle( rcsc::PlayerAgent * agent,//me
    		const rcsc::AngleDeg & player_body,
    		double player_speed,
    		double target_dist,
    		rcsc::AngleDeg target_angle,
    		double dist_thr,
    		bool use_back_dash
    		/*const rcsc::Vector2D & new_opp_pos*/);
};

#endif

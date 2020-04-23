// -*-c++-*-

/*!
  \file bhv_body_neck_to_ball.h
  \brief face to the ball with turn & turn_neck header file
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

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

#ifndef RCSC_ACTION_BODY_NECK_TO_BALL_H
#define RCSC_ACTION_BODY_NECK_TO_BALL_H

#include <rcsc/player/soccer_action.h>

namespace rcsc {

/*!
  \class Bhv_BodyNeckToBall
  \brief face to ball. if possible, turn only body & align neck angle
*/
class Bhv_BodyNeckToBall
    : public SoccerBehavior {
private:

public:
    /*!
      \brief accessible from global.
     */
    Bhv_BodyNeckToBall()
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
     */
    bool execute( PlayerAgent * agent );
};

}

#endif

// -*-c++-*-

/*!
  \file bhv_deflecting_tackle.h
  \brief tackle ball to out of our goal
*/

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#ifndef BHV_DEFLECTING_TACKLE_H
#define BHV_DEFLECTING_TACKLE_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/math_util.h>

namespace rcsc {

class PlayerAgent;
class WorldModel;
class AngleDeg;

/*!
  \class Bhv_DeflectingTackle
  \brief tackle ball to out of our goal
*/
class Bhv_DeflectingTackle
    : public rcsc::SoccerBehavior {

private:
    const double M_tackle_prob_threshold;
    const bool M_force_tackle;

public:
    static
    rcsc::Vector2D getTackleResult( const rcsc::AngleDeg & absolute_dir,
                                    const rcsc::WorldModel & wm );
    static
    rcsc::Vector2D getTackleAccel( const rcsc::AngleDeg & absolute_dir,
                                   const rcsc::WorldModel & wm );

    static
    bool isShootBall( const rcsc::Vector2D & vec,
                      const rcsc::WorldModel & wm,
                      double buffer );

public:
    /*!
      \brief constructor
     */
    Bhv_DeflectingTackle( double tackle_prob_threshold = rcsc::EPS,
                          bool force_tackle = false );

    /*!
      \brief tackle ball to out of our goal
      \param agent agent pointer to the agent itself
      \return true with action, false if not performed
     */
    bool execute( rcsc::PlayerAgent * agent );
};

}
#endif

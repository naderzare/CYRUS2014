// -*-c++-*-

/*!
  \file body_go_to_point2009.h
  \brief go to point action
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

#ifndef RCSC_ACTION_BODY_GO_TO_POINT_2009_H
#define RCSC_ACTION_BODY_GO_TO_POINT_2009_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/common/player_type.h>
#include <rcsc/geom/vector_2d.h>

namespace rcsc {

/*!
  \class Body_GoToPoint2009
  \brief go to point action
 */
class Body_GoToPoint2009
    : public BodyAction {
private:
    //! target point to be reached
    const Vector2D M_target_point;
    //! distance threshold to the target point
    const double M_dist_thr;
    //! maximum power parameter (this value has to be positive)
    const double M_max_dash_power;
    //! preferred dash speed. negative value means no preferred speed
    const double M_dash_speed;
    //! recommended reach cycle
    const int M_cycle;
    //! if this is true, agent must save recover parameter.
    const bool M_save_recovery;
    //! minimal turn buffer
    const double M_dir_thr;

    //! internal variable. if this value is true, agent will dash backward.
    bool M_back_mode;
public:
    /*!
      \brief construct with all paramters
      \param point target point to be reached
      \param dist_thr distance threshold to the target point
      \param max_dash_power maximum dash power parameter (have to be a positive value)
      \param dash_speed prefered dash speed. negative value means no preferred speed
      \param cycle preferred reach cycle
      \param save_recovery if this is true, player always saves its recoverry.
      \param dir_thr turn angle threshold
    */
    Body_GoToPoint2009( const Vector2D & point,
                        const double & dist_thr,
                        const double & max_dash_power,
                        const double & dash_speed = -1.0,
                        const int cycle = 100,
                        const bool save_recovery = true,
                        const double & dir_thr = 12.0 )
        : M_target_point( point ),
          M_dist_thr( dist_thr ),
          M_max_dash_power( std::fabs( max_dash_power ) ),
          M_dash_speed( dash_speed ),
          M_cycle( cycle ),
          M_save_recovery( save_recovery ),
          M_dir_thr( dir_thr ),
          M_back_mode( false )
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool execute( PlayerAgent * agent );

private:

    /*!
      \brief if possible, try omnidash action.
      \param agent pointer to the agent
      \return true if action is performed, otherwise false
    */
    bool doOmniDash( PlayerAgent * agent );

    /*!
      \brief if necesarry, perform turn action and return true
      \param agent pointer to the agent instance
      \return true if turn is performed
    */
    bool doTurn( PlayerAgent * agent );

    /*!
      \brief if necesarry, perform dash action and return true
      \param agent pointer to the agent instance
      \return true if turn is performed
    */
    bool doDash( PlayerAgent * agent );
};

}

#endif

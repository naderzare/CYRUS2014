// -*-c++-*-

/*!
  \file body_stop_dash.h
  \brief try to change the agent's velocity to 0.
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

#ifndef RCSC_ACTION_BODY_STOP_DASH_H
#define RCSC_ACTION_BODy_STOP_DASH_H

#include <rcsc/player/soccer_action.h>

namespace rcsc {

/*!
  \class Body_StopDash
  \brief try to change the agent's velocity to 0.
 */
class Body_StopDash
    : public BodyAction {
private:
    const bool M_save_recovery;

public:
    /*!
      \brief constructor
      \param save_recovery if true, agent will keep the recovery.
     */
    explicit
    Body_StopDash( const bool save_recovery )
        : M_save_recovery( save_recovery )
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

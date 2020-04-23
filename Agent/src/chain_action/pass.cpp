// -*-c++-*-

/*!
  \file pass.cpp
  \brief pass object Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pass.h"

#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

 */
Pass::Pass( const int passer,
            const int receiver,
            const Vector2D & receive_point,
            const double & ball_speed,
            const int duration_step,
            const int kick_count,
            const bool final_action,
            const char * description,
            int pass_safe)
    : CooperativeAction( CooperativeAction::Pass,
                         passer,
                         receive_point,
                         duration_step,
                         description,
                         pass_safe)
{

    setTargetPlayerUnum( receiver );
    setFirstBallSpeed( ball_speed );
    setKickCount( kick_count );
    setFinalAction( final_action );
    setActionsafe(pass_safe);
}

}

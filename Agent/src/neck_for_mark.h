// -*-c++-*-

/*!
  \file neck_for_mark.h
  \brief check the ball or scan field or check the marked player with neck.
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

#ifndef RCSC_ACTION_NECK_FOR_MARK_H
#define RCSC_ACTION_NECK_FOR_MARK_H

#include <rcsc/player/soccer_action.h>

namespace rcsc {

/*!
  \class Neck_ForMark
  \brief check the ball or scan field or check the marked player with neck.
  if the marked player's location is not accurate, scan him
  else if next ball position is NOT over view width ,scan field
  else face to ball
*/
class Neck_ForMark
    : public NeckAction {
private:

    const int M_count_thr;

public:
    /*!
      \brief accessible from global.
      \param count_thr if ball::posCount() is less than or equal to
      this value, agent always perform field scan, otherwise try to
      look the ball. if this value is negative, agent always try to
      look the ball.
     */
    Neck_ForMark( const int count_thr = 0 )
        : M_count_thr( count_thr )
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
     */
    bool execute( PlayerAgent * agent );

    /*!
      \brief create cloned object
      \return pointer to the cloned object
     */
    NeckAction * clone() const
      {
          return new Neck_ForMark;
      }
};

}

#endif

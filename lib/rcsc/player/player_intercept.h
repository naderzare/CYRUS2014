// -*-c++-*-

/*!
  \file player_intercept.h
  \brief intercept predictor for other players Header File
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

#ifndef RCSC_PLAYER_PLAYER_INTERCEPT_H
#define RCSC_PLAYER_PLAYER_INTERCEPT_H

#include <rcsc/geom/vector_2d.h>
#include <vector>

namespace rcsc {

class PlayerType;
class BallObject;
class PlayerObject;
class WorldModel;

/*!
  \class PlayerIntercept
  \brief intercept predictor for other players
*/
class PlayerIntercept {
private:
    //! const reference to the WorldModel instance
    const WorldModel & M_world;
    //! const reference to the predicted ball position cache instance
    const std::vector< Vector2D > & M_ball_pos_cache;

    // not used
    PlayerIntercept();

public:

    /*!
      \brief construct with all variables.
      \param world const reference to the WormdModel instance
      \param ball_pos_cache const reference to the ball position container
    */
    PlayerIntercept( const WorldModel & world,
                     const std::vector< Vector2D > & ball_pos_cache )
        : M_world( world )
        , M_ball_pos_cache( ball_pos_cache )
      { }

    /*!
      \brief destructor. nothing to do
    */
    ~PlayerIntercept()
      { }

    //////////////////////////////////////////////////////////
    /*!
      \brief get predicted ball gettable cycle
      \param player const reference to the player object
      \param player_type player type parameter for the player
      \param max_cycle max predict cycle. estimation loop is limited to this value.
      \return predicted cycle value
    */
    int predict( const PlayerObject & player,
                 const PlayerType & player_type,
                 const int max_cycle ) const;

private:
    /*!
      \brief check if player can reach after turn & dash 'cycle' cycles later
      \param cycle we consder the status 'cycle' cycles later
      \param player const reference to the player object
      \param player_type player type parameter
      \param control_area player's ball controllable radius
      \param ball_pos ball position after 'cycle'
      \return true if player can get the ball
    */
    bool canReachAfterTurnDash( const int cycle,
                                const PlayerObject & player,
                                const PlayerType & player_type,
                                const double & control_area,
                                const Vector2D & ball_pos ) const;

    /*!
      \brief predict required cycle to face to the ball position
      \param cycle we consder the status 'cycle' cycles later
      \param player const reference to the player object
      \param player_type player type parameter
      \param control_area player's ball controllable radius
      \param ball_pos ball position 'cycle' cycles later
      \return predicted cycle value
    */
    int predictTurnCycle( const int cycle,
                          const PlayerObject & player,
                          const PlayerType & player_type,
                          const double & control_aera,
                          const Vector2D & ball_pos ) const;

    /*!
      \brief check if player can reach by n_dash dashes
      \param n_turn the number of tunes to be used
      \param n_dash the number of dashes to be used
      \param player const reference to the player object
      \param player_type player type parameter
      \param control_area player's ball controllable radius
      \param ball_pos ball position 'cycle' cycles later
      \return true if player can get the ball
    */
    bool canReachAfterDash( const int n_turn,
                            const int n_dash,
                            const PlayerObject & player,
                            const PlayerType & player_type,
                            const double & control_area,
                            const Vector2D & ball_pos ) const;

    /*!
      \brief predict player's reachable cycle to the ball final point
      \param player const reference to the player object
      \param player_type player type parameter
      \return predicted cycle value
    */
    int predictFinal( const PlayerObject & player,
                      const PlayerType & player_type ) const;

};

}

#endif

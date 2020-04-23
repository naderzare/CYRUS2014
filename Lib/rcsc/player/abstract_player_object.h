// -*-c++-*-

/*!
  \file abstract_player_object.h
  \brief abstract player object class Header File
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

#ifndef RCSC_PLAYER_ABSTRACT_PLAYER_OBJECT_H
#define RCSC_PLAYER_ABSTRACT_PLAYER_OBJECT_H

#include <rcsc/player/localization.h>
#include <rcsc/common/player_type.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/types.h>

#include <vector>

namespace rcsc {

class AbstractPlayerObject;
class PlayerEvaluator;

typedef std::vector< const AbstractPlayerObject * > AbstractPlayerCont;

/*!
  \class AbstractPlayerObject
  \brief abstact player object class
*/
class AbstractPlayerObject {
protected:

    SideID M_side; //!< team side
    int  M_unum; //!< uniform number
    int M_unum_count; //!< accuracy count
    bool M_goalie; //!< goalie flag

    int M_type; //!< player type id
    const PlayerType * M_player_type; //!< player type reference
    Card M_card; //!< card information

    Vector2D M_pos; //!< global coordinate
    int M_pos_count; //!< main accuracy counter

    Vector2D M_seen_pos; //!< last seen global coordinate
    int M_seen_pos_count; //!< count since last observation

    Vector2D M_heard_pos; //!< last heard global coordinate
    int M_heard_pos_count; //!< count since last observation

    Vector2D M_vel; //!< velocity
    int M_vel_count; //!< accuracy count

    Vector2D M_seen_vel; //!< last seen velocity
    int M_seen_vel_count; //!< count since last observation

    AngleDeg M_body; //!< global body angle
    int M_body_count; //!< body angle accuracy
    AngleDeg M_face; //!< global neck angle
    int M_face_count; //!< face angle accuracy

    bool M_kicked; //!< kicking state

    double M_dist_from_ball; //!< distance from ball
    AngleDeg M_angle_from_ball; //!< angle from ball
    double M_dist_from_self; //!< distance from self
    AngleDeg M_angle_from_self; //!< angle from self

public:

    /*!
      \brief initialize member variables.
    */
    AbstractPlayerObject();

    /*!
      \brief initialize member variables using observed info
      \param side analyzed side info
      \param p analyzed seen player info
    */
    AbstractPlayerObject( const SideID side,
                          const Localization::PlayerT & p );

    /*!
      \brief destructor. nothing to do
    */
    virtual
    ~AbstractPlayerObject()
      { }


    // ------------------------------------------
    /*!
      \brief check if this player is self or not
      \return true if this player is self
     */
    virtual
    bool isSelf() const
      {
          return false;
      }

    /*!
      \brief check if this player is ghost object or not
      \return true if this player may be ghost object
     */
    virtual
    bool isGhost() const
      {
          return false;
      }

    /*!
      \brief get the counter value as a ghost recognition
      \return count as a ghost recognition
     */
    virtual
    int ghostCount() const
      {
          return 0;
      }

    /*!
      \brief check if player is tackling or not
      \return checked result
     */
    virtual
    bool isTackling() const = 0;

    /*!
      \brief update player type id
      \param type new player type id
     */
    virtual
    void setPlayerType( const int type );

    /*!
      \brief update card state
      \param card new card type
     */
    void setCard( const Card card )
      {
          M_card = card;
      }

    // ------------------------------------------

    /*!
      \brief get team side id
      \return side id (LEFT,RIGHT,NEUTRAL)
    */
    SideID side() const
      {
          return M_side;
      }

    /*!
      \brief get player's uniform number
      \return uniform number. if unknown player, returned -1
    */
    int unum() const
      {
          return M_unum;
      }

    /*!
      \brief get uniform number accuracy count
      \return count since last observation
     */
    int unumCount() const
      {
          return M_unum_count;
      }

    /*!
      \brief get goalie flag
      \return true if this player is goalie
    */
    bool goalie() const
      {
          return M_goalie;
      }

    /*!
      \brief get the player type id
      \return player type id
     */
    int type() const
      {
          return M_type;
      }

    /*!
      \brief get the player type as a pointer.
      \return player type pointer variable
     */
    const
    PlayerType * playerTypePtr() const
      {
          return M_player_type;
      }

    /*!
      \brief get card type
      \return card type{NO_CARD,YELLOW,RED}
     */
    Card card() const
      {
          return M_card;
      }

    /*!
      \brief get global position
      \return const reference to the point object
    */
    const
    Vector2D & pos() const
      {
          return M_pos;
      }

    /*!
      \brief get global position accuracy
      \return count since last observation
    */
    int posCount() const
      {
          return M_pos_count;
      }

    /*!
      \brief get the last seen position
      \return const reference to the point object
     */
    const
    Vector2D & seenPos() const
      {
          return M_seen_pos;
      }

    /*!
      \brief get the number of cycles since last observation
      \return count since last seen
    */
    int seenPosCount() const
      {
          return M_seen_pos_count;
      }

    /*!
      \brief get the last heard position
      \return const reference to the point object
     */
    const
    Vector2D & heardPos() const
      {
          return M_heard_pos;
      }

    /*!
      \brief get the number of cycles since last observation
      \return count since last observation
    */
    int heardPosCount() const
      {
          return M_heard_pos_count;
      }

    /*!
      \brief get velocity
      \return const reference to the vector object
    */
    const
    Vector2D & vel() const
      {
          return M_vel;
      }

    /*!
      \brief get velocity accuracy
      \return count from last observation
    */
    int velCount() const
      {
          return M_vel_count;
      }

    /*!
      \brief get the last seen velocity
      \return const reference to the vector object
     */
    const
    Vector2D & seenVel() const
      {
          return M_seen_vel;
      }

    /*!
      \brief get the number of cycles since last observation
      \return count since last seen
    */
    int seenVelCount() const
      {
          return M_seen_vel_count;
      }

    /*!
      \brief get global body angle
      \return const reference to the angle object
    */
    const
    AngleDeg & body() const
      {
          return M_body; // global body angle
      }

    /*!
      \brief get global body angle accuracy
      \return count from last observation
    */
    int bodyCount() const
      {
          return M_body_count;
      }

    /*!
      \brief get global neck angle
      \return const reference to the angle object
    */
    const
    AngleDeg & face() const
      {
          return M_face; // global neck angle
      }

    /*!
      \brief get global neck angle accuracy
      \return count from last observation
    */
    int faceCount() const
      {
          return M_face_count;
      }

    /*!
      \brief get kicking state information
      \return true if player performed the kick.
     */
    bool kicked() const
      {
          return M_kicked;
      }

    /*!
      \brief get distance from ball
      \return distance value from ball
    */
    const
    double & distFromBall() const
      {
          return M_dist_from_ball;
      }

    /*!
      \brief get angle from ball
      \return absolute angle value from ball
    */
    const
    AngleDeg & angleFromBall() const
      {
          return M_angle_from_ball;
      }

    /*!
      \brief get distance from self
      \return distance value from self
    */
    const
    double & distFromSelf() const
      {
          return M_dist_from_self;
      }

    /*!
      \brief get global angle from self position
      \return angle value from self position
    */
    const
    AngleDeg & angleFromSelf() const
      {
          return M_angle_from_self;
      }

    /*!
      \brief get current estimated kick power rate
      \return calculated kick rate value
    */
    virtual
    double kickRate() const;

    /*!
      \brief estimate reach point
      \param n_step this method estimates ball point after this steps
      \return estimated point vector
    */
    Vector2D inertiaPoint( const int n_step ) const
      {
          return ( playerTypePtr()
                   ? playerTypePtr()->inertiaPoint( pos(), vel(), n_step )
                   : Vector2D::INVALIDATED );
      }

    /*!
      \brief estimate final reach point
      \return estimated point vector
    */
    Vector2D inertiaFinalPoint() const
      {
          return ( playerTypePtr()
                   ? playerTypePtr()->inertiaFinalPoint( pos(), vel() )
                   : Vector2D::INVALIDATED );
      }

    // ------------------------------------------
    /*!
      \brief template method. check if player is in the region
      \param region template resion. REGION must have method contains()
      \return true if region contains player position
    */
    template < typename REGION >
    bool isWithin( const REGION & region ) const
      {
          return region.contains( this->pos() );
      }

    /*!
      \brief get minimum evaluation value within the input container using evaluator
      \param cont container of AbstractPlayerObject
      \param evaluator evaluator object (has to be dynamically allocated)
     */
    static double get_minimum_evaluation( const AbstractPlayerCont & cont,
                                          const PlayerEvaluator * evaluator );

    /*!
      \brief get maximum evaluation value within the input container using evaluator
      \param cont container of AbstractPlayerObject
      \param evaluator evaluator object (has to be dynamically allocated)
     */
    static double get_maximum_evaluation( const AbstractPlayerCont & cont,
                                          const PlayerEvaluator * evaluator );

};

}

#endif

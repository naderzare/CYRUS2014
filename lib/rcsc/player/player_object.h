// -*-c++-*-

/*!
  \file player_object.h
  \brief player object class Header File
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

#ifndef RCSC_PLAYER_PLAYER_OBJECT_H
#define RCSC_PLAYER_PLAYER_OBJECT_H

#include <rcsc/player/abstract_player_object.h>

#include <rcsc/player/localization.h>
#include <rcsc/player/fullstate_sensor.h>

#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/types.h>

#include <functional>

namespace rcsc {

/*!
  \class PlayerObject
  \brief observed player object class
*/
class PlayerObject
    : public AbstractPlayerObject {
private:
    //! validation count threshold value for M_pos and M_rpos
    static int S_pos_count_thr;
    //! validation count threshold value for M_vel
    static int S_vel_count_thr;
    //! validation count threshold value for M_body and M_face
    static int S_face_count_thr;

    AngleDeg M_angle_from_self; //!< angle from self global

    int M_ghost_count; //!< count since this object was recognized as a ghost object.

    Vector2D M_rpos; //!< relative coordinate
    int M_rpos_count; //!< relative coordinate accuracy counter

    AngleDeg M_pointto_angle; //!< global pointing angle
    int M_pointto_count; //!< pointing angle accuracy

    int M_tackle_count; //!< tackle info accuracy

    double M_heard_stamina; //!< heard stamina
public:

    /*!
      \brief initialize member variables.
    */
    PlayerObject();

    /*!
      \brief initialize member variables using observed info
      \param side analyzed side info
      \param p analyzed seen player info
    */
    PlayerObject( const SideID side,
                  const Localization::PlayerT & p );

    /*!
      \brief destructor. nothing to do
    */
    ~PlayerObject()
      { }

    /*!
      \brief set accuracy count threshold values.
      \param pos_thr threshold value for M_pos
      \param vel_thr threshold value for M_vel
      \param face_thr threshold value for M_body and M_face
    */
    static
    void set_count_thr( const int pos_thr,
                        const int vel_thr,
                        const int face_thr );

    // ------------------------------------------

    /*!
      \brief check if this player is ghost object or not
      \return true if this player may be ghost object
     */
    bool isGhost() const
      {
          return M_ghost_count > 0;
      }

    /*!
      \brief get the count of ghost check
      \return count of ghost check
     */
    int ghostCount() const
      {
          return M_ghost_count;
      }

    /*!
      \brief get position relative to self position
      \return const reference to the point object
    */
    const
    Vector2D & rpos() const
      {
          return M_rpos;
      }

    /*!
      \brief get global pointing angle
      \return const reference to the angle object
    */
    const
    AngleDeg & pointtoAngle() const
      {
          return M_pointto_angle; // global pointing angle
      }

    /*!
      \brief get global pointing angle accuracy
      \return count from last observation
    */
    int pointtoCount() const
      {
          return M_pointto_count;
      }

    /*!
      \brief get tackling status accuracy
      \return count from last observation
    */
    int tackleCount() const
      {
          return M_tackle_count;
      }

    /*!
      \brief check if player is tackling
      \return true if tackle accuracy is less than tackle cycles in ServerParam
    */
    bool isTackling() const;


    /*!
      \brief get the heard stamina inf
      \return heard stamina value
     */
    const
    double & heardStamina() const
      {
          return M_heard_stamina;
      }

    /*!
      \brief velify global position accuracy
      \return true if position has enough accuracy
    */
    bool posValid() const
      {
          return M_pos_count < S_pos_count_thr;
      }

    /*!
      \brief verify relative position accuracy
      \return true if relative position has enough accuracy
    */
    bool rposValid() const
      {
          return M_rpos_count < S_pos_count_thr;
      }

    /*!
      \brief verify velocity accuracy
      \return true if accuracy  has enough accuracy
    */
    bool velValid() const
      {
          return M_vel_count < S_vel_count_thr;
      }

    /*!
      \brief verify angle accuracy
      \return true if angle has enoubh accuracy
    */
    bool bodyValid() const
      {
          return M_body_count < S_face_count_thr;
      }

    /*!
      \brief verify angle accuracy
      \return true if angle has enoubh accuracy
    */
    bool faceValid() const
      {
          return M_face_count < S_face_count_thr;
      }

    /*!
      \brief check if player can kick the ball or not
      \brief buf kickable area buffer
      \return true if player can kick the ball
     */
    bool isKickable( const double & buf = 0.05 ) const;

    // ------------------------------------------
    /*!
      \brief update status only with intenal info
    */
    void update();

    /*!
      \brief increment ghost count
     */
    void setGhost()
      {
          ++M_ghost_count;
      }

    /*!
      \brief set player's team info
      \param side player's team side
      \param unum player's uniform number
      \param goalie goalie flag
     */
    void setTeam( const SideID side,
                  const int unum,
                  const bool goalie )
      {
          M_side = side;
          M_unum = unum;
          M_goalie = goalie;
      }

    /*!
      \brief update status using localized player info
      \param side analyzed side info
      \param p localized player info
    */
    void updateBySee( const SideID side,
                      const Localization::PlayerT & p );

    /*!
      \brief update status using fullstate info
      \param p fullstate player info
      \param self_pos global self position
      \param ball_pos global ball position
     */
    void updateByFullstate( const FullstateSensor::PlayerT & p,
                            const Vector2D & self_pos,
                            const Vector2D & ball_pos );

    /*!
      \brief update staus using heard info
      \param heard_side heard side info
      \param heard_unum heard uniform number
      \param goalie update goalie info, only if this value is true.
      \param heard_pos heard global position
    */
    void updateByHear( const SideID heard_side,
                       const int heard_unum,
                       const bool goalie,
                       const Vector2D & heard_pos );

    /*!
      \brief update staus using heard info
      \param heard_side heard side info
      \param heard_unum heard uniform number
      \param goalie update goalie info, only if this value is true.
      \param heard_pos heard global position
      \param heard_body heard body angle
      \param heard_stamina heard stamina value
    */
    void updateByHear( const SideID heard_side,
                       const int heard_unum,
                       const bool goalie,
                       const Vector2D & heard_pos,
                       const double & heard_body,
                       const double & heard_stamina );

    /*!
      \brief update status related to other objects
      \param self self position
      \param ball ball position

      This method is called just before decide action
    */
    void updateSelfBallRelated( const Vector2D & self,
                                const Vector2D & ball );

    /*!
      \brief reset accuracy info
    */
    void forget();

    ///////////////////////////////////////////////////////////////
    /*!
      \class UpdateOp
      \brief functor to update
    */
    class UpdateOp
        : public std::unary_function< PlayerObject, void > {
    public:
        /*!
          \brief operation function
          \param player reference to the updated player
         */
        result_type operator()( argument_type & player )
          {
              player.update();
          }
    };

    /*!
      \class IsInvalidOp
      \brief functor to check if player has enough accuracy
    */
    class IsInvalidOp
        : public std::unary_function< PlayerObject, bool > {
    public:
        /*!
          \brief operation function
          \param player reference to the updated player.
          \return true if player's position accuracy is low.
         */
        result_type operator()( const argument_type & player ) const
          {
              return ( ! player.posValid() );
          }
    };

    ///////////////////////////////////////////////////////////////
    /*!
      \class CountCmp
      \brief predicate functor to compare player's accuracy. reference version
    */
    class CountCmp
        : public std::binary_function< PlayerObject, PlayerObject, bool > {
    public:
        /*!
          \brief operation function
          \param lhs left hand side variable
          \param rhs right hand side variable
          \return compared result
         */
        result_type operator()( const first_argument_type & lhs,
                                const second_argument_type & rhs ) const
          {
              if ( lhs.goalie() ) return true;
              if ( rhs.goalie() ) return false;
              return lhs.posCount() < rhs.posCount();
          }
    };

    /*!
      \class PtrCountCmp
      \brief predicate functor to compare player's accuracy. pointer version
    */
    class PtrCountCmp
        : public std::binary_function< const PlayerObject *,
                                       const PlayerObject *,
                                       bool > {
    public:
        /*!
          \brief operation function
          \param lhs left hand side variable
          \param rhs right hand side variable
          \return compared result
         */
        result_type operator()( first_argument_type lhs,
                                second_argument_type rhs ) const
          {
              if ( lhs->goalie() ) return true;
              if ( rhs->goalie() ) return false;
              return lhs->posCount() < rhs->posCount();
          }
    };

    ///////////////////////////////////////////////////////////////
    /*!
      \class PtrSelfDistCmp
      \brief predicate functor to compare player's distance from self
    */
    class PtrSelfDistCmp
        : public std::binary_function< const PlayerObject *,
                                       const PlayerObject *,
                                       bool > {
    public:
        /*!
          \brief operation function
          \param lhs left hand side variable
          \param rhs right hand side variable
          \return compared result
         */
        result_type operator()( first_argument_type lhs,
                                second_argument_type rhs ) const
          {
              return lhs->distFromSelf() < rhs->distFromSelf();
          }
    };

    /*!
      \class PtrBallDstCmp
      \brief predicate functor to compare player's distance from ball
    */
    class PtrBallDistCmp
        : public std::binary_function< const PlayerObject *,
                                       const PlayerObject *,
                                       bool > {
    public:
        /*!
          \brief operation function
          \param lhs left hand side variable
          \param rhs right hand side variable
          \return compared result
         */
        result_type operator()( first_argument_type lhs,
                                second_argument_type rhs ) const
          {
              return lhs->distFromBall() < rhs->distFromBall();
          }
    };

};

//! type of the player object container
typedef std::list< PlayerObject > PlayerCont;
//! type of the player object pointer container
typedef std::vector< PlayerObject * > PlayerPtrCont;

}

#endif

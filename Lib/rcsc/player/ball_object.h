// -*-c++-*-

/*!
  \file ball_object.h
  \brief ball object class Header File
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

#ifndef RCSC_PLAYER_BALL_OBJECT_H
#define RCSC_PLAYER_BALL_OBJECT_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

#include <deque>

namespace rcsc {

class GameMode;
class ActionEffector;
class SelfObject;

/*!
  \class BallObject
  \brief observed ball object class

  order of update process.
  update()
  -> updateAll() (updatePos(),updateOnlyVel(), updateOnlyRelativePos())
  -> updateByHeardInfo()
  -> updateByCollision()
  -> updateSelfRelated()
*/
class BallObject {
private:
    //! validation count threshold value for M_pos
    static int S_pos_count_thr;
    //! validation count threshold value for M_rpos
    static int S_rpos_count_thr;
    //! validation count threshold value for M_vel
    static int S_vel_count_thr;

    static const std::size_t MAX_RECORD; //!< max record size

public:
    /*!
      \brief internal state representation
     */
    struct State {
        Vector2D pos_; //!< estimated global position
        Vector2D pos_error_; //!< estimated error of global position
        int pos_count_; //!< accuracy count. this is usually the cycle count since last observation

        Vector2D rpos_; //!< estimated relative position
        Vector2D rpos_error_; //!< estimated error fo relative position
        int rpos_count_; //!< accuracy count. this is usually the cycle count since last observation

        Vector2D seen_pos_; //!< seen global position
        Vector2D seen_rpos_; //!< seen relative position
        int seen_pos_count_; //!< accuracy count. the cycle count since last see update

        Vector2D heard_pos_; //!< heard global position
        int heard_pos_count_; //!< accuracy count. the cycle count since last hear update

        Vector2D vel_; //!< estimated velocity
        Vector2D vel_error_; //!< estimated error of velocity
        int vel_count_; //!< accuracy count. this is usually the cycle count since last observation

        Vector2D seen_vel_; //!< seen velocity
        int seen_vel_count_; //!< accuracy count. the cycle count since last see update

        Vector2D heard_vel_; //!< heard velocity
        int heard_vel_count_; //!< accuracy count. the cycle count since last hear update

        int lost_count_; //!< the cycle count since ball lost

        int ghost_count_; //!< ghost flag

        /*!
          \brief initialize all variables
         */
        State();
    };

private:

    //! current state
    State M_state;

    std::deque< State > M_state_record;

    //! estimated distance from self
    double M_dist_from_self;
    //! estimated global angle from self
    AngleDeg M_angle_from_self;

    //! relative position at previous cycle. updated only in pure internal update
    Vector2D M_rpos_prev;


    // not used
    BallObject( const BallObject & ball );
    BallObject & operator=( const BallObject & ball );

public:
    /*!
      \brief constructor. initialize member variables
    */
    BallObject();

    /*!
      \brief set accuracy count threshold values.
      \param pos_thr threshold value for M_pos
      \param rpos_thr threshold value for M_rpos
      \param vel_thr threshold value for M_vel
    */
    static
    void set_count_thr( const int pos_thr,
                        const int rpos_thr,
                        const int vel_thr );

    /*!
      \brief get current state
      \return const reference to the state object
     */
    const
    State & state() const
      {
          return M_state;
      }

    /*!
      \brief get state record container
      \return const reference to the state container
     */
    const
    std::deque< State > & stateRecord() const
      {
          return M_state_record;
      }

    /*!
      \brief get estimated distance from self
      \return distance value
    */
    const
    double & distFromSelf() const
      {
          return M_dist_from_self;
      }
    /*!
      \brief get estimated global angle from self
      \return const reference to angle object
    */
    const
    AngleDeg & angleFromSelf() const
      {
          return M_angle_from_self;
      }

    /*!
      \brief get estimated global coordinate
      \return const reference to point object
    */
    const
    Vector2D & pos() const
      {
          return M_state.pos_;
      }

    /*!
      \brief get estimated error of global coordinate
      \return const reference to vector object
    */
    const
    Vector2D & posError() const
      {
          return M_state.pos_error_;
      }

    /*!
      \brief get global position accuracy count
      \return cycle count from last observation
    */
    int posCount() const
      {
          return M_state.pos_count_;
      }

    /*!
      \brief get estimated position relative from self
      \return const referenct to point object
    */
    const
    Vector2D & rpos() const
      {
          return M_state.rpos_;
      }

    /*!
      \brief get estimated error of relative coordinate
      \return const reference to vector object
    */
    const
    Vector2D & rposError() const
      {
          return M_state.rpos_error_;
      }

    /*!
      \brief get relative position accuracy count
      \return cycle count from last observation
    */
    int rposCount() const
      {
          return M_state.rpos_count_;
      }

    /*!
      \brief get previous cycle relative position
      \return const reference to point object
    */
    const
    Vector2D & rposPrev() const
      {
          return M_rpos_prev;
      }

    /*!
      \brief get the last seen position
      \return const reference to the point object
     */
    const
    Vector2D & seenPos() const
      {
          return M_state.seen_pos_;
      }

    /*!
      \brief get the number of cycles since last seen
      \return count since last seen
    */
    int seenPosCount() const
      {
          return M_state.seen_pos_count_;
      }

    /*!
      \brief get the last seen relative position
      \return const reference to the variable
     */
    const
    Vector2D & seenRPos() const
      {
          return M_state.seen_rpos_;
      }

    /*!
      \brief get the last heard position
      \return const reference to the point object
     */
    const
    Vector2D & heardPos() const
      {
          return M_state.heard_pos_;
      }

    /*!
      \brief get the number of cycles since last observation
      \return count since last observation
    */
    int heardPosCount() const
      {
          return M_state.heard_pos_count_;
      }

    /*!
      \brief get estimated velocity
      \return const referenct to vector object
    */
    const
    Vector2D & vel() const
      {
          return M_state.vel_;
      }

    /*!
      \brief get estimated error of velocity
      \return const reference to vector object
    */
    const
    Vector2D & velError() const
      {
          return M_state.vel_error_;
      }

    /*!
      \brief get velocity accuracy count
      \return cycle count from last observation
    */
    int velCount() const
      {
          return M_state.vel_count_;
      }
    /*!
      \brief get the last seen velocity
      \return const reference to the point object
     */
    const
    Vector2D & seenVel() const
      {
          return M_state.seen_vel_;
      }

    /*!
      \brief get the number of cycles since last velocity seen
      \return count since last velocity seen
    */
    int seenVelCount() const
      {
          return M_state.seen_vel_count_;
      }

    /*!
      \brief get count since ball lost
      \return cycle count since last observation
    */
    int lostCount() const
      {
          return M_state.lost_count_;
      }

    /*!
      \brief velify global position accuracy
      \return true if position has enough accuracy
    */
    bool posValid() const
      {
          return M_state.pos_count_ < S_pos_count_thr;
      }

    /*!
      \brief velify relative position accuracy
      \return true if relative position has enough accuracy
    */
    bool rposValid() const
      {
          return M_state.rpos_count_ < S_rpos_count_thr;
      }

    /*!
      \brief verify velocity accuracy
      \return true if velocity has enough accuracy
    */
    bool velValid() const
      {
          return M_state.vel_count_ < S_vel_count_thr;
      }

    /*!
      \brief clear all confidence values and set ghost time
      \param current current game time
    */
    void setGhost( const GameTime & current );

    /*!
      \brief update status only with intenal info
      \param act const reference to action effector
      \param game_mode const reference to referee info
      \param current current game time
    */
    void update( const ActionEffector & act,
                 const GameMode & game_mode,
                 const GameTime & current );

    /*!
      \brief update status with fullstate info
      \param pos no error position in fullstate info
      \param vel no error velocity in fullstate info
      \param self_pos global self position
     */
    void updateByFullstate( const Vector2D & pos,
                            const Vector2D & vel,
                            const Vector2D & self_pos );
private:
    /*!
      \brief apply wind effect

      This method is called only from update()
    */
    void updateWindEffect();

public:
    /*!
      \brief apply collision effect
      \param pos new global position
      \param pos_count accuracy counter
      \param rpos new relative position
      \param rpos_count accuracy counter
      \param vel new velocity
      \param vel_count accuracy counter

      Thie method is called when collision estimatation
    */
    void updateByCollision( const Vector2D & pos,
                            const int pos_count,
                            const Vector2D & rpos,
                            const int rpos_count,
                            const Vector2D & vel,
                            const int vel_count );

    /*!
      \brief update relative position using see info.
      \param rpos observed relative position
      \param rpos_err estimated error of relative position
    */
    void updateOnlyRelativePos( const Vector2D & rpos,
                                const Vector2D & rpos_err );

    /*!
      \brief update velocity using see info
      \param vel estimated velocity
      \param vel_err estimated error of velocity
      \param vel_count new accuracy value of the velocity
    */
    void updateOnlyVel( const Vector2D & vel,
                        const Vector2D & vel_err,
                        const int vel_count );

    /*!
      \brief update by opponent control effect
     */
    void setOpponentControlEffect();

    /*!
      \brief update position by see info (not include velocity)
      \param pos estimated global position
      \param pos_err estimated error of global position
      \param pos_count new accuracy value of global position.
      usually same as the self position accuracy.
      \param rpos estimated relative position
      \param rpos_err estimated error of relative position
    */
    void updatePos( const Vector2D & pos,
                    const Vector2D & pos_err,
                    const int pos_count,
                    const Vector2D & rpos,
                    const Vector2D & rpos_err );

    /*!
      \brief update all status by see info
      \param pos estimated global position
      \param pos_err estimated error of global position
      \param pos_count new accuracy value of global position.
      usually same as the self position accuracy.
      \param rpos estimated relative position
      \param rpos_err estimated error of relative position
      \param vel estimated velocity
      \param vel_err estimated error of velocity
      \param vel_count new accuracy value of the velocity
    */
    void updateAll( const Vector2D & pos,
                    const Vector2D & pos_err,
                    const int pos_count,
                    const Vector2D & rpos,
                    const Vector2D & rpos_err,
                    const Vector2D & vel,
                    const Vector2D & vel_err,
                    const int vel_count );

    /*!
      \brief update ball status using heared info
      \param act const referenct to the ActionEffector instance
      \param sender_to_ball_dist distance from message sender to ball
      \param heard_pos heard position
      \param heard_vel heard velocity

      This method is called just before decision.
    */
    void updateByHear( const ActionEffector & act,
                       const double & sender_to_ball_dist,
                       const Vector2D & heard_pos,
                       const Vector2D & heard_vel );

    /*!
      \brief update self related info
      \param self const reference to the self object

      This method is called just before decision.
    */
    void updateSelfRelated( const SelfObject & self );

    /*!
      \brief update historical record data
     */
    void updateRecord();
    // ------------------------------------------
    // utilities


    /*!
      \brief get the recorded state
      \param history_index desired index. 0 means current, 1 means previous cycle data
      \return state pointer. if no matched data, NULL is returned.
     */
    const State * getState( const size_t history_index ) const;

    /*!
      \brief template method. check if ball is in the region
      \param region template resion. REGION must have method contains()
      \return true if region contains ball position
    */
    template < typename REGION >
    bool isWithin( const REGION & region ) const
      {
          return region.contains( this->pos() );
      }

    // inertia movement calculators

    /*!
      \brief estimate movement travel vector
      \param cycle this method estimates ball travel after this steps
      \return estimated travel vector
    */
    Vector2D inertiaTravel( const int cycle ) const;

    /*!
      \brief estimate reach point
      \param cycle this method estimates ball point after this steps
      \return estimated point vector
    */
    Vector2D inertiaPoint( const int cycle ) const;


    /*!
      \brief estimate reach point
      \return estimated point
    */
    Vector2D inertiaFinalPoint() const;

    /*!
      \brief estimate final reach point
      \return estimated point vector
    */
    static
    double calc_travel_step( const double & distance,
                             const double & first_speed );
};

}

#endif

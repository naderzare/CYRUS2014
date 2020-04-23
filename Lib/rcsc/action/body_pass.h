// -*-c++-*-

/*!
  \file body_pass.h
  \brief advanced pass planning & behavior.
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

#ifndef RCSC_ACTION_BODY_PASS_H
#define RCSC_ACTION_BODY_PASS_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

#include <functional>
#include <vector>

namespace rcsc {

class WorldModel;
class PlayerObject;

/*!
  \class Body_Pass
  \brief advanced pass planning & behavior.
 */
class Body_Pass
    : public BodyAction {
public:
    /*!
      \enum PassType
      \brief pass type id
     */
    enum PassType {
        DIRECT  = 1,
        LEAD    = 2,
        THROUGH = 3
    };

    /*!
      \struct PassRoute
      \brief pass route information object, that contains type,
      receiver info, receive point and ball first speed.
     */
    struct PassRoute {
        PassType type_; //!< pass type id
        const PlayerObject * receiver_; //!< pointer to the receiver player
        Vector2D receive_point_; //!< estimated receive point
        double first_speed_; //!< ball first speed
        bool one_step_kick_; //!< true if ball reaches first speed only by one kick.
        double score_; //!< evaluated value of this pass

        /*!
          \brief construct with all member variables
          \param type pass type id
          \param receiver pointer to the receiver player
          \param point pass receive point
          \param speed ball first speed
          \param one_step_kick true if ball reaches first speed only by one kick.
         */
        PassRoute( PassType type,
                   const PlayerObject * receiver,
                   const Vector2D & point,
                   const double & speed,
                   const bool one_step_kick )
            : type_( type )
            , receiver_( receiver )
            , receive_point_( point )
            , first_speed_( speed )
            , one_step_kick_( one_step_kick )
            , score_( 0.0 )
          { }
    };

    /*!
      \class PassRouteScoreComp
      \brief function object to evaluate the pass
     */
    class PassRouteScoreComp
        : public std::binary_function< PassRoute, PassRoute, bool > {
    public:
        /*!
          \brief compare operator
          \param lhs left hand side argument
          \param rhs right hand side argument
          \return compared result
         */
        result_type operator()( const first_argument_type & lhs,
                                const second_argument_type & rhs ) const
          {
              return lhs.score_ < rhs.score_;
          }
    };

private:

    //! cached calculated pass data
    static std::vector< PassRoute > S_cached_pass_route;


public:
    /*!
      \brief accessible from global.
    */
    Body_Pass()
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool execute( PlayerAgent * agent );

    /*!
      \brief calculate best pass route
      \param world consr rerefence to the WorldModel
      \param target_point receive target point is stored to this
      \param first_speed ball first speed is stored to this
      \param receiver receiver number
      \return true if pass route is found.
    */
    static
    bool get_best_pass( const WorldModel & world,
                        Vector2D * target_point,
                        double * first_speed,
                        int * receiver );

private:
    static
    void create_routes( const WorldModel & world );

    static
    void create_direct_pass( const WorldModel & world,
                             const PlayerObject * teammates );
    static
    void create_lead_pass( const WorldModel & world,
                           const PlayerObject * teammates );
    static
    void create_through_pass( const WorldModel & world,
                              const PlayerObject * teammates );

    static
    bool verify_direct_pass( const WorldModel & world,
                             const PlayerObject * receiver,
                             const Vector2D & target_point,
                             const double & target_dist,
                             const AngleDeg & target_angle,
                             const double & first_speed );
    static
    bool verify_through_pass( const WorldModel & world,
                              const PlayerObject * receiver,
                              const Vector2D & receiver_pos,
                              const Vector2D & target_point,
                              const double & target_dist,
                              const AngleDeg & target_angle,
                              const double & first_speed,
                              const double & reach_step );

    static
    void evaluate_routes( const WorldModel & world );

    static
    bool can_kick_by_one_step( const WorldModel & world,
                               const double & first_speed,
                               const AngleDeg & target_angle );
};

}

#endif

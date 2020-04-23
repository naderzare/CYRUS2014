// -*-c++-*-

/*!
  \file kick_table.h
  \brief kick table class Header File
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

#ifndef RCSC_ACTION_KICK_TABLE_H
#define RCSC_ACTION_KICK_TABLE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/angle_deg.h>

#include <vector>
#include <algorithm>

namespace rcsc {

class GameTime;
class PlayerType;
class WorldModel;

/*-------------------------------------------------------------------*/
/*!
  \class KickTabke
  \brief kick table to generate smart kick.
*/
class KickTable {
public:

    static const double NEAR_SIDE_RATE; //!< kickable margin rate for the near side sub-target
    static const double MID_RATE; //!< kickable margin rate for the middle distance sub-target
    static const double FAR_SIDE_RATE; //!< kickable margin rate for the far side sub-target

    enum {
        MAX_DEPTH = 2,
    };

    enum {
        STATE_DIVS_NEAR = 8,
        STATE_DIVS_MID = 12,
        STATE_DIVS_FAR = 15,
        NUM_STATE = STATE_DIVS_NEAR + STATE_DIVS_MID + STATE_DIVS_FAR,
    };

    enum {
        DEST_DIR_DIVS = 72, // step: 5 degree
    };

    enum {
        MAX_TABLE_SIZE = 256,
    };

    /*!
      \enum Flag
      \brief status bit flags
     */
    enum Flag {
        SAFETY = 0x0000,
        NEXT_TACKLABLE = 0x0001,
        NEXT_KICKABLE = 0x0002,
        TACKLABLE = 0x0004,
        KICKABLE = 0x0008,
        SELF_COLLISION = 0x0010,
        RELEASE_INTERFERE = 0x0020,
        MAYBE_RELEASE_INTERFERE = 0x0040,
        OUT_OF_PITCH = 0x0080,
        KICK_MISS_POSSIBILITY = 0x0100,
    };

    /*!
      \struct State
      \brief class to represent a kick intermidiate state
    */
    struct State {
        int index_; //!< index of this point
        double dist_; //!< distance from self
        Vector2D pos_; //!< position relative to player's body
        double kick_rate_; //!< kick rate
        int flag_; //!< status bit flag

        /*!
          \brief construct an illegal state object
         */
        State()
            : index_( -1 )
            , dist_( 0.0 )
            , kick_rate_( 0.0 )
            , flag_( 0xFFFF )
          { }

        /*!
          \brief construct a legal state object. flag is set to SAFETY.
          \param index index number of this state
          \param dist distance from self
          \param pos global posigion
          \param kick_rate kick rate at this state
         */
        State( const int index,
               const double & dist,
               const Vector2D & pos,
               const double & kick_rate )
            : index_( index )
            , dist_( dist )
            , pos_( pos )
            , kick_rate_( kick_rate )
            , flag_( SAFETY )
          { }

    };

    /*!
      \struct Path
      \brief used as a heuristic knowledge. path representation between two states
     */
    struct Path {
        int origin_; //!< index of origin state
        int dest_; //!< index of destination state
        double max_speed_; //!< reachable ball max speed
        double power_; //!< kick power to generate max_speed_

        /*!
          \brief construct a kick path object
          \param origin index of origin state
          \param dest index of destination state
         */
        Path( const int origin,
              const int dest )
            : origin_( origin )
            , dest_( dest )
            , max_speed_( 0.0 )
            , power_( 1000.0 )
          { }

    };

    /*!
      \struct Sequence
      \brief simulated kick sequence
     */
    struct Sequence {
        int flag_; //!< safety level flags. usually the combination of State flags
        std::vector< Vector2D > pos_list_; //!< ball positions
        double speed_; //!< released ball speed
        double power_; //!< estimated last kick power
        double score_; //!< evaluated score of this sequence

        /*!
          \brief constuct an illegal sequence object
         */
        Sequence()
            : flag_( 0x0000 )
            , speed_( 0.0 )
            , power_( 10000.0 )
            , score_( 0.0 )
          { }

        /*!
          \brief copy constructor
          \param arg another instance
         */
        Sequence( const Sequence & arg )
            : flag_( arg.flag_ )
            , pos_list_( arg.pos_list_ )
            , speed_( arg.speed_ )
            , power_( arg.power_ )
            , score_( arg.score_ )
          { }

        /*!
          \brief copy operator
          \param arg another instance
          \return const reference to this instance
         */
        const
        Sequence & operator=( const Sequence & arg )
          {
              if ( this != &arg )
              {
                  flag_ = arg.flag_;
                  pos_list_ = arg.pos_list_;
                  speed_ = arg.speed_;
                  power_ = arg.power_;
                  score_ = arg.score_;
              }

              return *this;
          }
    };

    /*!
      \brief caclulate the distance of near side sub-target
      \param player_type calculated PlayerType
      \return distance from the center of the player
     */
    static
    double calc_near_dist( const PlayerType & player_type );

    /*!
      \brief caclulate the distance of middile distance sub-target
      \param player_type calculated PlayerType
      \return distance from the center of the player
     */
    static
    double calc_mid_dist( const PlayerType & player_type );

    /*!
      \brief caclulate the distance of far side sub-target
      \param player_type calculated PlayerType
      \return distance from the center of the player
     */
    static
    double calc_far_dist( const PlayerType & player_type );

    /*!
      \brief calculate maxmum velocity for the target angle by one step kick with krate and ball_vel
      \param target_angle target angle of the next ball velocity
      \param krate current kick rate
      \param ball_vel current ball velocity
      \return maxmum velocity for the target angle
     */
    static
    Vector2D calc_max_velocity( const AngleDeg & target_angle,
                                const double & krate,
                                const Vector2D & ball_vel );

private:

    //
    // offline data
    //

    //! default player size
    double M_player_size;
    //! default kickable margin
    double M_kickable_margin;
    //! default ball size
    double M_ball_size;

    //! static state list
    std::vector< State > M_state_list;

    //! static heuristic table
    std::vector< Path > M_tables[DEST_DIR_DIVS];

    //
    // online data
    //

    //! current state cache
    State M_current_state;

    //! future state cache
    std::vector< State > M_state_cache[MAX_DEPTH];

    //! result kick sequences
    std::vector< Sequence > M_candidates;

    /*!
      \brief private constructor for singleton
     */
    KickTable();

    // not used
    KickTable( const KickTable & );
    const KickTable & operator=( const KickTable & );

private:

    /*!
      \brief create static state list
     */
    void createStateList( const PlayerType & player_type );

    /*!
      \brief create table for angle
      \param angle target angle relative to body angle
      \param table referecne to the container variable
     */
    void createTable( const AngleDeg & angle,
                      std::vector< Path > & table );

    /*!
      \brief update internal state
      \param world const rererence to the WorldModel
     */
    void updateState( const WorldModel & world );

    /*!
      \brief implementation of the state update
      \param world const rererence to the WorldModel
     */
    void createStateCache( const WorldModel & world );

    /*!
      \brief update collision flag of state caches for the target_point and first_speed
      \param world const rererence to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
     */
    void checkCollisionAfterRelease( const WorldModel & world,
                                     const Vector2D & target_point,
                                     const double & first_speed );

    /*!
      \brief update interfere level at state
      \param world const reference to the WorldModel
      \param cycle the cycle delay for state
      \param state reference to the State variable to be updated
     */
    void checkInterfereAt( const WorldModel & world,
                           const int cycle,
                           State & state );

    /*!
      \brief update interfere level after release kick for all states
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
     */
    void checkInterfereAfterRelease( const WorldModel & world,
                                     const Vector2D & target_point,
                                     const double & first_speed );

    /*!
      \brief update interfere level after release kick for each state
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
      \param cycle the cycle delay for state
      \param state reference to the State variable to be updated
     */
    void checkInterfereAfterRelease( const WorldModel & world,
                                     const Vector2D & target_point,
                                     const double & first_speed,
                                     const int cycle,
                                     State & state );

    /*!
      \brief simulate one step kick
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
     */
    bool simulateOneStep( const WorldModel & world,
                          const Vector2D & target_point,
                          const double & first_speed );

    /*!
      \brief simulate two step kicks
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
     */
    bool simulateTwoStep( const WorldModel & world,
                          const Vector2D & target_point,
                          const double & first_speed );

    /*!
      \brief simulate three step kicks
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
     */
    bool simulateThreeStep( const WorldModel & world,
                            const Vector2D & target_point,
                            const double & first_speed );

    /*!
      \brief evaluate candidate kick sequences
      \param first_speed required first speed
      \param allowable_speed required first speed threshold
     */
    void evaluate( const double & first_speed,
                   const double & allowable_speed );

public:

    /*!
      \brief singleton interface
      \return reference to the singleton instance
     */
    static
    KickTable & instance();

    /*!
      \brief create heuristic table
      \return result of table creation
     */
    bool createTables();

    /*!
      \brief read table data from file
      \param file_path file path to read
      \return read result
     */
    bool read( const std::string & file_path );

    /*!
      \brief write table data to file
      \param file_path file path to write
      \return write result
     */
    bool write( const std::string & file_path );

    /*!
      \brief simulate kick sequence
      \param world const reference to the WorldModel
      \param target_point kick target point
      \param first_speed required first speed
      \param allowable_speed required first speed threshold
      \param max_step maximum size of kick sequence
      \param sequence reference to the result variable
      \return if successful kick is found, then true, else false is returned but kick sequence is generated anyway.
     */
    bool simulate( const WorldModel & world,
                   const Vector2D & target_point,
                   const double & first_speed,
                   const double & allowable_speed,
                   const int max_step,
                   Sequence & sequence );

    /*!
      \brief get the candidate kick sequences
      \return const reference to the container of Sequence
     */
    const
    std::vector< Sequence > & candidates() const
      {
          return M_candidates;
      }

};

}

#endif

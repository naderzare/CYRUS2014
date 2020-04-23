// -*-c++-*-

/*!
  \file stamina_model.h
  \brief player's stamina model Header File
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

#ifndef RCSC_PLAYER_STAMINA_MODEL_H
#define RCSC_PLAYER_STAMINA_MODEL_H

namespace rcsc {

class PlayerType;
class GameTime;

/*!
  \class StaminaModel
  \brief stamina management class
*/
class StaminaModel {
private:

    //! stamina value
    double M_stamina;
    //! effort rate
    double M_effort;
    //! recover rate
    double M_recovery;
    //! remained stamina capacity
    double M_capacity;

public:
    /*!
      \brief init members by built-in values
    */
    StaminaModel();

    /*!
      \brief initialize internal variables with server settings
      \param player_type heterogeneous player type
    */
    void init( const PlayerType & player_type );

    //
    // accessor method
    //

    /*!
      \brief get current stamina value
      \return const reference to the current stamina value
    */
    const
    double & stamina() const
      {
          return M_stamina;
      }

    /*!
      \brief get current recovery value
      \return const reference to the current recovery value
    */
    const
    double & recovery() const
      {
          return M_recovery;
      }

    /*!
      \brief get current effort value
      \return const reference to the current effort value
    */
    const
    double & effort() const
      {
          return M_effort;
      }

    /*!
      \brief get the current remained stamina capacity
      \return stamina capacity value
     */
    const
    double & capacity() const
      {
          return M_capacity;
      }

    /*!
      \brief check if stamina capacity is empty.
      \return checked result.
     */
    bool capacityIsEmpty() const
      {
          return M_capacity <= 1.0e-5
              && M_capacity >= 0.0;
      }

    //
    // interface for update
    //

    /*!
      \brief update by sense_body information
      \param sensed_stamina stamina value informed by sense_body
      \param sensed_effort effort value informed by sense_body
      \param sensed_capacity stamina capacity value informed by sense_body
      \param current game time that sense_body was received
    */
    void updateBySenseBody( const double & sensed_stamina,
                            const double & sensed_effort,
                            const double & sensed_capacity,
                            const GameTime & current );

    /*!
      \brief update by fullstate information
      \param fullstate_stamina stamina value informed by fullstate
      \param fullstate_effort effort value informed by fullstate
      \param fullstate_recovery recovery value informed by fullstate
      \param fullstate_capacity stamina capacity value informed by fullstate
     */
    void updateByFullstate( const double & fullstate_stamina,
                            const double & fullstate_effort,
                            const double & fullstate_recovery,
                            const double & fullstate_capacity );

    //
    // utility for prediction
    //

    /*!
      \brief simulate stamina variables after one wait.
      \param player_type heterogeneous player type
     */
    void simulateWait( const PlayerType & player_type );

    /*!
      \brief simulate stamina variables after nr waits.
      \param player_type heterogeneous player type
      \param n_wait number of wait cycles (kick, turn, tackle...)
     */
    void simulateWaits( const PlayerType & player_type,
                        const int n_wait );

    /*!
      \brief simulate stamina variables after one dash.
      \param player_type heterogeneous player type
      \param dash_power dash power for simulation
     */
    void simulateDash( const PlayerType & player_type,
                       const double & dash_power );

    /*!
      \brief simulate stamina variables after nr dashes.
      \param player_type heterogeneous player type
      \param n_dash number of dash cycles
      \param dash_power dash power for simulation
     */
    void simulateDashes( const PlayerType & player_type,
                         const int n_dash,
                         const double & dash_power );

    /*!
      \brief simulate stamina variables
      \param player_type heterogeneous player type
      \param n_wait number of wait cycles (kick, turn, tackle...)
      \param n_dash number of dash cycles
      \param dash_power dash power for simulation
     */
    void simulate( const PlayerType & player_type,
                   const int n_wait,
                   const int n_dash,
                   const double & dash_power );

    /*!
      \brief get dash power to save recovery
      \param player_type heterogeneous player type
      \param dash_power desired dash power
      \return result dash power
    */
    double getSafetyDashPower( const PlayerType & player_type,
                               const double & dash_power ) const;
};

}

#endif

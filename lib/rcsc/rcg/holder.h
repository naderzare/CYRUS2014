// -*-c++-*-

/*!
  \file holder.h
  \brief rcg data holder Header File.
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

#ifndef RCSC_RCG_HOLDER_H
#define RCSC_RCG_HOLDER_H

#include <rcsc/rcg/types.h>

#include <string>

namespace rcsc {
namespace rcg {

/*!
  \class Holder
  \brief data holder interface class

  The cncrete class that implements this interface should hold
  the whole game log data.
*/
class Holder {
private:

    int M_log_version; //!< version number of rcg format

public:

    Holder()
        : M_log_version( 0 )
      { }

    /*!
      \brief virtual destructor
     */
    virtual
    ~Holder()
      { }

    /*!
      \brief get version number of rcg format
      \return version number
     */
    int logVersion() const
      {
          return M_log_version;
      }

    /*!
      \brief set version number of rcg format
      \return always true
     */
    bool setLogVersion( const int ver )
      {
          M_log_version = ver;
          return true;
      }

    /*!
      \brief add new dispinfo_t (rcg v1, monitor v1)
      \param dinfo dispinfo_t struct data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
     */
    bool addDispInfo( const dispinfo_t & dinfo );

    /*!
      \brief add new dispinfo_t2 (monitor v2)
      \param dinfo2 dispinfo_t2 struct data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    bool addDispInfo2( const dispinfo_t2 & dinfo2 );

    /*!
      \brief (pure virtual) add showinfo_t (rcg v2, monitor v1)
      \param show added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addShowInfo( const showinfo_t & show ) = 0;

    /*!
      \brief (pure virtual) add showinfo_t2 (monitor v2 only)
      \param show added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addShowInfo2( const showinfo_t2 & show ) = 0;

    /*!
      \brief (pure virtual) add short_showinfo_t2 (rcg v3 only)
      \param show2 added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addShortShowInfo2( const short_showinfo_t2 & show2 ) = 0;

    /*!
      \brief (pure virtual) add msginfo_t
      \param board added message type
      \param msg added message
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addMsgInfo( const Int16 board,
                     const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) add drawinfo_t(rcg v1:v2, monitor v1)
      \param draw added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addDrawInfo( const drawinfo_t & draw ) = 0;

    /*!
      \brief (pure virtual) add playmode
      \param pmode added playmode character
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addPlayMode( const char pmode ) = 0;

    /*!
      \brief (pure virtual) add team_t * 2
      \param team_l added left team data
      \param team_r added right team data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addTeamInfo( const team_t & team_l,
                      const team_t & team_r ) = 0;

    /*!
      \brief (pure virtual) add player_type_t
      \param ptinfo added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addPlayerType( const player_type_t & ptinfo ) = 0;

    /*!
      \brief (pure virtual) add server_params_t
      \param sparams added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addServerParam( const server_params_t & sparams ) = 0;

    /*!
      \brief (pure virtual) add player_param_t
      \param pparams added data
      \retval true if successfully added.
      \retval false mode is incorrect, or failed to add.
    */
    virtual
    bool addPlayerParam( const player_params_t & pparams ) = 0;

    //
    // Monitor version 3
    //

    /*!
      \brief add new display information for monitor version 3.
      \param msg received message string
      \return added result
     */
    bool addDisp3( const std::string & msg );


    //
    // Monitor version 4
    //

    /*!
      \brief add new display information for monitor version 4.
      \param msg received message string
      \return added result
     */
    bool addDisp4( const std::string & msg );

    //
    // RCG version 4 & 5
    //

    /*!
      \brief (pure virtual) add the start of show info v4
      \param time game time of handled data
      \param show added data
      \return added result
    */
    virtual
    bool addShow( const int time,
                  const ShowInfoT & show ) = 0;

    /*!
      \brief (pure virtual) add msg info v4
      \param time game time of handled data
      \param board parsed board info
      \param msg parsed message string
      \return added result
    */
    virtual
    bool addMsg( const int time,
                 const int board,
                 const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) add playmode info v4
      \param time game time of handled data
      \param pm parsed playmode type
      \return added result
    */
    virtual
    bool addPlayMode( const int time,
                      const PlayMode pm ) = 0;

    /*!
      \brief (pure virtual0 add team info v4
      \param time game time of handled data
      \param team_l parsed left team data
      \param team_r parsed right team data
      \return added result
    */
    virtual
    bool addTeam( const int time,
                  const TeamT & team_l,
                  const TeamT & team_r ) = 0;

    /*!
      \brief (pure virtual) add server_param
      \param msg raw message string
      \return added result
    */
    virtual
    bool addServerParam( const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) add player_param
      \param msg raw message string
      \return added result
    */
    virtual
    bool addPlayerParam( const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) add player_type
      \param msg raw message string
      \return added result
    */
    virtual
    bool addPlayerType( const std::string & msg ) = 0;


};

} // end namespace
} // end namespace

#endif

// -*-c++-*-

/*!
  \file handler.h
  \brief rcg data handler Header File.
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

#ifndef RCSC_RCG_HANDLER_H
#define RCSC_RCG_HANDLER_H

#include <rcsc/rcg/types.h>

#include <string>

namespace rcsc {
namespace rcg {

/*!
  \class Handler
  \brief abstract rcg data handler class.

  This is an interface class, all member methods are virtual.
*/
class Handler {
private:
    //! RCG version number(1-3, default:0)
    int M_log_version;

protected:
    /*!
      \brief default constructor. log version is set to zero
    */
    Handler()
        : M_log_version( 0 )
      { }

public:
    /*!
      \brief virtual destructor
    */
    virtual
    ~Handler()
      { }

    /*!
      \brief records rcg version
      \param ver log version.
      \return always true.

      This method is virtual.
      You can override this.
    */
    virtual
    bool handleLogVersion( const int ver )
      {
          M_log_version = ver;
          return true;
      }

    /*!
      \brief returns rcg version number
      \return rcg version number

      This method is virtual.
      You can override this.
    */
    virtual
    int logVersion() const
      {
          return M_log_version;
      }

    /*!
      \brief (pure virtual) handle dispinfo_t
      \param info handled data
      \return handled result
    */
    virtual
    bool handleDispInfo( const dispinfo_t & info ) = 0;

    /*!
      \brief (pure virtual) handle showinfo_t
      \param info handled data
      \return handled result
    */
    virtual
    bool handleShowInfo( const showinfo_t & info ) = 0;

    /*!
      \brief (pure virtual) handle short_showinfo_t
      \param info handled data
      \return handled result
    */
    virtual
    bool handleShortShowInfo2( const short_showinfo_t2 & info ) = 0;

    //! virtual method
    /*!
      \brief (pure virtual) handle msginfo_t
      \param board handled message type
      \param msg handled message data
      \return handled result
    */
    virtual
    bool handleMsgInfo( Int16 board,
                        const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) handle playmode
      \param playmode handled playmode character
      \return handled result
    */
    virtual
    bool handlePlayMode( char playmode ) = 0;

    /*!
      \brief (pure virtual) handle team data
      \param team_left left team data
      \param team_right right team data
      \return handled result
    */
    virtual
    bool handleTeamInfo( const team_t & team_left,
                         const team_t & team_right ) = 0;

    /*!
      \brief (pure virtual) handle player_type_t
      \param type handled data
      \return handled result
    */
    virtual
    bool handlePlayerType( const player_type_t & type ) = 0;

    /*!
      \brief (pure virtual) handle server_params_t
      \param param handled data
      \return handled result
    */
    virtual
    bool handleServerParam( const server_params_t & param ) = 0;

    /*!
      \brief (pure virtual) handle player_params_t
      \param param handled data
      \return handled result
    */
    virtual
    bool handlePlayerParam( const player_params_t & param ) = 0;

    /*!
      \brief (pure virtual) handle end of file
      \return handled result
    */
    virtual
    bool handleEOF() = 0;


    //
    // version 4
    //

    /*!
      \brief (pure virtual) handle the start of show info v4
      \param time game time of handled show info
      \param show read data
      \return handled result
     */
    virtual
    bool handleShow( const int time,
                     const ShowInfoT & show ) = 0;

    /*!
      \brief (pure virtual) handle msg info
      \param time game time of handled msg info
      \param board message board type
      \param msg read data
      \return handled result
     */
    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg ) = 0;

    /*!
      \brief handle playmode
      \param time game time of handled playmode info
      \param pm playmode id
      \return handled result
     */
    virtual
    bool handlePlayMode( const int time,
                         const PlayMode pm ) = 0;

    /*!
      \brief handle team info
      \param time game time of handled team info
      \param team_l left team info
      \param team_r right team info
      \return handled result
    */
    virtual
    bool handleTeam( const int time,
                     const TeamT & team_l,
                     const TeamT & team_r ) = 0;

    /*!
      \brief handle server_param message
      \param msg raw message string
      \return handled result
    */
    virtual
    bool handleServerParam( const std::string & msg ) = 0;

    /*!
      \brief handle player_param message
      \param msg raw message string
      \return handled result
    */
    virtual
    bool handlePlayerParam( const std::string & msg ) = 0;

    /*!
      \brief handle player_type message
      \param msg raw message string
      \return handled result
    */
    virtual
    bool handlePlayerType( const std::string & msg ) = 0;

};


} // end of namespace
} // end of namespace

#endif

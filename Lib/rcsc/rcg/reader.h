// -*-c++-*-

/*!
  \file reader.h
  \brief rcg data reader Header File.
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

#ifndef RCSC_RCG_READER_H
#define RCSC_RCG_READER_H

#include <rcsc/rcg/holder.h>
#include <rcsc/rcg/handler.h>
#include <rcsc/rcg/types.h>

#include <string>

namespace rcsc {
namespace rcg {

/*!
  \class Reader
  \brief rcg data reader interface class.
*/
class Reader
    : public Handler {
private:

    //! reference to the repository instance.
    Holder & M_holder;

    //! default constructor must not be used.
    Reader();
    //! copy constructor must not be used.
    Reader( const Reader & );
    //! substitution operator must not be used.
    Reader & operator=( const Reader & );

public:
    /*!
      \brief construct with holder object
      \param holder reference to the holder instance.

      It is assumed that only this is used to construct this class.
     */
    explicit
    Reader( Holder & holder )
        : M_holder( holder )
      { }

    /*!
      \brief virtual destructor
    */
    virtual
    ~Reader()
      { }

    virtual
    bool handleLogVersion( const int ver )
      {
          return M_holder.setLogVersion( ver );
      }

    /*!
      \brief add distpinfo_t to the holder.
      \param info const reference of dispinfo_t struct.
      \return registry status.

      This method is used for rcg v1 format.
    */
    virtual
    bool handleDispInfo( const dispinfo_t & info )
      {
          return M_holder.addDispInfo( info );
      }

    /*!
      \brief adds showinfo_t to the holder.
      \param info const reference of showinfo_t struct.
      \return registry status.

      This method is used for rcg v2 format.
    */
    virtual
    bool handleShowInfo( const showinfo_t & info )
      {
          return M_holder.addShowInfo( info );
      }

    /*!
      \brief adds short_showinfo_t2 to the holder.
      \param info const reference of short_showinfo_t struct.
      \return registry status.

      This method is used for rcg v3 format.
    */
    virtual
    bool handleShortShowInfo2( const short_showinfo_t2 & info )
      {
          return M_holder.addShortShowInfo2( info );
      }

    /*!
      \brief adds msginfo_t to the holder.
      \param board board parametor of msginfo_t
      \param msg converted std::string message.
      \return registry status.
    */
    virtual
    bool handleMsgInfo( Int16 board,
                        const std::string & msg )
      {
          return M_holder.addMsgInfo( board, msg );
      }

    /*!
      \brief adds latest playmode to the holder.
      \param playmode type id(char) of playmode.
      \return registry status.
    */
    virtual
    bool handlePlayMode( char playmode )
      {
          return M_holder.addPlayMode( playmode );
      }

    /*!
      \brief adds two team info to the holder.
      \param team_left team_t for the left team
      \param team_right team_t for the right team
      \return registry status.
    */
    virtual
    bool handleTeamInfo( const team_t & team_left,
                         const team_t & team_right )
      {
          return M_holder.addTeamInfo( team_left, team_right );
      }

    /*!
      \brief adds player_type_t to the holder.
      \param type player_type_t info
      \return registry status.
    */
    virtual
    bool handlePlayerType( const player_type_t & type )
      {
          return M_holder.addPlayerType( type );
      }

    /*!
      \brief adds server_params_t to the holder.
      \param param server_params_t info
      \return registry status.
    */
    virtual
    bool handleServerParam( const server_params_t & param )
      {
          return M_holder.addServerParam( param );
      }

    /*!
      \brief adds player_params_t to the holder.
      \param param player_params_t info
      \return registry status.
    */
    virtual
    bool handlePlayerParam( const player_params_t & param )
      {
          return M_holder.addPlayerParam( param );
      }

    /*!
      \brief called when stream reaches the end of file.
      \return always true.

      You can override this in the derived classes.
    */
    virtual
    bool handleEOF()
      {
          return true;
      }


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
                     const ShowInfoT & show )
      {
          return M_holder.addShow( time, show );
      }

    /*!
      \brief handle msg info
      \param time game time of handled msg info
      \param board message board type
      \param msg read data
      \return handled result
     */
    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg )
      {
          return M_holder.addMsg( time, board, msg );
      }

    /*!
      \brief handle playmode
      \param time game time of handled playmode info
      \param pm playmode id
      \return handled result
     */
    virtual
    bool handlePlayMode( const int time,
                         const PlayMode pm )
      {
          return M_holder.addPlayMode( time, pm );
      }

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
                     const TeamT & team_r )
      {
          return M_holder.addTeam( time, team_l, team_r );
      }

    /*!
      \brief handle server_param message
      \param msg raw message string
      \return handled result
     */
    virtual
    bool handleServerParam( const std::string & msg )
      {
          return M_holder.addServerParam( msg );
      }

    /*!
      \brief handle player_param message
      \param msg raw message string
      \return handled result
     */
    virtual
    bool handlePlayerParam( const std::string & msg )
      {
          return M_holder.addPlayerParam( msg );
      }

    /*!
      \brief handle player_type message
      \param msg raw message string
      \return handled result
     */
    virtual
    bool handlePlayerType( const std::string & msg )
      {
          return M_holder.addPlayerType( msg );
      }

};

} // end of namespace
} // end of namespace

#endif

// -*-c++-*-

/*!
  \file serializer.h
  \brief rcg serializer class Header File.
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

#ifndef RCSC_RCG_SERIALIZER_H
#define RCSC_RCG_SERIALIZER_H

#include <rcsc/rcg/types.h>
#include <rcsc/factory.h>
#include <rcsc/types.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <ostream>

namespace rcsc {
namespace rcg {

/*!
  \class Serializer
  \brief rcg data serializer interface class
*/
class Serializer {
public:

    typedef boost::shared_ptr< Serializer > Ptr; //!< rcg serializer pointer type
    typedef Ptr (*Creator)(); //!< rcg serializer creator function
    typedef rcss::Factory< Creator, int > Creators; //!< creator function holder

    /*!
      \brief factory holder singleton
      \return reference to the factory holder instance
     */
    static
    Creators & creators();


    /*!
      \brief create a suitable version serializer instance depending on the input version.
      \param version rcg format version.
      \return smart pointer to the rcg serializer instance
     */
    static
    Ptr create( const int version );

protected:

    char M_playmode; //!< temporal playmode holder
    TeamT M_teams[2]; //!< temporal team info holder

protected:

    /*!
      \brief initialize member variables.
      constructor is accessible only from the derived classes.
     */
    Serializer();

public:
    /*!
      \brief virtual destructor
    */
    virtual
    ~Serializer()
      { }

protected:

    /////////////////////////////////////////////////////////////
    // implementations

    /*!
      \brief write header
      \param os reference to the output stream
      \param version log version
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const int version );

    /*!
      \brief write server param
      \param os reference to the output stream
      \param param server_params_t data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const server_params_t & param );

    /*!
      \brief write player param
      \param os reference to the output stream
      \param pparam plyaer_params_t data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const player_params_t & pparam );

    /*!
      \brief write player type param
      \param os reference to the output stream
      \param type player_type_t data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const player_type_t & type );

    /*!
      \brief write team info
      \param os reference to the output stream
      \param team_l left team data
      \param team_r right team data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const team_t & team_l,
                                  const team_t & team_r );
    /*!
      \brief write playmode
      \param os reference to the output stream
      \param pmode playmode index
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const char pmode );

    /*!
      \brief write playmode
      \param os reference to the output stream
      \param pmode playmode index
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const PlayMode pmode );

    /*!
      \brief write dispinfo (version 1 protocol)
      \param os reference to the output stream
      \param disp serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const dispinfo_t & disp );

    /*!
      \brief write showinfo (version 2 protocol)
      \param os reference to the output stream
      \param show serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const showinfo_t & show );

    /*!
      \brief write showinfo_t2 (version 3 protocol).
      data is converted to short_showinfo_t2.
      \param os reference to the output stream
      \param show2 serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const showinfo_t2 & show2 );

    /*!
      \brief write short_showinfo (version 3 protocol)
      \param os reference to the output stream
      \param show2 serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const short_showinfo_t2 & show2 );

    /*!
      \brief write message info
      \param os reference to the output stream
      \param msg serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const msginfo_t & msg );

    /*!
      \brief write draw info
      \param os reference to the output stream
      \param draw serialized data
      \return reference to the output stream
    */
    std::ostream & serializeImpl( std::ostream & os,
                                  const drawinfo_t & draw );

    /*!
      \brief write dispinfo_t2, but data is converted.
      \param os reference to the output stream
      \param disp2 serialized data
      \return reference to the output stream
     */
    std::ostream & serializeImpl( std::ostream & os,
                                  const dispinfo_t2 & disp2 );



public:
    /////////////////////////////////////////////////////////////
    // utility

    /*!
      \brief convert pos_t to BallT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const pos_t & from,
                  BallT & to );

    /*!
      \brief convert ball_t to BallT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const ball_t & from,
                  BallT & to );

    /*!
      \brief convert pos_t to player_t
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const pos_t & from,
                  player_t & to );

    /*!
      \brief convert player_t to pos_t
      \param side player's side id
      \param unum uniform number
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const SideID side,
                  const int unum,
                  const player_t & from,
                  pos_t & to );

    /*!
      \brief convert pos_t to PlayerT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const pos_t & from,
                  PlayerT & to );

    /*!
      \brief convert player info to player_t
      \param from source player info
      \param to destination player_t variable
     */
    static
    void convert( const PlayerT & from,
                  player_t & to );

    /*!
      \brief convert player_t to PlayerT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const player_t & from,
                  PlayerT & to );

    /*!
      \brief convert team info to team_t
      \param name source team name string
      \param score source team score
      \param to destination team_t variable
     */
    static
    void convert( const std::string & name,
                  const int score,
                  team_t & to );

    /*!
      \brief convert team_t to TeamT
      \param from source data
      \param to destination team_t variable
     */
    static
    void convert( const TeamT & from,
                  team_t & to );

    /*!
      \brief convert TeamT to team_t
      \param from source data
      \param to destination TeamT variable
     */
    static
    void convert( const team_t & from,
                  TeamT & to );

    /*!
      \brief convert showinfo_t to showinfo_t2
      \param from source showinfo_t variable
      \param to destination showinfo_t2 variable
     */
    static
    void convert( const showinfo_t & from,
                  showinfo_t2 & to );

    /*!
      \brief convert showinfo_t to short_showinfo_t2
      \param from source showinfo_t variable
      \param to destination short_showinfo_t2 variable
     */
    static
    void convert( const showinfo_t & from,
                  short_showinfo_t2 & to );

    /*!
      \brief convert showinfo_t2 to showinfo_t
      \param from source showinfo_t2 variable
      \param to destination showinfo_t variable
     */
    static
    void convert( const showinfo_t2 & from,
                  showinfo_t & to );

    /*!
      \brief convert short_showinfo_t2 to showinfo_t
      \param playmode playmode variable
      \param team_l left team variable
      \param team_r right team variable
      \param from source short_showinfo_t2 variable
      \param to destination showinfo_t variable
     */
    static
    void convert( const char playmode,
                  const TeamT & team_l,
                  const TeamT & team_r,
                  const short_showinfo_t2 & from,
                  showinfo_t & to );

    /*!
      \brief convert ShowInfoT to showinfo_t
      \param playmode playmode variable
      \param team_l left team variable
      \param team_r right team variable
      \param from source ShowInfoT variable
      \param to destination showinfo_t variable
     */
    static
    void convert( const char playmode,
                  const TeamT & team_l,
                  const TeamT & team_r,
                  const ShowInfoT & from,
                  showinfo_t & to );

    /*!
      \brief convert showinfot_t to ShowInfoT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const showinfo_t & from,
                  ShowInfoT & to );

   /*!
      \brief convert ShowInfoT to showinfo_t2
      \param playmode playmode variable
      \param team_l left team variable
      \param team_r right team variable
      \param from source ShowInfoT variable
      \param to destination showinfo_t2 variable
     */
    static
    void convert( const char playmode,
                  const TeamT & team_l,
                  const TeamT & team_r,
                  const ShowInfoT & from,
                  showinfo_t2 & to );

    /*!
      \brief convert showinfot_t2 to ShowInfoT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const showinfo_t2 & from,
                  ShowInfoT & to );

   /*!
      \brief convert ShowInfoT to short_showinfo_t2
      \param from source ShowInfoT variable
      \param to destination short_showinfo_t2 variable
     */
    static
    void convert( const ShowInfoT & from,
                  short_showinfo_t2 & to );

    /*!
      \brief convert short_showinfot_t2 to ShowInfoT
      \param from source variable
      \param to destination variable
     */
    static
    void convert( const short_showinfo_t2 & from,
                  ShowInfoT & to );

    /*!
      \brief make msginfo_t from string
      \param from source message string
      \param to destination msginfo_t variable
    */
    static
    void convert( const std::string & from,
                  msginfo_t & to );

    /////////////////////////////////////////////////////////////
    // interfaces

    /*!
      \brief write header
      \param os reference to the output stream
      \return reference to the output stream
    */
    virtual
    std::ostream & serializeHeader( std::ostream & os ) = 0;

    /*!
      \brief write parameter message
      \param os reference to the output stream
      \param msg server parameter message
      \return reference to the output stream
    */
    virtual
    std::ostream & serializeParam( std::ostream & os,
                                   const std::string & msg ) = 0;

    /*!
      \brief write header
      \param os reference to the output stream
      \param param server_params_t variable by network byte order
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const server_params_t & param ) = 0;

    /*!
      \brief write header
      \param os reference to the output stream
      \param pparam player_params_t variable by network byte order
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const player_params_t & pparam ) = 0;

    /*!
      \brief write header
      \param os reference to the output stream
      \param type player_type_t variable by network byte order
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const player_type_t & type ) = 0;

    /*!
      \brief write dispinfo_t.
      \param os reference to the output stream
      \param disp network byte order data
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const dispinfo_t & disp ) = 0;

    /*!
      \brief write showinfo_t.
      \param os reference to the output stream
      \param show network byte order data
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const showinfo_t & show ) = 0;

    /*!
      \brief write showinfo_t2.
      \param os reference to the output stream
      \param show2 network byte order data
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const showinfo_t2 & show2 ) = 0;

    /*!
      \brief write short_showinfo_t2.
      \param os reference to the output stream
      \param show2 network byte order data
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const short_showinfo_t2 & show2 ) = 0;

    /*!
      \brief write message info
      \param os reference to the output stream
      \param msg msginfo_t variable by network byte order
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const msginfo_t & msg ) = 0;

    /*!
      \brief write message info
      \param os reference to the output stream
      \param board message board type
      \param msg message string
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const Int16 board,
                              const std::string & msg ) = 0;

    /*!
      \brief write drawinfo_t
      \param os reference to the output stream
      \param draw drawinfo_t variable
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const drawinfo_t & draw ) = 0;

    /*!
      \brief write playmode
      \param os reference to the output stream
      \param playmode play mode variable
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const char playmode ) = 0;

    /*!
      \brief write team info
      \param os reference to the output stream
      \param team_l left team variable
      \param team_r right team variable
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const team_t & team_l,
                              const team_t & team_r ) = 0;

    /*!
      \brief write team info
      \param os reference to the output stream
      \param team_l left team variable
      \param team_r right team variable
      \return reference to the output stream
    */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const TeamT & team_l,
                              const TeamT & team_r ) = 0;

    /*!
      \brief write ShowInfoT
      \param os reference to the output stream
      \param show data to be written
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const ShowInfoT & show ) = 0;

    /*!
      \brief write DispInfoT
      \param os reference to the output stream
      \param disp data to be written
      \return reference to the output stream
     */
    virtual
    std::ostream & serialize( std::ostream & os,
                              const DispInfoT & disp ) = 0;

};

} // end of namespace rcg
} // end of namespace rcsc

#endif

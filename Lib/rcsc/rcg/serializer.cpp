// -*-c++-*-

/*!
  \file serializer.cpp
  \brief rcg serializer Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "serializer.h"

#include "util.h"

#include "serializer_v1.h"
#include "serializer_v2.h"
#include "serializer_v3.h"
#include "serializer_v4.h"
#include "serializer_v5.h"


#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>

#ifndef M_PI
//! pi value
#define M_PI 3.14159265358979323846
#endif

namespace {
const float DEG2RADF = 3.14159265358979323846f / 180.0f;
const float RAD2DEGF = 180.0f / 3.14159265358979323846f;
}

namespace rcsc {
namespace rcg {


/*-------------------------------------------------------------------*/
/*!

*/
Serializer::Creators &
Serializer::creators()
{
    static Creators s_instance;
    return s_instance;
}


/*-------------------------------------------------------------------*/
/*!

*/
Serializer::Ptr
Serializer::create( const int version )
{
    Serializer::Ptr ptr( static_cast< Serializer * >( 0 ) );

    Serializer::Creator creator;
    if ( Serializer::creators().getCreator( creator, version ) )
    {
        ptr = creator();
    }
    else if ( version == REC_VERSION_5 ) ptr = Serializer::Ptr( new SerializerV5() );
    else if ( version == REC_VERSION_4 ) ptr = Serializer::Ptr( new SerializerV4() );
    else if ( version == REC_VERSION_3 ) ptr = Serializer::Ptr( new SerializerV3() );
    else if ( version == REC_VERSION_2 ) ptr = Serializer::Ptr( new SerializerV2() );
    else if ( version == REC_OLD_VERSION ) ptr = Serializer::Ptr( new SerializerV1() );

    return ptr;
}

/*-------------------------------------------------------------------*/
/*!

*/
Serializer::Serializer()
    : M_playmode( static_cast< char >( 0 ) )
{
    for ( int i = 0; i < 2; ++i )
    {
        M_teams[i].clear();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const int version )
{
    if ( version == REC_OLD_VERSION )
    {
        // v1 protocl does not have header.
        return os;
    }

    if ( version >= REC_VERSION_4 )
    {
        os << "ULG" << version << '\n';
    }
    else
    {
        char buf[5];

        buf[0] = 'U';
        buf[1] = 'L';
        buf[2] = 'G';
        buf[3] = static_cast< char >( version );

        os.write( buf, 4 );
    }

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const server_params_t & param )
{
    Int16 mode = htons( PARAM_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &param ),
              sizeof( server_params_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const player_params_t & pparam )
{
    Int16 mode = htons( PPARAM_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &pparam ),
              sizeof( player_params_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const player_type_t & type )
{
    Int16 mode = htons( PT_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &type ),
              sizeof( player_type_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const team_t & team_l,
                           const team_t & team_r )
{
    Int16 mode = htons( TEAM_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &team_l ),
              sizeof( team_t ) );

    os.write( reinterpret_cast< const char * >( &team_r ),
              sizeof( team_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const char pmode )
{
    Int16 mode = htons( PM_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &pmode ),
              sizeof( char ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const PlayMode pmode )
{
    Int16 mode = htons( PM_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    char pm = static_cast< char >( pmode );

    os.write( reinterpret_cast< char * >( &pm ),
              sizeof( char ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const dispinfo_t & disp )
{
    os.write( reinterpret_cast< const char * >( &disp ),
              sizeof( dispinfo_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const showinfo_t & show )
{
    Int16 mode = htons( SHOW_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &show ),
              sizeof( showinfo_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const showinfo_t2 & show2 )
{
    if ( M_playmode != show2.pmode )
    {
        M_playmode = show2.pmode;

        serializeImpl( os, show2.pmode );
    }

    if ( M_teams[0].name_.length() != std::strlen( show2.team[0].name )
         || M_teams[0].name_ != show2.team[0].name
         || M_teams[0].score_ != ntohs( show2.team[0].score )
         || M_teams[1].name_.length() != std::strlen( show2.team[1].name )
         || M_teams[1].name_ != show2.team[1].name
         || M_teams[1].score_ != ntohs( show2.team[1].score ) )
    {
        convert( show2.team[0], M_teams[0] );
        convert( show2.team[1], M_teams[1] );

        serializeImpl( os, show2.team[0], show2.team[1] );
    }

    short_showinfo_t2 short_show2;

    short_show2.ball = show2.ball;

    for ( int i = 0; i < MAX_PLAYER * 2; ++i )
    {
        short_show2.pos[i] = show2.pos[i];
    }

    short_show2.time = show2.time;

    serializeImpl( os, short_show2 );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const short_showinfo_t2 & show2 )
{
    Int16 mode = htons( SHOW_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &show2 ),
              sizeof( short_showinfo_t2 ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const msginfo_t & msg )
{
    Int16 mode = htons( MSG_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &msg.board ),
              sizeof( rcsc::rcg::Int16 ) );

    rcsc::rcg::Int16 len = 1;
    while ( msg.message[len-1] != '\0'
            && len < 2048 )
    {
        ++len;
    }

    rcsc::rcg::Int16 nlen = htons( len );

    os.write( reinterpret_cast< const char* >( &nlen ),
              sizeof( rcsc::rcg::Int16 ) );

    os.write( reinterpret_cast< const char * >( msg.message ),
              len );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const drawinfo_t & draw )
{
    Int16 mode = htons( DRAW_MODE );

    os.write( reinterpret_cast< char * >( &mode ),
              sizeof( Int16 ) );

    os.write( reinterpret_cast< const char * >( &draw ),
              sizeof( drawinfo_t ) );

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
Serializer::serializeImpl( std::ostream & os,
                           const dispinfo_t2 & disp2 )
{
    switch ( ntohs( disp2.mode ) ) {
    case SHOW_MODE:
        serializeImpl( os, disp2.body.show );
        break;
    case MSG_MODE:
        serializeImpl( os, disp2.body.msg );
        break;
    case DRAW_MODE:
        //serializeImpl( os, disp2.body.draw );
        break;
    case BLANK_MODE:
        break;
    case PM_MODE:
        serializeImpl( os, disp2.body.show.pmode );
        break;
    case TEAM_MODE:
        serializeImpl( os,
                       disp2.body.show.team[0],
                       disp2.body.show.team[1] );
        break;
    case PT_MODE:
        serializeImpl( os, disp2.body.ptinfo );
        break;
    case PARAM_MODE:
        serializeImpl( os, disp2.body.sparams );
        break;
    case PPARAM_MODE:
        serializeImpl( os, disp2.body.pparams );
        break;
    default:
        break;
    }

    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const pos_t & from,
                     BallT & to )
{
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const ball_t & from,
                     BallT & to )
{
    to.x_ = nltohf( from.x );
    to.y_ = nltohf( from.y );
    to.vx_ = nltohf( from.deltax );
    to.vy_ = nltohf( from.deltay );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const pos_t & from,
                     player_t & to )
{
    to.mode = from.enable;
    to.type = 0;
    to.x = nstonl( from.x );
    to.y = nstonl( from.y );
    to.deltax = 0;
    to.deltay = 0;
    to.body_angle = nstonl( from.angle );
    to.head_angle = 0;
    to.view_width = 0;
    to.stamina = hdtonl( 4000.0 );
    to.effort = hdtonl( 1.0 );
    to.recovery = hdtonl( 1.0 );
    to.kick_count = 0;
    to.dash_count = 0;
    to.turn_count = 0;
    to.say_count = 0;
    to.turn_neck_count = 0;
    to.catch_count = 0;
    to.move_count = 0;
    to.change_view_count = 0;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const SideID side,
                     const int unum,
                     const player_t & from,
                     pos_t & to )
{
    to.enable = from.mode;
    to.side = htons( static_cast< Int16 >( side ) );
    to.unum = htons( static_cast< Int16 >( unum ) );
    to.angle = htons( static_cast< Int16 >( rint( nltohd( from.body_angle ) * 180.0 / M_PI ) ) );
    to.x = hdtons( nltohd( from.x ) );
    to.y = hdtons( nltohd( from.y ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const pos_t & from,
                     PlayerT & to )
{
    SideID side = static_cast< SideID >( ntohs( from.side ) );

    to.state_ = static_cast< Int32 >( ntohs( from.enable ) );
    to.side_ = ( side == LEFT ? 'l'
                 : side == RIGHT ? 'r'
                 : 'n' );
    to.unum_ = ntohs( from.unum );
    to.body_ = static_cast< float >( ntohs( from.angle ) );
    to.x_ = nstohf( from.x );
    to.y_ = nstohf( from.y );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const PlayerT & from,
                     player_t & to )
{
    to.mode = ntohs( static_cast< Int16 >( from.state_ ) );
    to.type = ntohs( from.type_ );
    to.x = hftonl( from.x_ );
    to.y = hftonl( from.y_ );
    to.deltax = hftonl( from.vx_ );
    to.deltay = hftonl( from.vy_ );
    to.body_angle = hftonl( from.body_ * DEG2RADF );
    to.head_angle = hftonl( from.neck_ * DEG2RADF );
    to.view_width = hftonl( from.view_width_ * DEG2RADF );
    to.view_quality = htons( from.highQuality() ? 1 : 0 );
    to.stamina = hftonl( from.stamina_ );
    to.effort = hftonl( from.effort_ );
    to.recovery = hftonl( from.recovery_ );
    to.kick_count = htons( from.kick_count_ );
    to.dash_count = htons( from.dash_count_ );
    to.turn_count = htons( from.turn_count_ );
    to.say_count = htons( from.say_count_ );
    to.turn_neck_count = htons( from.turn_neck_count_ );
    to.catch_count = htons( from.catch_count_ );
    to.move_count = htons( from.move_count_ );
    to.change_view_count = htons( from.change_view_count_ );
}


/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const player_t & from,
                     PlayerT & to )
{
    to.state_ = static_cast< Int32 >( ntohs( from.mode ) );
    to.type_ = ntohs( from.type );
    to.x_ = nltohf( from.x );
    to.y_ = nltohf( from.y );
    to.vx_ = nltohf( from.deltax );
    to.vy_ = nltohf( from.deltay );
    to.body_ = nltohf( from.body_angle ) * RAD2DEGF;
    to.neck_ = nltohf( from.head_angle ) * RAD2DEGF;
    to.view_width_ = nltohf( from.view_width ) * RAD2DEGF;
    to.view_quality_ = ntohs( from.view_quality ) ? 'h' : 'l';
    to.stamina_ = nltohf( from.stamina );
    to.effort_ = nltohf( from.effort );
    to.recovery_ = nltohf( from.recovery );
    to.kick_count_ = ntohs( from.kick_count );
    to.dash_count_ = ntohs( from.dash_count );
    to.turn_count_ = ntohs( from.turn_count );
    to.say_count_ = ntohs( from.say_count );
    to.turn_neck_count_ = ntohs( from.turn_neck_count );
    to.catch_count_ = ntohs( from.catch_count );
    to.move_count_ = ntohs( from.move_count );
    to.change_view_count_ = ntohs( from.change_view_count );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const std::string & name,
                     const int score,
                     team_t & to )
{
    std::memset( to.name, 0, sizeof( to.name ) );
    std::strncpy( to.name,
                  name.c_str(),
                  std::min( sizeof( to.name ), name.length() ) );
    to.score = hitons( score );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const TeamT & from,
                     team_t & to )
{
    std::memset( to.name, 0, sizeof( to.name ) );
    std::strncpy( to.name,
                  from.name_.c_str(),
                  std::min( sizeof( to.name ), from.name_.length() ) );
    to.score = htons( from.score_ );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const team_t & from,
                     TeamT & to )
{
    char buf[18];
    std::memset( buf, 0, 18 );
    std::strncpy( buf, from.name, sizeof( from.name ) );

    to.name_ = buf;
    to.score_ = ntohs( from.score );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const showinfo_t & from,
                     showinfo_t2 & to )
{
    // playmode
    to.pmode = from.pmode;

    // team

    // ball
    to.ball.x = nstonl( from.pos[0].x );
    to.ball.y = nstonl( from.pos[0].y );
    to.ball.deltax = 0;
    to.ball.deltay = 0;

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.pos[i] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const showinfo_t & from,
                     short_showinfo_t2 & to )
{
    // ball
    to.ball.x = nstonl( from.pos[0].x );
    to.ball.y = nstonl( from.pos[0].y );
    to.ball.deltax = 0;
    to.ball.deltay = 0;

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.pos[i] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const showinfo_t2 & from,
                     showinfo_t & to )
{
    // playmode
    to.pmode = from.pmode;

    // team
    for ( int i = 0; i < 2; ++i )
    {
        std::strncpy( to.team[i].name, from.team[i].name, 16 );
        to.team[i].score = from.team[i].score;
    }

    // ball
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hdtons( nltohd( from.ball.x ) );
    to.pos[0].y = hdtons( nltohd( from.ball.y ) );

    // left team
    int unum = 1;
    for ( int i = 0; i < MAX_PLAYER; ++i, ++unum )
    {
        convert( LEFT, unum, from.pos[i], to.pos[i+1] );
    }

    // right team
    unum = 1;
    for ( int i = MAX_PLAYER; i < MAX_PLAYER *2; ++i, ++unum )
    {
        convert( RIGHT, unum, from.pos[i], to.pos[i+1] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const char playmode,
                     const TeamT & team_l,
                     const TeamT & team_r,
                     const short_showinfo_t2 & from,
                     showinfo_t & to )
{
    // playmode
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hdtons( nltohd( from.ball.x ) );
    to.pos[0].y = hdtons( nltohd( from.ball.y ) );

    // left team
    int unum = 1;
    for ( int i = 0; i < MAX_PLAYER; ++i, ++unum )
    {
        convert( LEFT, unum, from.pos[i], to.pos[i+1] );
    }

    // right team
    unum = 1;
    for ( int i = MAX_PLAYER; i < MAX_PLAYER *2; ++i, ++unum )
    {
        convert( RIGHT, unum, from.pos[i], to.pos[i+1] );
    }

    // time
    to.time = from.time;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const char playmode,
                     const TeamT & team_l,
                     const TeamT & team_r,
                     const ShowInfoT & from,
                     showinfo_t & to )
{
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    to.pos[0].side = htons( NEUTRAL );
    to.pos[0].x = hftons( from.ball_.x_ );
    to.pos[0].y = hftons( from.ball_.y_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        int idx = from.player_[i].unum_;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 + 1 <= idx ) continue;

        to.pos[idx].enable = htons( static_cast< Int16 >( from.player_[i].state_ ) );
        to.pos[idx].side = htons( from.player_[i].side() );
        to.pos[idx].unum = htons( from.player_[i].unum_ );
        to.pos[idx].angle = htons( static_cast< Int16 >( rintf( from.player_[i].body_ ) ) );
        to.pos[idx].x = hftons( from.player_[i].x_ );
        to.pos[idx].y = hftons( from.player_[i].y_ );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const char playmode,
                     const TeamT & team_l,
                     const TeamT & team_r,
                     const ShowInfoT & from,
                     showinfo_t2 & to )
{
    to.pmode = playmode;

    // team
    convert( team_l, to.team[0] );
    convert( team_r, to.team[1] );

    // ball
    to.ball.x = hftonl( from.ball_.x_ );
    to.ball.y = hftonl( from.ball_.y_ );
    to.ball.deltax = hftonl( from.ball_.vx_ );
    to.ball.deltay = hftonl( from.ball_.vx_ );

    // players
    for ( int i = 0; i < MAX_PLAYER * 2; ++i )
    {
        int idx = from.player_[i].unum_ - 1;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 <= idx ) continue;

        convert( from.player_[i], to.pos[idx] );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const ShowInfoT & from,
                     short_showinfo_t2 & to )
{
    // ball
    to.ball.x = hftonl( from.ball_.x_ );
    to.ball.y = hftonl( from.ball_.y_ );
    to.ball.deltax = hftonl( from.ball_.vx_ );
    to.ball.deltay = hftonl( from.ball_.vx_ );

    // players
    for ( int i = 0; i < MAX_PLAYER * 2; ++i )
    {
        int idx = from.player_[i].unum_ - 1;
        if ( from.player_[i].side() == NEUTRAL ) continue;
        if ( from.player_[i].side() == RIGHT ) idx += MAX_PLAYER;
        if ( idx < 0 || MAX_PLAYER*2 <= idx ) continue;

        convert( from.player_[i], to.pos[idx] );
    }

    // time
    to.time = htons( static_cast< Int16 >( from.time_ ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const showinfo_t & from,
                     ShowInfoT & to )

{
    // ball
    convert( from.pos[0], to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        convert( from.pos[i+1], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const showinfo_t2 & from,
                     ShowInfoT & to )

{
    // ball
    convert( from.ball, to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        to.player_[i].side_ = ( i < MAX_PLAYER ? 'l' : 'r' );
        to.player_[i].unum_ = ( i < MAX_PLAYER ? i + 1 : i + 1 - MAX_PLAYER );
        convert( from.pos[i], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const short_showinfo_t2 & from,
                     ShowInfoT & to )

{
    // ball
    convert( from.ball, to.ball_ );

    // players
    for ( int i = 0; i < MAX_PLAYER*2; ++i )
    {
        to.player_[i].side_ = ( i < MAX_PLAYER ? 'l' : 'r' );
        to.player_[i].unum_ = ( i < MAX_PLAYER ? i + 1 : i + 1 - MAX_PLAYER );
        convert( from.pos[i], to.player_[i] );
    }

    // time
    to.time_ = static_cast< UInt32 >( ntohs( from.time ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Serializer::convert( const std::string & from,
                     msginfo_t & to )
{
    to.board = htons( MSG_BOARD );
    std::memset( to.message, 0, sizeof( to.message ) );
    std::strncpy( to.message,
                  from.c_str(),
                  std::min( sizeof( to.message ) - 1, from.length() ) );
}

} // end of namespace
} // end of namespace

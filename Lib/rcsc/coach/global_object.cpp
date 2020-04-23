// -*-c++-*-

/*!
  \file global_object.cpp
  \brief The declaration of object types held by coach/trainer.
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

#include "global_object.h"

#include <rcsc/common/server_param.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
GlobalPlayerObject::GlobalPlayerObject()
    : M_side( NEUTRAL ),
      M_unum( -1 ),
      M_goalie( false ),
      M_type( Hetero_Unknown ),
      M_player_type( static_cast< PlayerType * >( 0 ) ),
      M_pos( Vector2D::INVALIDATED ),
      M_vel( 0.0, 0.0 ),
      M_body( 0.0 ),
      M_face( 0.0 ),
      M_recovery( ServerParam::i().recoverInit() ),
      M_pointto_cycle( 0 ),
      M_pointto_angle( 0.0 ),
      M_kicked( false ),
      M_tackle_cycle( 0 ),
      M_charged_cycle( 0 ),
      M_card( NO_CARD )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
GlobalPlayerObject::update( const GlobalPlayerObject & p )
{
    M_side = p.side();
    M_unum = p.unum();
    M_goalie = p.goalie();

    M_pos = p.pos();
    M_vel = p.vel();

    M_body = p.body();
    M_face = p.face();

    if ( p.isPointing() )
    {
        ++M_pointto_cycle;
        M_pointto_angle = p.pointtoAngle();
    }
    else
    {
        M_pointto_cycle = 0;
    }

    M_kicked = p.kicked();

    if ( p.isTackling() )
    {
        ++M_tackle_cycle;
    }
    else
    {
        M_tackle_cycle = 0;
    }

    if ( p.isCharged() )
    {
        ++M_charged_cycle;
    }
    else
    {
        M_charged_cycle = 0;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
GlobalPlayerObject::setPlayerType( const int type )
{
    M_type = type;
    M_player_type = PlayerTypeSet::i().get( type );
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
GlobalPlayerObject::print( std::ostream & os ) const
{
    os << "Player (" << ( M_side == LEFT ? "l " : "r " )
       << M_unum << ( M_goalie ? " g) " : ") " )
       << pos() << ' ' << vel() << ' '
       << body() << ' ' << face();

    if ( isPointing() )
    {
        os << " arm:cycle=" << pointtoCycle()
           << "dir=" << pointtoAngle();
    }

    if ( kicked() )
    {
        os << " kicked";
    }

    if ( isTackling() )
    {
        os << " tackle=" << tackleCycle();
    }

    return os;
}

} // namespace rcsc

// -*-c++-*-

/*!
  \file abstract_player_object.cpp
  \brief abstract player object class Source File
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

#include "abstract_player_object.h"
#include "world_model.h"

#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/player/player_evaluator.h>

#include <limits>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
AbstractPlayerObject::AbstractPlayerObject()
    : M_side( NEUTRAL )
    , M_unum( Unum_Unknown )
    , M_unum_count( 1000 )
    , M_goalie( false )
    , M_type( Hetero_Unknown )
    , M_player_type( static_cast< const PlayerType * >( 0 ) )
    , M_card( NO_CARD )
    , M_pos( Vector2D::INVALIDATED )
    , M_pos_count( 1000 )
    , M_seen_pos( Vector2D::INVALIDATED )
    , M_seen_pos_count( 1000 )
    , M_heard_pos( Vector2D::INVALIDATED )
    , M_heard_pos_count( 1000 )
    , M_vel( 0.0, 0.0 )
    , M_vel_count( 1000 )
    , M_seen_vel( 0.0, 0.0 )
    , M_seen_vel_count( 1000 )
    , M_body( 0.0 )
    , M_body_count( 1000 )
    , M_face( 0.0 )
    , M_face_count( 1000 )
    , M_kicked( false )
    , M_dist_from_ball( 1000.0 )
    , M_angle_from_ball( 0.0 )
    , M_dist_from_self( 1000.0 )
    , M_angle_from_self( 0.0 )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
AbstractPlayerObject::AbstractPlayerObject( const SideID side,
                                            const Localization::PlayerT & p )
    : M_side( side )
    , M_unum( p.unum_ )
    , M_unum_count( 1000 )
    , M_goalie( p.goalie_ )
    , M_type( Hetero_Unknown )
    , M_player_type( static_cast< const PlayerType * >( 0 ) )
    , M_pos( p.pos_ )
    , M_pos_count( 0 )
    , M_seen_pos( p.pos_ )
    , M_seen_pos_count( 0 )
    , M_heard_pos( Vector2D::INVALIDATED )
    , M_heard_pos_count( 1000 )
    , M_vel( 0.0, 0.0 )
    , M_vel_count( 1000 )
    , M_seen_vel( 0.0, 0.0 )
    , M_seen_vel_count( 1000 )
    , M_body( 0.0 )
    , M_body_count( 1000 )
    , M_face( 0.0 )
    , M_face_count( 1000 )
    , M_kicked( false )
    , M_dist_from_ball( 1000.0 )
    , M_angle_from_ball( 0.0 )
    , M_dist_from_self( 1000.0 )
    , M_angle_from_self( 0.0 )
{
    if ( p.unum_ != Unum_Unknown )
    {
        M_unum_count = 0;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
AbstractPlayerObject::setPlayerType( const int id )
{
    M_type = id;
    M_player_type = PlayerTypeSet::i().get( id );
}

/*-------------------------------------------------------------------*/
/*!

*/
double
AbstractPlayerObject::kickRate() const
{
//     return kick_rate( this->distFromBall(),
//                       ( this->angleFromBall() + AngleDeg( 180.0 ) - this->body() ).degree(),
//                       this->playerTypePtr()->kickPowerRate(), //ServerParam::i().kickPowerRate(),
//                       ServerParam::i().ballSize(),
//                       this->playerTypePtr()->playerSize(),
//                       this->playerTypePtr()->kickableMargin() );
    return playerTypePtr()->kickRate( distFromBall(),
                                      ( angleFromBall() + AngleDeg( 180.0 ) - body() ).degree() );
}

/*-------------------------------------------------------------------*/
/*!

*/
double
AbstractPlayerObject::get_minimum_evaluation( const AbstractPlayerCont & cont,
                                              const PlayerEvaluator * evaluator )
{
    double min_value = std::numeric_limits< double >::max();

    const AbstractPlayerCont::const_iterator p_end = cont.end();
    for ( AbstractPlayerCont::const_iterator it = cont.begin();
          it != p_end;
          ++it )
    {
        double value = (*evaluator)( **it );

        if ( value < min_value )
        {
            min_value = value;
        }
    }

    delete evaluator;
    return min_value;
}

/*-------------------------------------------------------------------*/
/*!

*/
double
AbstractPlayerObject::get_maximum_evaluation( const AbstractPlayerCont & cont,
                                              const PlayerEvaluator * evaluator )
{
    double max_value = -std::numeric_limits< double >::max();

    const AbstractPlayerCont::const_iterator p_end = cont.end();
    for ( AbstractPlayerCont::const_iterator it = cont.begin();
          it != p_end;
          ++it )
    {
        double value = (*evaluator)( **it );

        if ( value > max_value )
        {
            max_value = value;
        }
    }

    delete evaluator;
    return max_value;
}

}

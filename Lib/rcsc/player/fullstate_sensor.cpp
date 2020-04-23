// -*-c++-*-

/*!
  \file fullstate_sensor.cpp
  \brief fullstate info sensor Source File
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

#include "fullstate_sensor.h"

#include <rcsc/common/logger.h>

#include <algorithm>
#include <iterator>
#include <cstring>

#if 0
/*-------------------------------------------------------------------*/
/*!
  \brief stream operator
  \param os reference to output stream
  \param p printed data
  \return reference to output stream
*/
inline
std::ostream &
operator<<( std::ostream & os,
            const rcsc::FullstateSensor::PlayerT & p )
{
    return p.print( os );
    /*
    os << "FS player: side:" << p.side_
       << " unum:" << p.unum_
       << " goalie:" << p.goalie_
       << " type:" << p.player_type_
       << "\n    pos:" << p.pos_
       << " vel:" << p.vel_
       << " b:" << p.body_
       << " n:" << p.neck_
       << " h:" << rcsc::AngleDeg::normalize_angle( p.body_ + p.neck_ )
       << " s:" << p.stamina_
       << " e:" << p.effort_
       << " r:" << p.recovery_
       << " pdist:" << p.pointto_dist_
       << " pdir:" << p.pointto_dir_;
    return os;
    */
}
#endif

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
FullstateSensor::PlayerT::print( std::ostream & os ) const
{
    os << "FS player: side:" << side_
       << " unum:" << unum_
       << " goalie:" << goalie_
       << " type:" << type_
       << "\n    pos:" << pos_
       << " vel:" << vel_
       << " b:" << body_
       << " n:" << neck_
       << " h:" << AngleDeg::normalize_angle( body_ + neck_ )
       << " s:" << stamina_
       << " e:" << effort_
       << " r:" << recovery_
       << " pdist:" << pointto_dist_
       << " pdir:" << pointto_dir_;
    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FullstateSensor::parse( const char * msg,
                        const double & version,
                        const GameTime & current )
{
    M_time = current;

    M_left_team.clear();
    M_right_team.clear();

    if ( version >= 8.0 )
    {
        parseV8( msg );
    }
    else
    {
        parseV7( msg );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FullstateSensor::reverse()
{
    M_ball.pos_ *= -1.0;
    M_ball.vel_ *= -1.0;

    {
        const PlayerCont::iterator end = M_left_team.end();
        for ( PlayerCont::iterator it = M_left_team.begin();
              it != end;
              ++it )
        {
            it->pos_ *= -1.0;
            it->vel_ *= -1.0;
            it->body_ = AngleDeg::normalize_angle( it->body_ + 180.0 );
        }
    }

    {
        const PlayerCont::iterator end = M_right_team.end();
        for ( PlayerCont::iterator it = M_right_team.begin();
              it != end;
              ++it )
        {
            it->pos_ *= -1.0;
            it->vel_ *= -1.0;
            it->body_ = AngleDeg::normalize_angle( it->body_ + 180.0 );
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
FullstateSensor::parseV8( const char * msg )
{
    /*
      fullstate v8+ format:

      (fullstate <time>
        (pmode {goalie_catch_ball_{l|r}|<play mode>})
        (vmode {high|low} {narrow|normal|high})
        //(stamina <stamina> <effort> <recovery>)
        (count <kicks> <dashes> <turns> <catches> <moves>
               <turn_necks> <change_views> <says>)
        (arm (movable <MOVABLE>) (expires <EXPIRES>)
        (target <DIST> <DIR>) (count <COUNT>))
        (score <team_points> <enemy_points>)
        ((b) <pos.x> <pos.y> <vel.x> <vel.y>)
        <players>)

      // after rcssserver-8.03, self stamina info has been omit.
      // and added "arm" info.

      players : {<player>|<player> <players>}

      player : (v8-13)
       ((p {l|r} <unum> {g|<player_type_id>})
        <pos.x> <pos.y> <vel.x> <vel.y> <body_angle> <neck_angle>[ <point_dist> <point_dir>]
        (<stamina> <effort> <recovery>[ <capacity>]))

      player : (v14)
        ((p {l|r} <unum> [g] <player_type_id>)
         <pos.x> <pos.y> <vel.x> <vel.y> <body_angle> <neck_angle>[ <point_dist> <point_dir>]
         (<stamina> <effort> <recovery>[ <capacity>])
         [t|k] [y|r])

      */

    char * next;

    while ( *msg != '\0' && *msg != ' ' ) ++msg; // skip "(fullstate"
    // play mode
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "(pmode"
    while ( *msg != '\0' && *msg != ')' ) ++msg; // ignore playmode info

    // view mode
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to (vmode
    while ( *msg != '\0' && *msg != ')' ) ++msg;// get_view_mode(msg); // ignore view mode info

    // stamina info or count info
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to next token
    if ( ! std::strncmp( msg, "(stamina", 8 ) )
    {
        while (  *msg != '\0' && *msg != ')' ) ++msg; // ignore stamina info
    }
    // count info
    // (count <kicks> <dashes> <turns> <catches> <moves> <turn_necks> <change_views> <says>)
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "(count"
    while ( *msg != '\0' && *msg != ')' ) ++msg; // ignore count info

    // arm info
    // (arm (movable <MOVABLE>) (expires <EXP>) (target <DIST> <DIR>) (count <CNT>))
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "(arm..."
    while ( *msg != '\0' && *msg != ')' ) ++msg; // skip to movable end
    ++msg;
    while ( *msg != '\0' && *msg != ')' ) ++msg; // skip to expires end
    ++msg;
    while ( *msg != '\0' && *msg != ')' ) ++msg; // skip to target end
    ++msg;
    while ( *msg != '\0' && *msg != ')' ) ++msg; // skip to count end

    // score info
    // (score <team_points> <enemy_points>)
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to (score
    while ( *msg != '\0' && *msg != ' ' ) ++msg; // skip to " LSCORE..."
    M_left_score = static_cast< int >( std::strtol( msg, &next, 10) ); msg = next;
    M_right_score = static_cast< int >( std::strtol( msg, &next, 10 ) ); msg = next;

    // ball info
    // ((b) <pos.x> <pos.y> <vel.x> <vel.y>)
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to (ball
    while ( *msg != '\0' && *msg != ' ' ) ++msg; // skip "(ball"

    M_ball.pos_.x = std::strtod( msg, &next ); msg = next;
    M_ball.pos_.y = std::strtod( msg, &next ); msg = next;
    M_ball.vel_.x = std::strtod( msg, &next ); msg = next;
    M_ball.vel_.y = std::strtod( msg, &next ); msg = next;

    //((p {l|r} <unum>{g|<player_type_id>}) <pos.x> <pos.y>
    //   <vel.x> <vel.y> <body_angle> <neck_angle>[ <point_dist> <point_dir>]
    //   (<stamina> <effort> <recovery>[ <capacity>])
    //   [t|k] [y|r])
    //
    //((p {l|r} <unum> [g] <player_type_id>}) <pos.x> <pos.y>
    //   <vel.x> <vel.y> <body_angle> <neck_angle>[ <point_dist> <point_dir>]
    //   (<stamina> <effort> <recovery>[ <capacity>])
    //   [t|k|f] [y|r])
    while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "(p"
    while ( *msg != '\0' )
    {
        while ( *msg != '\0' && *msg != 'p' ) ++msg; // skip to "p"
        if ( *msg == '\0' )
        {
            break;
        }

        PlayerT player;

        while ( *msg != '\0' && *msg != ' ' ) ++msg;
        ++msg; // skip "p "
        player.side_ = ( *msg == 'l'
                         ? LEFT
                         : RIGHT );

        msg += 2; // skip "l " or "r "
        player.unum_ = static_cast< int >( std::strtol( msg, &next, 10 ) );
        msg = next;

        while ( *msg == ' ' ) ++msg;

        if ( *msg == 'g' )
        {
            player.goalie_ = true;
            player.type_ = Hetero_Default;
            ++msg;
            while ( *msg == ' ' ) ++msg;
        }

        if ( std::isdigit( *msg ) )
        {
            player.type_ = static_cast< int >( std::strtol( msg, &next, 10 ) );
            msg = next;
        }

        while ( *msg == ' ' || *msg == ')' ) ++msg; // skip to x pos

        player.pos_.x = std::strtod( msg, &next ); msg = next;
        player.pos_.y = std::strtod( msg, &next ); msg = next;
        player.vel_.x = std::strtod( msg, &next ); msg = next;
        player.vel_.y = std::strtod( msg, &next ); msg = next;
        player.body_ = std::strtod( msg, &next ); msg = next;
        player.neck_ = std::strtod( msg, &next ); msg = next;
        ++msg;
        if ( *msg != '(' )
        {
            player.pointto_dist_ = std::strtod( msg, &next ); msg = next;
            player.pointto_dir_ = std::strtod( msg, &next ); msg = next;
        }
        while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "(stamina"
        while ( *msg != '\0' && *msg != ' ' ) ++msg; // skip to space " "
        ++msg;
        player.stamina_ = std::strtod( msg, &next ); msg = next;
        player.effort_ = std::strtod( msg, &next ); msg = next;
        player.recovery_ = std::strtod( msg, &next ); msg = next;
        if ( *msg != ')' )
        {
            player.stamina_capacity_ = std::strtod( msg, &next ); msg = next;
        }

        while ( *msg == ')' ) ++msg;
        while ( *msg == ' ' ) ++msg;

        if ( *msg == 'k' ) // kick
        {
            player.kicked_ = true;
            ++msg;
            while ( *msg == ' ' ) ++msg;
        }
        else if ( *msg == 't' ) // tackle
        {
            player.tackle_ = true;
            ++msg;
            while ( *msg == ' ' ) ++msg;
        }
        else if ( *msg == 'f' )
        {
            player.charged_ = true;
            ++msg;
            while ( *msg == ' ' ) ++msg;
        }

        if ( *msg == 'y' ) // yellow card
        {
            player.card_ = YELLOW;
        }
        else if ( *msg == 'r' ) // red card
        {
            player.card_ = RED;
        }

        if ( LEFT == player.side_ )
        {
            M_left_team.push_back( player );
        }
        else
        {
            M_right_team.push_back( player );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FullstateSensor::parseV7( const char * msg )
{
    /*
      (fullstate <time> (pmode <play_mode>) (vmode <qual> <width>)
      (score <left> <right>)
      (ball <x> <y> <vx> <vy>)
      ({l|r}_<unum> <x> <y> <vx> <vy> <body> <neck> <stamina> <effort> <recovery>)
      ...)\n
    */

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   This class doesn't manage playmode & view mode
    // !!!!!!!!!!!!!!!!!!!!!    //! left team score!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    char *next;

    while ( *msg != ' ' ) ++msg; // skip "(fullstate"
    while ( *msg != '(' ) ++msg; // skip to "(pmode"

    while ( *msg != ' ' ) ++msg; // skip "(pmode"
    ++msg;
    // get_playmode(msg); // ignore playmode info

    while ( *msg != '(' ) ++msg; // skip to (vmode
    ++msg; // skip paren
    // get_view_mode(msg); // ignore view mode info

    while ( *msg != '(' ) ++msg; // skip to (score
    while ( *msg != ' ' ) ++msg; // skip to " LSCORE..."
    M_left_score = static_cast< int >( std::strtol( msg, &next, 10 ) ); msg = next;
    M_right_score = static_cast< int >( std::strtol( msg, &next, 10 ) ); msg = next;

    while ( *msg != '(' ) ++msg; // skip to (ball
    while ( *msg != ' ' ) ++msg; // skip "(ball"

    M_ball.pos_.x = std::strtod( msg, &next ); msg = next;
    M_ball.pos_.y = std::strtod( msg, &next ); msg = next;
    M_ball.vel_.x = std::strtod( msg, &next ); msg = next;
    M_ball.vel_.y = std::strtod( msg, &next ); msg = next;

    while ( *msg != '\0' )
    {
        // ({l|r}_<unum> <x> <y> <vx> <vy> <body> <neck> <stamina> <effort> <recovery>)
        while ( *msg != '\0' && *msg != '(' ) ++msg; // skip to "({l|r}"
        if ( *msg == '\0' )
        {
            break;
        }

        PlayerT player;

        ++msg; // skip "("

        player.side_ = ( *msg == 'l'
                         ? LEFT
                         : RIGHT );

        msg += 2; // skip "l_" or "r_"
        player.unum_ = std::atoi( msg );
        msg += 2; // skip to x pos

        player.pos_.x = std::strtod( msg, &next ); msg = next;
        player.pos_.y = std::strtod( msg, &next ); msg = next;
        player.vel_.x = std::strtod( msg, &next ); msg = next;
        player.vel_.y = std::strtod( msg, &next ); msg = next;
        player.body_ = std::strtod( msg, &next ); msg = next;
        player.neck_ = std::strtod( msg, &next ); msg = next;
        player.stamina_ = std::strtod( msg, &next ); msg = next;
        player.effort_ = std::strtod( msg, &next ); msg = next;
        player.recovery_ = std::strtod( msg, &next ); msg = next;
        // now, msg point the last paren of this player

        if ( LEFT == player.side_ )
        {
            M_left_team.push_back( player );
        }
        else
        {
            M_right_team.push_back( player );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
std::ostream &
FullstateSensor::print( std::ostream & os ) const
{
    os << "Fullstate: " << M_time
       << " score " << M_left_score << " - " << M_right_score
       << '\n';

    os << "FS ball "
       << M_ball.pos_ << M_ball.vel_ << M_ball.vel_.r()
       << '\n';

#if 0
    std::copy( M_left_team.begin(), M_left_team.end(),
               std::ostream_iterator< PlayerT >( os, "\n" ) );
    std::copy( M_right_team.begin(), M_right_team.end(),
               std::ostream_iterator< PlayerT >( os, "\n" ) );
#else
    for ( PlayerCont::const_iterator it = M_left_team.begin();
          it != M_left_team.end();
          ++it )
    {
        it->print( os );
    }
    for ( PlayerCont::const_iterator it = M_right_team.begin();
          it != M_right_team.end();
          ++it )
    {
        it->print( os );
    }
#endif
    return os;
}

/*-------------------------------------------------------------------*/
/*!

*/
#if 0
void
FullstateSensor::printWithWorld( const WorldModel & world ) const
{
    Vector2D tmpv;
    double tmpval;
    dlog.addText( Logger::WORLD,
                  "FS ball    (%+.3f %+.3f) (%+.3f %+.3f) %.3f",
                  ball().pos_.x, ball().pos_.y,
                  ball().vel_.x, ball().vel_.y,
                  ball().vel_.r() );
    dlog.addText( Logger::WORLD,
                  "____internal (%+.3f %+.3f) (%+.3f %+.3f) %.3f  gconf=%d rconf=%d",
                  world.ball().pos().x, world.ball().pos().y,
                  world.ball().vel().x, world.ball().vel().y,
                  world.ball().vel().r(),
                  world.ball().posCount(), world.ball().rposCount() );
    tmpv = ball().pos_ - world.ball().pos();
    dlog.addText( Logger::WORLD,
                  "__ball pos err (%+.3f %+.3f) %.3f",
                  tmpv.x, tmpv.y, tmpv.r() );
    dlog.addText( Logger::WORLD,
                  "____internal (%+.3f %+.3f)",
                  world.ball().posError().x, world.ball().posError().y );
    tmpv = ball().vel_ - world.ball().vel();
    tmpval = tmpv.r();
    dlog.addText( Logger::WORLD,
                  "__ball vel err (%+.3f %+.3f) %.3f  %s",
                  tmpv.x, tmpv.y, tmpval, (tmpval > 1.0 ? "big error" : "" ) );
    dlog.addText( Logger::WORLD,
                  "____internal (%+.3f %+.3f)",
                  world.ball().velError().x, world.ball().velError().y );

    const FullstateSensor::PlayerCont& player_cont
        = ( world.isOurLeft()
            ? leftTeam()
            : rightTeam() );
    for ( FullstateSensor::PlayerCont::const_iterator it = player_cont.begin();
          it != player_cont.end();
          ++it )
    {
        if ( it->unum_ == world.self().unum() )
        {
            dlog.addText( Logger::WORLD,
                          "FS self  (%+.3f %+.3f) (%+.3f %+.3f) b=%+.2f n=%+.2f f=%+.2f",
                          it->pos_.x, it->pos_.y,
                          it->vel_.x, it->vel_.y,
                          it->body_, it->neck_,
                          AngleDeg::normalize_angle( it->body_ + it->neck_ ) );

            dlog.addText( Logger::WORLD,
                          "____internal (%+.3f %+.3f) (%+.3f %+.3f) b=%+.2f n=%+.2f f=%+.2f",
                          world.self().pos().x, world.self().pos().y,
                          world.self().vel().x, world.self().vel().y,
                          world.self().body().degree(),
                          world.self().neck().degree(),
                          world.self().face().degree() );

            tmpv = it->pos_ - world.self().pos();
            double d = tmpv.r();
            dlog.addText( Logger::WORLD,
                          "__self pos err (%+.3f %+.3f) %.3f %s",
                          tmpv.x, tmpv.y, d, ( d > 0.3 ? "  big error" : "" ) );
            dlog.addText( Logger::WORLD,
                          "____internal (%+.3f %+.3f) %.3f",
                          world.self().posError().x,
                          world.self().posError().y,
                          world.self().posError().r() );
            tmpv = it->vel_ - world.self().vel();
            dlog.addText( Logger::WORLD,
                          "__self vel err (%+.3f %+.3f) %.3f",
                          tmpv.x, tmpv.y, tmpv.r() );
            dlog.addText( Logger::WORLD,
                          "____internal (%+.3f %+.3f) %.3f",
                          world.self().velError().x,
                          world.self().velError().y,
                          world.self().velError().r() );
            tmpv = ball().pos_ - it->pos_;
            dlog.addText( Logger::WORLD,
                          "__ball rpos (%+.3f %+.3f) %.3f",
                          tmpv.x, tmpv.y, tmpv.r() );
            dlog.addText( Logger::WORLD,
                          "____internal (%+.3f %+.3f) %.3f",
                          world.ball().rpos().x,
                          world.ball().rpos().y,
                          world.ball().rpos().r() );
            tmpv -= world.ball().rpos();
            dlog.addText( Logger::WORLD,
                          "__ball rpos err (%+.3f %+.3f) %.3f",
                          tmpv.x, tmpv.y, tmpv.r() );
            dlog.addText( Logger::WORLD,
                          "____internal (%+.3f %+.3f) %.3f",
                          world.ball().rposError().x,
                          world.ball().rposError().y,
                          world.ball().rposError().r() );
            break;
        }
    }
}
#endif

}

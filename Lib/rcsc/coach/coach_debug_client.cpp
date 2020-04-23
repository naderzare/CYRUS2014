// -*-c++-*-

/*!
  \file coach_debug_client.cpp
  \brief interface for visual debug server Header File
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

#include "coach_debug_client.h"

#include "global_world_model.h"

#include <rcsc/net/udp_socket.h>
#include <rcsc/types.h>

#include <algorithm>
#include <sstream>
#include <cstdio>
#include <cstdarg>
#include <cmath>

namespace rcsc {

// accessible only from this file
namespace {

//! max send buffer size for debug client
#define G_BUFFER_SIZE 8192*4

//! global variable
static char g_buffer[G_BUFFER_SIZE];


//! rounding utility
inline
double
ROUND( const double & val,
       const double & step )
{
    return rint( val / step ) * step;
}

/*-------------------------------------------------------------------*/

class LinePrinter {
private:
    std::ostream & M_os;
public:
    LinePrinter( std::ostream & os )
        : M_os( os )
      { }
    void operator()( const Segment2D & line )
      {
          M_os << " (line "
               << ROUND( line.origin().x, 0.001 ) << ' '
               << ROUND( line.origin().y, 0.001 ) << ' '
               << ROUND( line.terminal().x, 0.001 ) << ' '
               << ROUND( line.terminal().y, 0.001 ) << ')';
      }
};


class TrianglePrinter {
private:
    std::ostream & M_os;
public:
    TrianglePrinter( std::ostream & os )
        : M_os( os )
      { }
    void operator()( const Triangle2D & tri )
      {
          M_os << " (tri "
               << ROUND( tri.a().x, 0.001 ) << ' '
               << ROUND( tri.a().y, 0.001 ) << ' '
               << ROUND( tri.b().x, 0.001 ) << ' '
               << ROUND( tri.b().y, 0.001 ) << ' '
               << ROUND( tri.c().x, 0.001 ) << ' '
               << ROUND( tri.c().y, 0.001 ) << ')';
      }
};

class RectPrinter {
private:
    std::ostream & M_os;
public:
    RectPrinter( std::ostream & os )
        : M_os( os )
      { }
    void operator()( const Rect2D & rect )
      {
          M_os << " (rect "
               << ROUND( rect.left(), 0.001 ) << ' '
               << ROUND( rect.top(), 0.001 ) << ' '
               << ROUND( rect.right(), 0.001 ) << ' '
               << ROUND( rect.bottom(), 0.001 ) << ')';
      }
};

class CirclePrinter {
private:
    std::ostream & M_os;
public:
    CirclePrinter( std::ostream & os )
        : M_os( os )
      { }
    void operator()( const Circle2D & circle )
      {
          M_os << " (circle "
               << ROUND( circle.center().x, 0.001 ) << ' '
               << ROUND( circle.center().y, 0.001 ) << ' '
               << ROUND( circle.radius(), 0.001 ) << ')';
      }
};

}

/*-------------------------------------------------------------------*/
/*!

*/
CoachDebugClient::CoachDebugClient()
    : M_on( false ),
      M_connected( false ),
      M_write_mode( false ),
      M_main_buffer( "" ),
      M_target_unum( Unum_Unknown ),
      M_target_point( Vector2D::INVALIDATED ),
      M_message( "" )
{
    M_main_buffer.reserve( 8192 );
    M_message.reserve( 8192 );

    M_lines.reserve( MAX_LINE );
    M_triangles.reserve( MAX_TRIANGLE );
    M_rectangles.reserve( MAX_RECT );
    M_circles.reserve( MAX_CIRCLE );
}

/*-------------------------------------------------------------------*/
/*!

*/
CoachDebugClient::~CoachDebugClient()
{
    this->close();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachDebugClient::connect( const std::string & hostname,
                           const int port )
{
    M_socket = boost::shared_ptr< UDPSocket >( new UDPSocket( hostname.c_str(), port ) );

    if ( ! M_socket
         || M_socket->fd() == -1 )
    {
        std::cout << __FILE__ << ' ' << __LINE__
                  << ":cannot connect to the debug server host=["
                  << hostname << "] port=[" << port << "]"
                  << std::endl;
        if ( M_socket )
        {
            M_socket.reset();
        }

        M_connected = false;
        return false;
    }

    M_on = true;
    M_connected = true;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
CoachDebugClient::open( const std::string & log_dir,
                        const std::string & teamname )
{
    if ( M_server_log.is_open() )
    {
        M_server_log.close();
    }

    std::ostringstream filepath;
    if ( ! log_dir.empty() )
    {
        filepath << log_dir;
        if ( *log_dir.rbegin() != '/' )
        {
            filepath << '/';
        }
    }
    filepath << teamname << "-coach" << ".dcl";

    M_server_log.open( filepath.str().c_str() );

    if ( ! M_server_log.is_open() )
    {
        return false;
    }

    M_on = true;
    M_write_mode = true;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::writeAll( const GlobalWorldModel & world )
{
    if ( M_on )
    {
        this->toStr( world );

        if ( M_connected )
        {
            this->send();
        }

        if ( M_write_mode
             && world.time().stopped() == 0 )
        {
            this->write( world.time().cycle() );
        }

        this->clear();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::close()
{
    if ( M_connected
         && M_socket )
    {
        M_socket->close();
        M_socket.reset();
    }

    if ( M_server_log.is_open() )
    {
        M_server_log.flush();
        M_server_log.close();
    }

    M_write_mode = false;
    M_on = false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::toStr( const GlobalWorldModel & world )
{
    std::ostringstream ostr;

    ostr << "((debug (format-version 3)) (time "
         << world.time().cycle() << ")";


    // coach identifier
    if ( world.ourSide() == LEFT )
    {
        ostr << " (s l c)";
    }
    else
    {
        ostr << " (s r c)";
    }

    // say message
    // if ( ! effector.getSayMessage().empty() )
    // {
    //     ostr << " (say \"";
    //     for ( std::vector< const SayMessage * >::const_iterator it = effector.sayMessageCont().begin();
    //           it != effector.sayMessageCont().end();
    //           ++it )
    //     {
    //         (*it)->printDebug( ostr );
    //     }
    //     ostr << " {" << effector.getSayMessage() << "}\")";
    // }

    // heard information
    // if ( world.audioMemory().time() == world.time() )
    // {
    //     ostr << " (hear ";
    //     world.audioMemory().printDebug( ostr );
    //     ostr << ')';
    // }

    // target number
    if ( M_target_unum != Unum_Unknown )
    {
        ostr << " (target-teammate " << M_target_unum << ")";
    }

    // target point
    if ( M_target_point.isValid() )
    {
        ostr << " (target-point "
             << M_target_point.x << " " << M_target_point.y
             << ")";
    }

    // message
    if ( ! M_message.empty() )
    {
        ostr << " (message \"" << M_message << "\")";
    }

    // lines
    std::for_each( M_lines.begin(), M_lines.end(),
                   LinePrinter( ostr ) );
    // triangles
    std::for_each( M_triangles.begin(), M_triangles.end(),
                   TrianglePrinter( ostr ) );
    // rectangles
    std::for_each( M_rectangles.begin(), M_rectangles.end(),
                   RectPrinter( ostr ) );
    // circles
    std::for_each( M_circles.begin(), M_circles.end(),
                   CirclePrinter( ostr ) );

    ostr << ")";

    M_main_buffer.assign( ostr.str() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::send()
{
    if ( M_connected
         && M_socket )
    {
        if ( M_socket->send( M_main_buffer.c_str(),
                             M_main_buffer.length() + 1 ) == -1 )
        {
            std::cerr << "debug server send error" << std::endl;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::write( const long & cycle )
{
#if 0
    if ( dlog.isOpen() )
    {
//         char buf[32];
//         snprintf( buf, 32, "%%%% step %ld\n", cycle );
//         dlog.print( buf );
//         dlog.print( "%% debug [" );
//         dlog.print( M_main_buffer.c_str() );
//         dlog.print( "]\n" );
        dlog.printDebugClientMessage( cycle, M_main_buffer );
    }
#endif

    if ( M_server_log.is_open() )
    {
        char buf[32];
        snprintf( buf, 32, "%%%% step %ld\n", cycle );

        M_server_log << buf << "%% debug [" << M_main_buffer << "]"
                     << std::endl;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::clear()
{
    M_main_buffer.erase();

    M_target_unum = Unum_Unknown;
    M_target_point.invalidate();
    M_message.erase();

    M_lines.clear();
    M_triangles.clear();
    M_rectangles.clear();
    M_circles.clear();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::addMessage( const char * msg,
                              ... )
{
    if ( M_on )
    {
        va_list argp;
        va_start( argp, msg );
        vsnprintf( g_buffer, G_BUFFER_SIZE, msg, argp );
        va_end( argp );

        M_message += g_buffer;
        M_message += "/";
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::addLine( const Vector2D & from,
                           const Vector2D & to )
{
    if ( M_on )
    {
        if ( M_lines.size() < MAX_LINE )
        {
            M_lines.push_back( Segment2D( from, to ) );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::addTriangle( const Triangle2D & tri )
{
    if ( M_on )
    {
        if ( M_triangles.size() < MAX_TRIANGLE )
        {
            M_triangles.push_back( tri );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::addRectangle( const Rect2D & rect )
{
    if ( M_on )
    {
        if ( M_rectangles.size() < MAX_RECT )
        {
            M_rectangles.push_back( rect );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachDebugClient::addCircle( const Circle2D & circle )
{
    if ( M_on )
    {
        if ( M_circles.size() < MAX_CIRCLE )
        {
            M_circles.push_back( circle );
        }
    }
}

}

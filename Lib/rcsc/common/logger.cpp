// -*-c++-*-

/*!
  \file logger.cpp
  \brief Logger class Source File
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

#include "logger.h"
#include <rcsc/game_time.h>

#include <string>
#include <iostream>
#include <cstdarg>
#include <cstring>

namespace rcsc {

namespace  {

//! buffer size for the log message.
#define G_BUFFER_SIZE 2048

//! temporary buffer
char g_buffer[G_BUFFER_SIZE];

//! main buffer
std::string g_str;

}

//! global variable
Logger dlog;

/*-------------------------------------------------------------------*/
/*!

 */
Logger::Logger()
    : M_time( static_cast< GameTime * >( 0 ) )
    , M_fout( NULL )
    , M_flags( 0 )
{
    g_str.reserve( 8192 * 4 );
    std::strcpy( g_buffer, "" );
}

/*-------------------------------------------------------------------*/
/*!

 */
Logger::~Logger()
{
    if ( M_fout )
    {
        this->flush();
        fclose( M_fout );
        M_fout = NULL;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::open( const std::string & filepath )
{
    M_fout = std::fopen( filepath.c_str(), "w" );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::setLogFlag( const GameTime * time,
                    const boost::int32_t level,
                    const bool on )
{
    M_time = time;
    if ( on )
    {
        M_flags |= level;
    }
    else
    {
        M_flags &= ~level;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::flush()
{
    if ( M_fout && g_str.length() > 0 )
    {
        fputs( g_str.c_str(), M_fout );
        //fwrite( g_str.c_str(), sizeof( char ), g_str.length(), M_fout );
        fflush( M_fout );
    }
    g_str.erase();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::clear()
{
    g_str.erase();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addText( const boost::int32_t level,
                 const char * msg,
                 ... )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        va_list argp;
        va_start( argp, msg );
        vsnprintf( g_buffer, G_BUFFER_SIZE, msg, argp );
        va_end( argp );

        char header[32];
        snprintf( header, 32, "%ld %d M ",
                  M_time->cycle(),
                  level );

        g_str += header;
        g_str += g_buffer;
        g_str += '\n';
        if ( g_str.length() > 8192 * 3 )
        {
            flush();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addPoint( const boost::int32_t level,
                  const double & x,
                  const double & y,
                  const char * color )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d p %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  x, y );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addPoint( const boost::int32_t level,
                  const double & x,
                  const double & y,
                  const int r, const int g, const int b )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d p %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  x, y,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addLine( const boost::int32_t level,
                 const double & x1,
                 const double & y1,
                 const double & x2,
                 const double & y2,
                 const char * color )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d l %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  x1, y1, x2, y2 );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addLine( const boost::int32_t level,
                 const double & x1,
                 const double & y1,
                 const double & x2,
                 const double & y2,
                 const int r, const int g, const int b )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d l %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  x1, y1, x2, y2,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addArc( const boost::int32_t level,
                const double & x,
                const double & y,
                const double & radius,
                const AngleDeg & start_angle,
                const double & span_angle,
                const char * color )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d a %.4f %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  x, y, radius, start_angle.degree(), span_angle );
        g_str += msg;

        if ( color )
        {
            g_str += color;
        }

        g_str += '\n';
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addArc( const boost::int32_t level,
                const double & x,
                const double & y,
                const double & radius,
                const AngleDeg & start_angle,
                const double & span_angle,
                const int r, const int g, const int b )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d a %.4f %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  x, y, radius, start_angle.degree(), span_angle,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addCircle( const boost::int32_t level,
                   const double & x,
                   const double & y,
                   const double & radius,
                   const char * color,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  ( fill ? 'C' : 'c' ),
                  x, y, radius );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addCircle( const boost::int32_t level,
                   const double & x,
                   const double & y,
                   const double & radius,
                   const int r, const int g, const int b,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  ( fill ? 'C' : 'c' ),
                  x, y, radius,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addTriangle( const boost::int32_t level,
                     const double & x1,
                     const double & y1,
                     const double & x2,
                     const double & y2,
                     const double & x3,
                     const double & y3,
                     const char * color,
                     const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  ( fill ? 'T' : 't' ),
                  x1, y1, x2, y2, x3, y3 );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addTriangle( const boost::int32_t level,
                     const double & x1,
                     const double & y1,
                     const double & x2,
                     const double & y2,
                     const double & x3,
                     const double & y3,
                     const int r, const int g, const int b,
                     const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  ( fill ? 'T' : 't' ),
                  x1, y1, x2, y2, x3, y3,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addRect( const boost::int32_t level,
                 const double & left,
                 const double & top,
                 const double & length,
                 const double & width,
                 const char * color,
                 const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  ( fill ? 'R' : 'r' ),
                  left, top, length, width );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addRect( const boost::int32_t level,
                 const double & left,
                 const double & top,
                 const double & length,
                 const double & width,
                 const int r, const int g, const int b,
                 const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  ( fill ? 'R' : 'r' ),
                  left, top, length, width,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addSector( const boost::int32_t level,
                   const double & x,
                   const double & y,
                   const double & min_radius,
                   const double & max_radius,
                   const AngleDeg & start_angle,
                   const double & span_angle,
                   const char * color,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  ( fill ? 'S' : 's' ),
                  x, y, min_radius, max_radius,
                  start_angle.degree(), span_angle );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addSector( const boost::int32_t level,
                   const double & x,
                   const double & y,
                   const double & min_radius,
                   const double & max_radius,
                   const AngleDeg & start_angle,
                   const double & span_angle,
                   const int r, const int g, const int b,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  ( fill ? 'S' : 's' ),
                  x, y, min_radius, max_radius,
                  start_angle.degree(), span_angle,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addSector( const boost::int32_t level,
                   const Sector2D & sector,
                   const char * color,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        double span_angle = ( sector.angleLeftStart().isLeftOf( sector.angleRightEnd() )
                              ? ( sector.angleLeftStart() - sector.angleRightEnd() ).abs()
                              : 360.0 - ( sector.angleLeftStart() - sector.angleRightEnd() ).abs() );
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  ( fill ? 'S' : 's' ),
                  sector.center().x, sector.center().y,
                  sector.radiusMin(), sector.radiusMax(),
                  sector.angleLeftStart().degree(), span_angle );
        g_str += msg;
        if ( color )
        {
            g_str += color;
        }
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addSector( const boost::int32_t level,
                   const Sector2D & sector,
                   const int r, const int g, const int b,
                   const bool fill )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char msg[128];
        double span_angle = ( sector.angleLeftStart().isLeftOf( sector.angleRightEnd() )
                              ? ( sector.angleLeftStart() - sector.angleRightEnd() ).abs()
                              : 360.0 - ( sector.angleLeftStart() - sector.angleRightEnd() ).abs() );
        snprintf( msg, 128, "%ld %d %c %.4f %.4f %.4f %.4f %.4f %.4f #%02x%02x%02x",
                  M_time->cycle(),
                  level,
                  ( fill ? 'S' : 's' ),
                  sector.center().x, sector.center().y,
                  sector.radiusMin(), sector.radiusMax(),
                  sector.angleLeftStart().degree(), span_angle,
                  r, g, b );
        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addMessage( const boost::int32_t level,
                    const double & x,
                    const double & y,
                    const char * msg,
                    const char * color )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char header[128];
        snprintf( header, 128, "%ld %d m %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  x, y );
        g_str += header;

        if ( color )
        {
            g_str += "(c ";
            g_str += color;
            g_str += ") ";
        }

        g_str += msg;
        g_str += '\n';
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Logger::addMessage( const boost::int32_t level,
                    const double & x,
                    const double & y,
                    const char * msg,
                    const int r, const int g, const int b )
{
    if ( M_fout && ( level & M_flags ) && M_time )
    {
        char header[128];
        snprintf( header, 128, "%ld %d m %.4f %.4f ",
                  M_time->cycle(),
                  level,
                  x, y );
        g_str += header;

        char col[8];
        snprintf( col, 8, "#%02x%02x%02x", r, g, b );
        g_str += "(c ";
        g_str += col;
        g_str += ") ";

        g_str += msg;
        g_str += '\n';
    }
}

}

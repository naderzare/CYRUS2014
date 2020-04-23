// -*-c++-*-

/*!
  \file basic_client.cpp
  \brief abstract soccer client class Source File.
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

#include "basic_client.h"

#include "soccer_agent.h"

#include <rcsc/gz/gzcompressor.h>
#include <rcsc/net/udp_socket.h>

#include <iostream>
#include <cstring>

#include <unistd.h> // select()
#include <sys/select.h> // select()
#include <sys/time.h> // select()
#include <sys/types.h> // select()

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
BasicClient::BasicClient()
    : M_client_mode( ONLINE ),
      M_server_alive( false ),
      M_interval_msec( 10 ),
      M_compression_level( 0 )
{
    std::memset( M_message, 0, MAX_MESG );
    M_compression_message.reserve( MAX_MESG );
    M_decompression_message.reserve( MAX_MESG );
}

/*-------------------------------------------------------------------*/
/*!

*/
BasicClient::~BasicClient()
{
    //std::cerr << "delete BasicClient" << std::endl;
    if ( M_offline_out.is_open() )
    {
        M_offline_out.flush();
        M_offline_out.close();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::run( SoccerAgent * agent )
{
    assert( agent );

    if ( clientMode() == ONLINE )
    {
        runOnline( agent );
    }
    else // if ( clientMode() == OFFLINE )
    {
        runOffline( agent );
    }

    agent->handleExit();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::runOnline( SoccerAgent * agent )
{
    if ( ! agent->handleStart()
         || ! isServerAlive() )
    {
        agent->handleExit();
        return;
    }

    // set interval timeout
    struct timeval interval;

    fd_set read_fds;
    fd_set read_fds_back;

    FD_ZERO( &read_fds );
    FD_SET( M_socket->fd(), &read_fds );
    read_fds_back = read_fds;

    int timeout_count = 0;
    long waited_msec = 0;

    while ( isServerAlive() )
    {
        read_fds = read_fds_back;
        interval.tv_sec = M_interval_msec / 1000;
        interval.tv_usec = ( M_interval_msec % 1000 ) * 1000;

        int ret = ::select( M_socket->fd() + 1, &read_fds,
                            static_cast< fd_set * >( 0 ),
                            static_cast< fd_set * >( 0 ),
                            &interval );
        if ( ret < 0 )
        {
            perror( "select" );
            break;
        }
        else if ( ret == 0 )
        {
            // no meesage. timeout.
            waited_msec += M_interval_msec;
            ++timeout_count;
            agent->handleTimeout( timeout_count,
                                  waited_msec );
        }
        else
        {
            // received message, reset wait time
            waited_msec = 0;
            timeout_count = 0;
            agent->handleMessage();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::runOffline( SoccerAgent * agent )
{
    if ( ! agent->handleStartOffline() )
    {
        agent->handleExit();
        return;
    }

    while ( isServerAlive() )
    {
        agent->handleMessageOffline();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::setIntervalMSec( const long & interval_msec )
{
    if ( interval_msec <= 0 )
    {
        std::cerr << "***ERROR*** interval msec must be positive value. ["
                  << interval_msec << "]"
                  << std::endl;
        return;
    }

    if ( interval_msec < 10 )
    {
        std::cerr << "***ERROR*** interval msec should be more than or equal 10. ["
                  << interval_msec << "]"
                  << std::endl;
        return;
    }

    M_interval_msec = interval_msec;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::setServerAlive( const bool alive )
{
    M_server_alive = alive;
}

/*-------------------------------------------------------------------*/
/*!

*/
int
BasicClient::setCompressionLevel( const int level )
{
#ifdef HAVE_LIBZ
    if ( level < 0 || 9 <= level )
    {
        std::cerr << "***ERROR*** unsupported compression level "
                  << level << std::endl;
        return M_compression_level;
    }


    int old_level = M_compression_level;
    M_compression_level = level;

    if ( level == 0 )
    {
        M_compressor.reset();
        M_decompressor.reset();
        return old_level;
    }

    M_compressor
        = boost::shared_ptr< GZCompressor >( new GZCompressor( level ) );

    M_decompressor
        = boost::shared_ptr< GZDecompressor >( new GZDecompressor() );

    return old_level;
#else
    return 0;
#endif
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
BasicClient::openOfflineLog( const std::string & filepath )
{
    if ( clientMode() == ONLINE )
    {
        M_offline_out.close();
        M_offline_out.open( filepath.c_str() );
        if ( ! M_offline_out.is_open() )
        {
            return false;
        }

        if ( ! M_decompression_message.empty() )
        {
            M_offline_out << M_decompression_message << std::endl;
        }

        return true;
    }
    else if ( clientMode() == OFFLINE )
    {
        M_offline_in.close();
        M_offline_in.open( filepath.c_str() );

        return M_offline_in.is_open();
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BasicClient::printOfflineThink()
{
    if ( M_offline_out.is_open() )
    {
        M_offline_out << "(think)" << std::endl;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
BasicClient::connectTo( const char * hostname,
                        const int port,
                        const long & interval_msec )
{
    if ( clientMode() != ONLINE )
    {
        M_socket.reset();
        return true;
    }

    M_socket = boost::shared_ptr< UDPSocket >( new UDPSocket( hostname, port ) );

    if ( ! M_socket
         || M_socket->fd() == -1 )
    {
        std::cerr << "BasicClinet::connectTo() Failed to create connection."
                  << std::endl;
        setServerAlive( false );
        return false;
    }

    setServerAlive( true );

    setIntervalMSec( interval_msec );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
int
BasicClient::sendMessage( const char * msg )
{
    if ( clientMode() != ONLINE )
    {
        return 1;
    }

    if ( ! M_socket )
    {
        return 0;
    }

#ifdef HAVE_LIBZ
    if ( M_compression_level > 0
         && M_compressor )
    {
        M_compressor->compress( msg,
                                std::strlen( msg ) + 1,
                                M_compression_message );

        if ( ! M_compression_message.empty() )
        {
            return M_socket->send( M_compression_message.data(),
                                   M_compression_message.length() );
        }

        return 0;
    }
#endif

    //std::cerr << "send [" << msg << "0]" << endl;
    // if the length of message is the result of only strlen,
    // server will reply "(warning message_not_null_terminated)"
    return M_socket->send( msg, std::strlen( msg ) + 1 );
}

/*-------------------------------------------------------------------*/
/*!

*/
int
BasicClient::recvMessage()
{
    //
    // offline mode
    //

    if ( clientMode() == OFFLINE )
    {
        while ( std::getline( M_offline_in, M_decompression_message ) )
        {
            if ( M_decompression_message.empty() ) continue;

            return M_decompression_message.size();
        }

        setServerAlive( false );
        return 0;
    }

    //
    // online mode
    //

    if ( ! M_socket )
    {
        return 0;
    }

    int n = M_socket->receive( M_message, MAX_MESG );

    if ( n > 0 )
    {
#ifdef HAVE_LIBZ
        if ( M_compression_level > 0
             && M_decompressor )
        {
            M_decompressor->decompress( M_message,
                                        n,
                                        M_decompression_message );
        }
        else
#endif
        {
            M_message[n] = '\0';
            M_decompression_message = M_message;
        }

        if ( M_offline_out.is_open() )
        {
            M_offline_out << M_decompression_message << '\n';
        }
    }

    return n;
}

}

// -*-c++-*-

/*!
  \file rcgrenameteam.cpp
  \brief team name renamer in the rcg file source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

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

#include <rcsc/gz.h>
#include <rcsc/rcg.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

class TeamNameRenamer
    : public rcsc::rcg::Handler {
private:

    std::ostream & M_os;

    rcsc::rcg::Serializer::Ptr M_serializer;

    std::string M_left_team_name;
    std::string M_right_team_name;

    // not used
    TeamNameRenamer();
public:

    TeamNameRenamer( std::ostream & os,
                     const std::string & left_team_name,
                     const std::string & right_team_name );

    bool handleLogVersion( const int ver );

    // v3 or older
    bool handleDispInfo( const rcsc::rcg::dispinfo_t & disp );
    bool handleShowInfo( const rcsc::rcg::showinfo_t & show );
    bool handleShortShowInfo2( const rcsc::rcg::short_showinfo_t2 & show );
    bool handleMsgInfo( rcsc::rcg::Int16 board,
                        const std::string & msg );
    bool handlePlayMode( char playmode );
    bool handleTeamInfo( const rcsc::rcg::team_t & team_left,
                         const rcsc::rcg::team_t & team_right );
    bool handleServerParam( const rcsc::rcg::server_params_t & param );
    bool handlePlayerParam( const rcsc::rcg::player_params_t & param );
    bool handlePlayerType( const rcsc::rcg::player_type_t & param );

    bool handleEOF();

    // v4 or later
    bool handleShow( const int time,
                     const rcsc::rcg::ShowInfoT & show );
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg );
    bool handlePlayMode( const int time,
                         const rcsc::PlayMode pm );
    bool handleTeam( const int time,
                     const rcsc::rcg::TeamT & team_l,
                     const rcsc::rcg::TeamT & team_r );
    bool handleServerParam( const std::string & msg );
    bool handlePlayerParam( const std::string & msg );
    bool handlePlayerType( const std::string & msg );
};


/*-------------------------------------------------------------------*/
/*!

*/
TeamNameRenamer::TeamNameRenamer( std::ostream & os,
                                  const std::string & left_team_name,
                                  const std::string & right_team_name )
    : M_os( os )
    , M_left_team_name( left_team_name )
    , M_right_team_name( right_team_name )
{
    M_serializer = rcsc::rcg::Serializer::create( 1 );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleLogVersion( const int ver )
{
    rcsc::rcg::Handler::handleLogVersion( ver );

    M_serializer = rcsc::rcg::Serializer::create( ver );

    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serializeHeader( M_os );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleDispInfo( const rcsc::rcg::dispinfo_t & disp )
{
    if ( ! M_serializer )
    {
        return false;
    }

    switch ( ntohs( disp.mode ) ) {
    case rcsc::rcg::SHOW_MODE:
        {
            rcsc::rcg::dispinfo_t new_disp = disp;

            if ( ! M_left_team_name.empty()
                 && M_left_team_name.length() < 16 )
            {
                std::strncpy( new_disp.body.show.team[0].name,
                              M_left_team_name.c_str(),
                              M_left_team_name.length() );
            }

            if ( ! M_right_team_name.empty()
                 && M_right_team_name.length() < 16 )
            {
                std::strncpy( new_disp.body.show.team[1].name,
                              M_right_team_name.c_str(),
                              M_right_team_name.length() );
            }

            M_serializer->serialize( M_os, new_disp );
        }
        break;
    default:
        M_serializer->serialize( M_os, disp );
        break;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleShowInfo( const rcsc::rcg::showinfo_t & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    const std::size_t max_len = 15;
    rcsc::rcg::showinfo_t new_show = show;

    if ( ! M_left_team_name.empty() )
    {
        std::memset( new_show.team[0].name, 0, 16 );
        std::strncpy( new_show.team[0].name,
                      M_left_team_name.c_str(),
                      std::min( max_len, M_left_team_name.length() ) );
    }

    if ( ! M_right_team_name.empty() )
    {
        std::memset( new_show.team[1].name, 0, 16 );
        std::strncpy( new_show.team[1].name,
                      M_right_team_name.c_str(),
                      std::min( max_len, M_right_team_name.length() ) );
    }

    M_serializer->serialize( M_os, new_show );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleShortShowInfo2( const rcsc::rcg::short_showinfo_t2 & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, show );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handlePlayMode( char playmode )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, playmode );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleTeamInfo( const rcsc::rcg::team_t & team_left,
                                 const rcsc::rcg::team_t & team_right )
{
    if ( ! M_serializer )
    {
        return false;
    }

    const std::size_t max_len = 15;

    rcsc::rcg::team_t teams[2];
    teams[0] = team_left;
    teams[1] = team_right;

    if ( ! M_left_team_name.empty() )
    {
        std::memset( teams[0].name, 0, 16 );
        std::strncpy( teams[0].name,
                      M_left_team_name.c_str(),
                      std::min( max_len, M_left_team_name.length() ) );
    }

    if ( ! M_right_team_name.empty() )
    {
        std::memset( teams[1].name, 0, 16 );
        std::strncpy( teams[1].name,
                      M_right_team_name.c_str(),
                      std::min( max_len, M_right_team_name.length() ) );
    }

    M_serializer->serialize( M_os, teams[0], teams[1] );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleServerParam( const rcsc::rcg::server_params_t & param )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, param );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handlePlayerParam( const rcsc::rcg::player_params_t & param )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, param );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleMsgInfo( rcsc::rcg::Int16 board,
                                const std::string & msg )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, board, msg );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handlePlayerType( const rcsc::rcg::player_type_t & param )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, param );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
TeamNameRenamer::handleEOF()
{
    M_os.flush();
    return true;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handleShow( const int,
                             const rcsc::rcg::ShowInfoT & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, show );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handleMsg( const int,
                            const int board,
                            const std::string & msg )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, board, std::string( msg ) );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handlePlayMode( const int,
                                 const rcsc::PlayMode pm )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, static_cast< char >( pm ) );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handleTeam( const int,
                             const rcsc::rcg::TeamT & team_l,
                             const rcsc::rcg::TeamT & team_r )
{
    if ( ! M_serializer )
    {
        return false;
    }

    rcsc::rcg::TeamT teams[2];
    teams[0] = team_l;
    teams[1] = team_r;

    if ( ! M_left_team_name.empty() )
    {
        teams[0].name_ = M_left_team_name;
    }

    if ( ! M_right_team_name.empty() )
    {
        teams[1].name_ = M_right_team_name;
    }

    M_serializer->serialize( M_os, teams[0], teams[1] );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handleServerParam( const std::string & msg )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serializeParam( M_os, msg );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handlePlayerParam( const std::string & msg )
{
   if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serializeParam( M_os, msg );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
TeamNameRenamer::handlePlayerType( const std::string & msg )
{
   if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serializeParam( M_os, msg );
    return true;
}


///////////////////////////////////////////////////////////

/*---------------------------------------------------------------*/
/*
  Usage:
  $ rcgrenameteam <RCGFile>[.gz]
*/
void
usage( const char * prog )
{
    std::cerr << "Usage: " << prog <<  " [Options] <RcgFile>[.gz] -o <OutputFile>\n"
              << "Available options:\n"
              << "    --help [ -h ]\n"
              << "        print this message.\n"
              << "    --left <Value> : (DefaultValue=\"\")\n"
              << "        specify the left team name.\n"
              << "    --right <Value> : (DefaultValue=\"\")\n"
              << "        specify the right team name.\n"
              << "    --output [ -o ]<Value>\n"
              << "        specify the output file name.\n"
              << std::endl;
}


////////////////////////////////////////////////////////////////////////

int
main( int argc, char** argv )
{
    std::string input_file;
    std::string output_file;
    std::string left_team_name;
    std::string right_team_name;

    for ( int i = 1; i < argc; ++i )
    {
        if ( ! std::strcmp( argv[i], "--help" )
             || ! std::strcmp( argv[i], "-h" ) )
        {
            usage( argv[0] );
            return 0;
        }
        else if ( ! std::strcmp( argv[i], "--left" ) )
        {
            ++i;
            if ( i >= argc )
            {
                usage( argv[0] );
                return 1;
            }
            left_team_name = argv[i];
        }
        else if ( ! std::strcmp( argv[i], "--right" ) )
        {
            ++i;
            if ( i >= argc )
            {
                usage( argv[0] );
                return 1;
            }
            right_team_name = argv[i];
        }
        else if ( ! std::strcmp( argv[i], "--output" )
                  || ! std::strcmp( argv[i], "-o" ) )
        {

            ++i;
            if ( i >= argc )
            {
                usage( argv[0] );
                return 1;
            }
            output_file = argv[i];
        }
        else
        {
            input_file = argv[i];
        }
    }

    if ( input_file.empty() )
    {
        std::cerr << "No input file" << std::endl;
        usage( argv[0] );
        return 1;
    }

    if ( output_file.empty() )
    {
        std::cerr << "No output file" << std::endl;
        usage( argv[0] );
        return 1;
    }

    if ( input_file == output_file )
    {
        std::cerr << "The output file is same as the input file." << std::endl;
        return 1;
    }

    if ( left_team_name.empty()
         && right_team_name.empty() )
    {
        std::cerr << "No new team names!" << std::endl;
        usage( argv[0] );
        return 1;
    }

    if ( left_team_name == right_team_name )
    {
        std::cerr << "Same team names!" << std::endl;
        return 1;
    }

    rcsc::gzifstream fin( input_file.c_str() );

    if ( ! fin.is_open() )
    {
        std::cerr << "Failed to open file : " << input_file << std::endl;
        return 1;
    }

    boost::shared_ptr< std::ostream > fout;

    if ( output_file.compare( output_file.length() - 3, 3, ".gz" ) == 0 )
    {
        fout = boost::shared_ptr< std::ostream >
            ( new rcsc::gzofstream( output_file.c_str() ) );
    }
    else
    {
        fout = boost::shared_ptr< std::ostream >
            ( new std::ofstream( output_file.c_str(),
                                 std::ios_base::out | std::ios_base::binary ) );
    }

    if ( ! fout
         || fout->fail() )
    {
        std::cerr << "output stream for the new rcg file. [" << output_file
                  << "] is not good." << std::endl;
        return 1;
    }

    rcsc::rcg::Parser::Ptr parser = rcsc::rcg::Parser::create( fin );

    if ( ! parser )
    {
        std::cerr << "Failed to create rcg parser." << std::endl;
        return 1;
    }

    // create rcg handler instance
    TeamNameRenamer renamer( *fout,
                             left_team_name,
                             right_team_name );

    parser->parse( fin, renamer );

    return 0;
}

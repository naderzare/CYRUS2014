// -*-c++-*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rcsc/gz.h>
#include <rcsc/rcg.h>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

class Reverser
    : public rcsc::rcg::Handler {
private:

    std::ostream & M_os;

    rcsc::rcg::Serializer::Ptr M_serializer;

    // not used
    Reverser();
public:

    explicit
    Reverser( std::ostream & os );

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
    bool handlePlayerType( const rcsc::rcg::player_type_t & param );
    bool handleServerParam( const rcsc::rcg::server_params_t & param );
    bool handlePlayerParam( const rcsc::rcg::player_params_t & param );

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

private:

    rcsc::rcg::pos_t reverse( const rcsc::rcg::pos_t & pos,
                              const bool ball = false );
    rcsc::rcg::ball_t reverse( const rcsc::rcg::ball_t & ball );
    rcsc::rcg::player_t reverse( const rcsc::rcg::player_t & player );

    rcsc::rcg::BallT reverse( const rcsc::rcg::BallT & ball );
    rcsc::rcg::PlayerT reverse( const rcsc::rcg::PlayerT & player );
};


/*-------------------------------------------------------------------*/
/*!

*/
Reverser::Reverser( std::ostream & os )
    : M_os( os )
{
    M_serializer = rcsc::rcg::Serializer::create( 1 );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleLogVersion( const int ver )
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
Reverser::handleDispInfo( const rcsc::rcg::dispinfo_t & disp )
{
    //std::cerr << "handleDispInfo size = " << sizeof( disp ) << std::endl;
    switch ( ntohs( disp.mode ) ) {
    case rcsc::rcg::SHOW_MODE:
        //std::cerr << "SHOW_MODE" << std::endl;
        handleShowInfo( disp.body.show );
        break;
    case rcsc::rcg::MSG_MODE:
        //std::cerr << "MSG_MODE" << std::endl;
        handleMsgInfo( disp.body.msg.board, disp.body.msg.message );
        break;
    case rcsc::rcg::BLANK_MODE:
        std::cerr << "BLANK_MODE" << std::endl;
        break;
    case rcsc::rcg::DRAW_MODE:
    case rcsc::rcg::PM_MODE:
    case rcsc::rcg::TEAM_MODE:
    case rcsc::rcg::PT_MODE:
    case rcsc::rcg::PARAM_MODE:
    case rcsc::rcg::PPARAM_MODE:
    default:
        std::cerr << __FILE__ << ":" << __LINE__
                  << " Unsupported mode " << ntohs( disp.mode )
                  << " in dispinfo_t" << std::endl;
        return false;
        break;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleShowInfo( const rcsc::rcg::showinfo_t & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    rcsc::rcg::showinfo_t new_show;

    new_show.pmode = show.pmode;
    new_show.team[0] = show.team[1];
    new_show.team[1] = show.team[0];

    new_show.pos[0] = reverse( show.pos[0], true );

    for ( int i = 1; i < rcsc::MAX_PLAYER + 1; ++i )
    {
        new_show.pos[i] = reverse( show.pos[i + rcsc::MAX_PLAYER] );
    }
    for ( int i = rcsc::MAX_PLAYER + 1; i < rcsc::MAX_PLAYER * 2 + 1; ++i )
    {
        new_show.pos[i] = reverse( show.pos[i - rcsc::MAX_PLAYER] );
    }

    new_show.time = show.time;

    M_serializer->serialize( M_os, new_show );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleShortShowInfo2( const rcsc::rcg::short_showinfo_t2 & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    rcsc::rcg::short_showinfo_t2 new_show;

    new_show.ball = reverse( show.ball );

    for ( int i = 0; i < rcsc::MAX_PLAYER; ++i )
    {
        new_show.pos[i] = reverse( show.pos[i + rcsc::MAX_PLAYER] );
    }
    for ( int i = rcsc::MAX_PLAYER; i < rcsc::MAX_PLAYER * 2; ++i )
    {
        new_show.pos[i] = reverse( show.pos[i - rcsc::MAX_PLAYER] );
    }

    new_show.time = show.time;

    M_serializer->serialize( M_os, new_show );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleMsgInfo( rcsc::rcg::Int16 board,
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
Reverser::handlePlayMode( char playmode )
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
Reverser::handleTeamInfo( const rcsc::rcg::team_t & team_left,
                          const rcsc::rcg::team_t & team_right )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, team_right, team_left );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handlePlayerType( const rcsc::rcg::player_type_t & param )
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
Reverser::handleServerParam( const rcsc::rcg::server_params_t & param )
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
Reverser::handlePlayerParam( const rcsc::rcg::player_params_t & param )
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
Reverser::handleEOF()
{
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleShow( const int,
                      const rcsc::rcg::ShowInfoT & show )
{
    if ( ! M_serializer )
    {
        return false;
    }

    rcsc::rcg::ShowInfoT new_show;

    new_show.ball_ = reverse( show.ball_ );

    for ( int i = 0; i < rcsc::MAX_PLAYER*2; ++i )
    {
        int idx = show.player_[i].unum_ - 1;
        if ( show.player_[i].side_ == 'l' ) idx += rcsc::MAX_PLAYER;
        if ( idx < 0 || rcsc::MAX_PLAYER*2 <= idx ) continue;

        new_show.player_[idx] = reverse( show.player_[i] );
    }

    new_show.time_ = show.time_;

    M_serializer->serialize( M_os, new_show );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleMsg( const int,
                     const int board,
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
Reverser::handlePlayMode( const int,
                          const rcsc::PlayMode pm )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, pm );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleTeam( const int,
                      const rcsc::rcg::TeamT & team_l,
                      const rcsc::rcg::TeamT & team_r )
{
    if ( ! M_serializer )
    {
        return false;
    }

    M_serializer->serialize( M_os, team_r, team_l );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Reverser::handleServerParam( const std::string & msg )
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
Reverser::handlePlayerParam( const std::string & msg )
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
Reverser::handlePlayerType( const std::string & msg )
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
rcsc::rcg::pos_t
Reverser::reverse( const rcsc::rcg::pos_t & pos,
                   const bool ball )
{
    rcsc::rcg::Int16 enable = ntohs( pos.enable );
    rcsc::rcg::Int16 side = ntohs( pos.side );

    rcsc::rcg::pos_t new_pos;
    new_pos.enable = pos.enable;
    new_pos.side = htons( -side );
    new_pos.unum = pos.unum;

    if ( ball
         || enable != rcsc::rcg::DISABLE )
    {
        rcsc::rcg::Int16 angle = ntohs( pos.angle );
        double x = rcsc::rcg::nstohd( pos.x );
        double y = rcsc::rcg::nstohd( pos.y );

        angle += 180;
        if ( angle > 180 ) angle -= 360;

        new_pos.angle = htons( angle );
        new_pos.x = rcsc::rcg::hdtons( - x );
        new_pos.y = rcsc::rcg::hdtons( - y );
    }
    else
    {
        double x = rcsc::rcg::nstohd( pos.x );

        new_pos.angle = pos.angle;
        new_pos.x = rcsc::rcg::hdtons( - x );
        new_pos.y = pos.y;
    }

    return new_pos;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::rcg::ball_t
Reverser::reverse( const rcsc::rcg::ball_t & ball )
{
    rcsc::rcg::ball_t new_ball;

    new_ball.x = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( ball.x ) );
    new_ball.y = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( ball.y ) );
    new_ball.deltax = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( ball.deltax ) );
    new_ball.deltay = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( ball.deltay ) );

    return new_ball;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::rcg::player_t
Reverser::reverse( const rcsc::rcg::player_t & player )
{
    rcsc::rcg::player_t new_player;

    new_player = player;

    rcsc::rcg::Int16 mode = ntohs( player.mode );

    if ( mode != rcsc::rcg::DISABLE )
    {
        new_player.x = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( player.x ) );
        new_player.y = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( player.y ) );
        new_player.deltax = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( player.deltax ) );
        new_player.deltay = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( player.deltay ) );

        double body_angle = rcsc::rcg::nltohd( player.body_angle );
        body_angle += M_PI;
        if ( body_angle > M_PI ) body_angle -= 2 * M_PI;
        new_player.body_angle = rcsc::rcg::hdtonl( body_angle );
    }
    else
    {
        new_player.x = rcsc::rcg::hdtonl( - rcsc::rcg::nltohd( player.x ) );
    }

    return new_player;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::rcg::BallT
Reverser::reverse( const rcsc::rcg::BallT & ball )
{
    rcsc::rcg::BallT new_ball;

    new_ball.x_ = - ball.x_;
    new_ball.y_ = - ball.y_;
    new_ball.vx_ = - ball.vx_;
    new_ball.vy_ = - ball.vy_;

    return new_ball;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::rcg::PlayerT
Reverser::reverse( const rcsc::rcg::PlayerT & player )
{
    rcsc::rcg::PlayerT new_player = player;

    new_player.side_ = ( player.side_ == 'l' ? 'r' : 'l' );

    if ( player.state_ != rcsc::rcg::DISABLE )
    {
        new_player.x_ = - player.x_;
        new_player.y_ = - player.y_;
        new_player.vx_ = - player.vx_;
        new_player.vy_ = - player.vy_;
        new_player.body_ += 180.0;
        if ( new_player.body_ > 180.0 ) new_player.body_ -= 360.0;
    }
    else
    {
        new_player.x_ = - player.x_;
    }

    return new_player;
}

////////////////////////////////////////////////////////////////////////

int
main( int argc, char** argv )
{
    if ( argc < 2
         || ! strcmp( argv[1], "--help" )
         || ! strcmp( argv[1], "-h" ) )
    {
        std::cerr << "usage: " << argv[0]
                  << " <RcgFile>[.gz]"
                  << " [outputFile]"
                  << std::endl;
        return 0;
    }

    rcsc::gzifstream fin( argv[1] );

    if ( ! fin.is_open() )
    {
        std::cerr << "Failed to open the input file : " << argv[1] << std::endl;
        return 1;
    }

    std::string out_filepath;
    if ( argc >= 3 )
    {
        out_filepath = argv[2];
    }
    else
    {
        out_filepath = "reverse-";
        out_filepath += argv[1];
    }

    if ( out_filepath.length() > 3
         && out_filepath.compare( out_filepath.length() - 3, 3, ".gz" ) == 0 )
    {
        out_filepath.erase( out_filepath.length() - 3 );
    }

    std::ofstream fout( out_filepath.c_str(), std::ios_base::binary );
    if ( ! fout.is_open() )
    {
        std::cerr << "Failed to open the output file : " << out_filepath << std::endl;
        return 1;
    }

    rcsc::rcg::Parser::Ptr parser = rcsc::rcg::Parser::create( fin );

    if ( ! parser )
    {
        std::cerr << "Failed to create rcg parser." << std::endl;
        return 1;
    }

    std::cout << "input file = " << argv[1] << std::endl;
    std::cout << "output file = " << out_filepath << std::endl;

    // create rcg handler instance
    Reverser reverser( fout );

    parser->parse( fin, reverser );

    return 0;
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sample_coach.h"

#include <rcsc/coach/coach_command.h>
#include <rcsc/coach/coach_config.h>
#include <rcsc/coach/coach_debug_client.h>
#include <rcsc/common/basic_client.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

#include <rcsc/coach/global_world_model.h>

#include <cstdio>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

#include "team_logo.xpm"

using namespace rcsc;
using namespace std;

int SampleCoach::isPlayOn = 0 ;
////////////////////////////////// possession Counters /////////////////////////////////////
//Cycle Memory
// if cycle is zero = 0
// if goal = 1
// if corner = 2
// if fouls && Free kicks = 3
// if kick in = 4
// if tackle = 5
// if shoot = 7
// if save = 8
// if goal kick = 9
// if pass = 10
// if dribble = 11

int SampleCoach::cycleMemory = 0;
bool SampleCoach::cycleMemoryChanged = false ;

//Who Kicked Last??
bool SampleCoach::weKickedLast = false;
bool SampleCoach::oppKickedLast = false;
int SampleCoach::kicker = 0 ;
int SampleCoach::kicker2 = 0 ;
int SampleCoach::passKicker = 100;
int SampleCoach::passKicker2 = 100 ;
bool SampleCoach::isKicker2 = false;
bool SampleCoach::kickable = false ;
int SampleCoach::kickableNum = 0 ;
bool SampleCoach::oppKickable = false ;
bool SampleCoach::owner = false ;

//Normal Possession
int SampleCoach::ourPoss = 0;
int SampleCoach::theirPoss = 0;
int SampleCoach::playOnPoss = 0;

//Goals
int SampleCoach::goalDiff = 0;
int SampleCoach::ourGoals = 0;
int SampleCoach::oppGoals = 0;

//Corners
int SampleCoach::ourCornerLeft = 0;
int SampleCoach::ourCornerRight = 0;
int SampleCoach::oppCornerLeft = 0;
int SampleCoach::oppCornerRight = 0;

//Dribble
int SampleCoach::ourDribbles = 0;
int SampleCoach::oppDribbles = 0;
int SampleCoach::ourCorrectDribbles =0;
int SampleCoach::oppCorrectDribbles =0;
int SampleCoach::ourDribblesPercentage =0;
int SampleCoach::oppDribblesPercentage =0;
bool SampleCoach::weDribbled = false ;
bool SampleCoach::oppDribbled = false ;

//Passes
int SampleCoach::ourPasses = 0 ;
int SampleCoach::oppPasses = 0 ;
int SampleCoach::ourCorrectPasses = 0 ;
int SampleCoach::oppCorrectPasses = 0 ;
int SampleCoach::ourPassesPercentage = 0 ;
int SampleCoach::oppPassesPercentage = 0 ;
bool SampleCoach::wePassed = false ;
bool SampleCoach::oppPassed = false ;

//Fouls
int SampleCoach::ourFouls = 0;
int SampleCoach::oppFouls = 0;

//Yellow Cards
int SampleCoach::ourYellowCards = 0;
int SampleCoach::oppYellowCards = 0;

//Outs
int SampleCoach::ourKickIns = 0;
int SampleCoach::oppKickIns = 0;

//Tackles
int SampleCoach::ourTackle = 0;
int SampleCoach::oppTackle = 0;
int SampleCoach::ourTackleCycle = 0 ;
int SampleCoach::oppTackleCycle = 0 ;

//Free kick
int SampleCoach::ourFreeKick = 0;
int SampleCoach::oppFreeKick = 0;
int SampleCoach::ourFreeKickCycle = 0;
int SampleCoach::oppFreeKickCycle = 0;
Vector2D SampleCoach::freeKickPoint = Vector2D(0.0,0.0);

//Shoots
int SampleCoach::ourShoots = 0;
int SampleCoach::oppShoots = 0;
bool SampleCoach::weShot = false ;
bool SampleCoach::oppShot = false;

//Saves
int SampleCoach::ourSaves = 0;
int SampleCoach::oppSaves = 0;
int SampleCoach::ourSaveCycle = 0;
int SampleCoach::oppSaveCycle = 0;

//Goal Kicks
int SampleCoach::ourGoalKicks = 0;
int SampleCoach::oppGoalKicks = 0;
int SampleCoach::ourGoalKickCycle = 0 ;
int SampleCoach::oppGoalKickCycle = 0 ;

//Y_poss
int SampleCoach::posYposs = 0;
int SampleCoach::negYposs = 0;

//Third positions
int SampleCoach::ourThirdPoss = 0;
int SampleCoach::middleThirdPoss = 0;
int SampleCoach::oppThirdPoss = 0;

//Relative Possession
int SampleCoach::ourRelativePoss = 0;
int SampleCoach::oppRelativePoss = 0;

//Role Poss Our

int SampleCoach::CB_ourPoss = 0;
int SampleCoach::LB_ourPoss = 0;
int SampleCoach::RB_ourPoss = 0;
int SampleCoach::DM_ourPoss = 0;
int SampleCoach::CM_ourPoss = 0;
int SampleCoach::LM_ourPoss = 0;
int SampleCoach::RM_ourPoss = 0;
int SampleCoach::AM_ourPoss = 0;
int SampleCoach::LW_ourPoss = 0;
int SampleCoach::RW_ourPoss = 0;
int SampleCoach::CF_ourPoss = 0;

//Role Poss Opp

int SampleCoach::CB_oppPoss = 0;
int SampleCoach::LB_oppPoss = 0;
int SampleCoach::RB_oppPoss = 0;
int SampleCoach::DM_oppPoss = 0;
int SampleCoach::CM_oppPoss = 0;
int SampleCoach::LM_oppPoss = 0;
int SampleCoach::RM_oppPoss = 0;
int SampleCoach::AM_oppPoss = 0;
int SampleCoach::LW_oppPoss = 0;
int SampleCoach::RW_oppPoss = 0;
int SampleCoach::CF_oppPoss = 0;

/////////////////////////////// End of Possession Counters ///////////////////////////////////


struct RealSpeedMaxCmp
		: public std::binary_function< const PlayerType *,
		  const PlayerType *,
		  bool > {
	result_type operator()( first_argument_type lhs,
			second_argument_type rhs ) const
	{
		if ( std::fabs( lhs->realSpeedMax() - rhs->realSpeedMax() ) < 0.005 )
		{
			return lhs->cyclesToReachMaxSpeed() < rhs->cyclesToReachMaxSpeed();
		}

		return lhs->realSpeedMax() > rhs->realSpeedMax();
	}

};

/*-------------------------------------------------------------------*/
/*!

 */
SampleCoach::SampleCoach()
: CoachAgent()
{
	//
	// register audio memory & say message parsers
	//

	boost::shared_ptr< AudioMemory > audio_memory( new AudioMemory );

	M_worldmodel.setAudioMemory( audio_memory );

	addSayMessageParser( SayMessageParser::Ptr( new BallMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new PassMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new InterceptMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new GoalieMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new GoalieAndPlayerMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new OffsideLineMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new DefenseLineMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new WaitRequestMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new PassRequestMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new DribbleMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new BallGoalieMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new OnePlayerMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new TwoPlayerMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new ThreePlayerMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new SelfMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new TeammateMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new OpponentMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new BallPlayerMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new StaminaMessageParser( audio_memory ) ) );
	addSayMessageParser( SayMessageParser::Ptr( new RecoveryMessageParser( audio_memory ) ) );

	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 9 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 8 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 7 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 6 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 5 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 4 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 3 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 2 >( audio_memory ) ) );
	// addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 1 >( audio_memory ) ) );

	//
	//
	//

	for ( int i = 0; i < 11; ++i )
	{
		M_opponent_player_types[i] = Hetero_Default;
	}

}

/*-------------------------------------------------------------------*/
/*!

 */
SampleCoach::~SampleCoach()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCoach::initImpl( CmdLineParser & cmd_parser )
{
	bool result =CoachAgent::initImpl( cmd_parser );

#if 0
	ParamMap my_params;
	if ( cmd_parser.count( "help" ) )
	{
		my_params.printHelp( std::cout );
		return false;
	}
	cmd_parser.parse( my_params );
#endif

	if ( cmd_parser.failed() )
	{
		        std::cerr << "coach: ***WARNING*** detected unsupported options: ";
		        cmd_parser.print( std::cerr );
		        std::cerr << std::endl;
	}

	if ( ! result )
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////
	// Add your code here.
	//////////////////////////////////////////////////////////////////

	if ( config().useTeamGraphic() )
	{
		if ( config().teamGraphicFile().empty() )
		{
			M_team_graphic.createXpmTiles( team_logo_xpm );
		}
		else
		{
			M_team_graphic.readXpmFile( config().teamGraphicFile().c_str() );
		}
	}

	return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::actionImpl()
{
	if (world().time().cycle()>10){
		weDribbled = false ;
		oppDribbled = false ;
		weShot = false ;
		oppShot = false ;
		wePassed = false ;
		oppPassed = false ;
		if (world().gameMode().type()==GameMode::PlayOn && (cycleMemory == 2 || cycleMemory == 3 || cycleMemory == 6 || cycleMemory == 9)){
			cycleMemory = 0 ;
		}
		updateAnalyzer();
	}
	if ( world().time().cycle() == 0
			&& config().useTeamGraphic()
			&& M_team_graphic.tiles().size() != teamGraphicOKSet().size() )
	{
		sendTeamGraphic();
	}

	doSubstitute();
	sayPlayerTypes();
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::handleServerParam()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::handlePlayerParam()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::handlePlayerType()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::doSubstitute()
{
	static bool S_first_substituted = false;

	if ( ! S_first_substituted
			&& world().time().cycle() == 0
			&& world().time().stopped() > 10 )
	{
		doFirstSubstitute();
		S_first_substituted = true;

		return;
	}

	if ( world().time().cycle() > 0
			&& world().gameMode().type() != GameMode::PlayOn
			&& ! world().gameMode().isPenaltyKickMode() )
	{
		doSubstituteTiredPlayers();

		return;
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::doFirstSubstitute()
{
	PlayerTypePtrCont candidates;

	    std::fprintf( stderr,
	                  "id speed step inc  power  stam"
	//                  //" decay"
	//                  //" moment"
	//                  //" dprate"
	                  "  karea"
	//                  //"  krand"
	//                  //" effmax effmin"
	                  "\n" );

	for ( int id = 0; id < PlayerParam::i().playerTypes(); ++id )
	{
		const PlayerType * param = PlayerTypeSet::i().get( id );

		if ( ! param )
		{
			            std::cerr << config().teamName() << " coach: "
			                      << " could not get the player type " << id << std::endl;
			continue;
		}

		if ( id == Hetero_Default
				&& PlayerParam::i().allowMultDefaultType() )
		{
			for ( int i = 0; i <= MAX_PLAYER; ++i )
			{
				candidates.push_back( param );
			}
		}

		for ( int i = 0; i < PlayerParam::i().ptMax(); ++i )
		{
			candidates.push_back( param );
		}

		        std::fprintf( stderr,
		                      " %d %.3f  %2d  %.1f %5.1f %5.1f"
		                      //" %.3f"
		                      //"  %4.1f"
		                      //"  %.5f"
		                      "  %.3f"
		                      //"  %.2f"
		                      //"  %.3f  %.3f"
		                      "\n",
		                      id,
		                      param->realSpeedMax(),
		                      param->cyclesToReachMaxSpeed(),
		                      param->staminaIncMax(),
		                      param->getDashPowerToKeepMaxSpeed(),
		                      param->getOneStepStaminaComsumption(),
		                      //param->playerDecay(),
		                      //param->inertiaMoment(),
		                      //param->dashPowerRate(),
		                      param->kickableArea()
		                      //param->kickRand(),
		                      //param->effortMax(), param->effortMin()
		                      );
	}

	std::vector< int > ordered_unum;
	ordered_unum.reserve( 11 );

#if 0
	// side back has priority
	ordered_unum.push_back( 11 ); // center forward
	ordered_unum.push_back( 2 );  // center back
	ordered_unum.push_back( 3 );  // center back
	ordered_unum.push_back( 4 );  // side back
	ordered_unum.push_back( 5 );  // side back
	ordered_unum.push_back( 10 ); // side half
	ordered_unum.push_back( 9 );  // side half
	ordered_unum.push_back( 6 );  // center half
	ordered_unum.push_back( 7 );  // defensive half
	ordered_unum.push_back( 8 );  // defensive half
#else
	// wing player has priority
	ordered_unum.push_back( 11 ); // center forward
	ordered_unum.push_back( 2 );  // center back
	ordered_unum.push_back( 3 );  // center back
	ordered_unum.push_back( 10 ); // side half
	ordered_unum.push_back( 9 );  // side half
	ordered_unum.push_back( 6 );  // center half
	ordered_unum.push_back( 4 );  // side back
	ordered_unum.push_back( 5 );  // side back
	ordered_unum.push_back( 7 );  // defensive half
	ordered_unum.push_back( 8 );  // defensive half
#endif


	//
	// goalie:
	// goalie is always assigned to the default type so far.
	//

	if ( config().version() >= 14.0 )
	{
		substituteTo( 1, Hetero_Default ); // goalie
	}
	{
		PlayerTypePtrCont::iterator it = candidates.begin();
		for ( ; it != candidates.end(); ++it )
		{
			if ( (*it)->id() == Hetero_Default )
			{
				break;
			}
		}

		if ( it != candidates.end() )
		{
			candidates.erase( it );
		}
	}

	//
	// change field players
	//

	for ( std::vector< int >::iterator unum = ordered_unum.begin();
			unum != ordered_unum.end();
			++unum )
	{
		const GlobalPlayerObject * p = world().teammate( *unum );
		if ( ! p )
		{
			            std::cerr << config().teamName() << " coach: "
			                      << " teammate " << *unum << " does not exist."
			                      << " skip first substitution." << std::endl;
			            dlog.addText( Logger::TEAM,
			                          __FILE__": teammate %d does not exist. skip first substitution.",
			                          *unum );
			continue;
		}

		int type = getFastestType( candidates );
		if ( type != Hetero_Unknown )
		{
			substituteTo( *unum, type );
		}
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::doSubstituteTiredPlayers()
{
	int substitute_count = world().ourSubstituteCount();

	if ( substitute_count >= PlayerParam::i().subsMax() )
	{
		// over the maximum substitution
		return;
	}

	const ServerParam & SP = ServerParam::i();

	//
	// check game time
	//
	const int half_time = SP.actualHalfTime();
	const int normal_time = half_time * SP.nrNormalHalfs();

	if ( world().time().cycle() < normal_time - 500
			//|| world().time().cycle() <= half_time + 1
			//|| world().gameMode().type() == GameMode::KickOff_
	)
	{
		return;
	}

	    dlog.addText( Logger::TEAM,
	                  __FILE__": consider to substitute tired teammates." );

	//
	// create candidate teamamte
	//
	std::vector< int > tired_teammate_unum;

	for ( std::vector< const GlobalPlayerObject * >::const_iterator
			t = world().teammates().begin(),
			end = world().teammates().end();
			t != end;
			++t )
	{
		if ( (*t)->recovery() < ServerParam::i().recoverInit() - 0.002 )
		{
			tired_teammate_unum.push_back( (*t)->unum() );
		}
	}

	if ( tired_teammate_unum.empty() )
	{
		        dlog.addText( Logger::TEAM,
		                      __FILE__": no tired teammates." );
		return;
	}

	//
	// create candidate player type
	//
	PlayerTypePtrCont candidates;

	for ( std::vector< int >::const_iterator
			id = world().availablePlayerTypeId().begin(),
			end = world().availablePlayerTypeId().end();
			id != end;
			++id )
	{
		const PlayerType * param = PlayerTypeSet::i().get( *id );
		if ( ! param )
		{
			            std::cerr << config().teamName() << " coach: "
			                      << world().time()
			                      << " : Could not get player type. id=" << *id << std::endl;
			continue;
		}

		candidates.push_back( param );
	}

	//
	// try substitution
	//


	for ( std::vector< int >::iterator unum = tired_teammate_unum.begin();
			unum != tired_teammate_unum.end();
			++unum )
	{
		int type = getFastestType( candidates );
		if ( type != Hetero_Unknown )
		{
			substituteTo( *unum, type );
			if ( ++substitute_count >= PlayerParam::i().subsMax() )
			{
				// over the maximum substitution
				break;
			}
		}
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::substituteTo( const int unum,
		const int type )
{
	if ( world().time().cycle() > 0
			&& world().ourSubstituteCount() >= PlayerParam::i().subsMax() )
	{
		        std::cerr << "***Warning*** "
		                  << config().teamName() << " coach: over the substitution max."
		                  << " cannot change the player " << unum
		                  << " to type " << type
		                  << std::endl;
		return;
	}

	std::vector< int >::const_iterator
	it = std::find( world().availablePlayerTypeId().begin(),
			world().availablePlayerTypeId().end(),
			type );
	if ( it == world().availablePlayerTypeId().end() )
	{
		        std::cerr << "***ERROR*** "
		                  << config().teamName() << " coach: "
		                  << " cannot change the player " << unum
		                  << " to type " << type
		                  << std::endl;
		return;
	}

	doChangePlayerType( unum, type );

	    /*std::cout << config().teamName() << " coach: "
	              << "change player " << unum
	              << " to type " << type
	              << std::endl;*/
}

/*-------------------------------------------------------------------*/
/*!

 */
int
SampleCoach::getFastestType( PlayerTypePtrCont & candidates )
{
	if ( candidates.empty() )
	{
		return Hetero_Unknown;
	}

	// sort by max speed
	std::sort( candidates.begin(),
			candidates.end(),
			RealSpeedMaxCmp() );

	//     std::cerr << "getFastestType candidate = ";
	//     for ( PlayerTypePtrCont::iterator it = candidates.begin();
	//           it != candidates.end();
	//           ++it )
	//     {Vector2D teamTpos[11];
	//         std::cerr << (*it)->id() << ' ';
	//     }
	//     std::cerr << std::endl;

	PlayerTypePtrCont::iterator best_type = candidates.end();
	double max_speed = 0.0;
	int min_cycle = 100;
	for ( PlayerTypePtrCont::iterator it = candidates.begin();
			it != candidates.end();
			++it )
	{
		if ( (*it)->realSpeedMax() < max_speed - 0.01 )
		{
			break;
		}

		if ( (*it)->cyclesToReachMaxSpeed() < min_cycle )
		{
			best_type = it;
			max_speed = (*best_type)->realSpeedMax();
			min_cycle = (*best_type)->cyclesToReachMaxSpeed();
			continue;
		}

		if ( (*it)->cyclesToReachMaxSpeed() == min_cycle )
		{
			if ( (*it)->getOneStepStaminaComsumption()
					< (*best_type)->getOneStepStaminaComsumption() )
			{
				best_type = it;
				max_speed = (*best_type)->realSpeedMax();
			}
		}
	}

	if ( best_type != candidates.end() )
	{
		int id = (*best_type)->id();
		candidates.erase( best_type );
		return id;
	}

	return Hetero_Unknown;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::sayPlayerTypes()
{
	/*
      format:
      "(player_types (1 0) (2 1) (3 2) (4 3) (5 4) (6 5) (7 6) (8 -1) (9 0) (10 1) (11 2))"
      ->
      (say (freeform "(player_type ...)"))
	 */

	static GameTime s_last_send_time( 0, 0 );

	if ( ! config().useFreeform() )
	{
		return;
	}

	if ( ! world().canSendFreeform() )
	{
		return;
	}

	int analyzed_count = 0;

	for ( int unum = 1; unum <= 11; ++unum )
	{
		const int id = world().theirPlayerTypeId( unum );

		if ( id != M_opponent_player_types[unum - 1] )
		{
			//            M_opponent_player_types[unum - 1] = id;

			if ( id != Hetero_Unknown )
			{
				++analyzed_count;
			}
		}
	}

	if ( analyzed_count == 0 )
	{
		return;
	}

	std::string msg;
	msg.reserve( 128 );

	msg = "(player_types ";

	for ( int unum = 1; unum <= 11; ++unum )
	{
		char buf[8];
		        snprintf( buf, 8, "(%d %d)",
		                  unum, M_opponent_player_types[unum - 1] );
		msg += buf;
	}

	msg += ")";

	doSayFreeform( msg );

	s_last_send_time = world().time();

	    /*std::cout << config().teamName()
	              << " coach: "
	              << world().time()
	              << " sent freeform " << msg
	              << std::endl;*/
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::sendTeamGraphic()
{
	int count = 0;
	for ( TeamGraphic::Map::const_reverse_iterator tile = M_team_graphic.tiles().rbegin();
			tile != M_team_graphic.tiles().rend();
			++tile )
	{
		if ( teamGraphicOKSet().find( tile->first ) == teamGraphicOKSet().end() )
		{
			if ( ! doTeamGraphic( tile->first.first,
					tile->first.second,
					M_team_graphic ) )
			{
				break;
			}
			++count;
		}
	}

	if ( count > 0 )
	{
		        /*std::cout << config().teamName()
		                  << " coach: "
		                  << world().time()
		                  << " send team_graphic " << count << " tiles"
		                  << std::endl;*/
	}
}
void SampleCoach::updateGoals() {
	ourGoals = (
			world().ourSide() == LEFT ?
					world().gameMode().scoreLeft() : world().gameMode().scoreRight());
	oppGoals = (
			world().ourSide() == LEFT ?
					world().gameMode().scoreRight() : world().gameMode().scoreLeft());
	goalDiff = ourGoals - oppGoals;
	return;
}
void SampleCoach::updateCorners() {
	if (world().ourSide() == LEFT && world().gameMode().type() == GameMode::CornerKick_) {
		if (world().ball().pos().y < 0 && world().ball().pos().x > 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			ourCornerLeft++;
			return;
		}
		if (world().ball().pos().y >= 0 && world().ball().pos().x > 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			ourCornerRight++;
			return;
		}
		if (world().ball().pos().y < 0 && world().ball().pos().x < 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			oppCornerRight++;
			return;
		}
		if (world().ball().pos().y >= 0 && world().ball().pos().x < 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			oppCornerLeft++;
			return;
		}
	} else if (world().ourSide() == RIGHT
			&& world().gameMode().type() == GameMode::CornerKick_) {
		if (world().ball().pos().y < 0 && world().ball().pos().x > 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			oppCornerLeft++;
			return;
		}
		if (world().ball().pos().y >= 0 && world().ball().pos().x > 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			oppCornerRight++;
			return;
		}
		if (world().ball().pos().y < 0 && world().ball().pos().x < 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			ourCornerRight++;
			return;
		}
		if (world().ball().pos().y >= 0 && world().ball().pos().x < 0
				&& cycleMemory != 2) {
			cycleMemory = 2;
			cycleMemoryChanged = true ;
			ourCornerLeft++;
			return;
		}
	}
	return;
}
void SampleCoach::updateFoulsAfreeKicks() {
	if (world().gameMode().isOurSetPlay(world().ourSide())
			&& (world().gameMode().type() == GameMode::FoulCharge_ || world().gameMode().type() == GameMode::FoulPush_ || world().gameMode().type() == GameMode::FoulBallOut_ || world().gameMode().type() == GameMode::FoulMultipleAttacker_||world().gameMode().type()==GameMode::BackPass_||world().gameMode().type()==GameMode::FreeKick_)
			&& cycleMemory != 3) {
		int	wc = world().time().cycle()-ourFreeKickCycle ;
		if (wc != 1 && wc != 0){
			freeKickPoint = world().ball().pos();
			ourFreeKick++;
			oppFouls++;
			ourFreeKickCycle = world().time().cycle() ;
		}
		else {
			ourFreeKickCycle = world().time().cycle() ;
		}

		cycleMemory = 3;
		passKicker = 0 ;
		passKicker2 = 0 ;
		cycleMemoryChanged = true ;
		return;
	}
	else if (!world().gameMode().isOurSetPlay(world().ourSide())
			&& (world().gameMode().type() == GameMode::FoulCharge_ || world().gameMode().type() == GameMode::FoulPush_ || world().gameMode().type() == GameMode::FoulBallOut_ || world().gameMode().type() == GameMode::FoulMultipleAttacker_||world().gameMode().type()==GameMode::BackPass_||world().gameMode().type()==GameMode::FreeKick_)
			&& cycleMemory != 3) {
		int	wc2 = world().time().cycle()-oppFreeKickCycle ;
		if (wc2 != 1 && wc2 != 0){
			freeKickPoint = world().ball().pos();
			oppFreeKick++;
			ourFouls++;
			oppFreeKickCycle = world().time().cycle() ;
		}
		else {
			oppFreeKickCycle = world().time().cycle() ;
		}
		cycleMemory = 3;
		passKicker = 0 ;
		passKicker2 = 0 ;
		cycleMemoryChanged = true ;
		return;
	}
	return;
}
void SampleCoach::updateYellowCards(){
	if (world().time().cycle()==5998 ){
		//check for team mates
		if(world().isTeammateYellowCarded(1)) ourYellowCards++;
		if(world().isTeammateYellowCarded(2)) ourYellowCards++;
		if(world().isTeammateYellowCarded(3)) ourYellowCards++;
		if(world().isTeammateYellowCarded(4)) ourYellowCards++;
		if(world().isTeammateYellowCarded(5)) ourYellowCards++;
		if(world().isTeammateYellowCarded(6)) ourYellowCards++;
		if(world().isTeammateYellowCarded(7)) ourYellowCards++;
		if(world().isTeammateYellowCarded(8)) ourYellowCards++;
		if(world().isTeammateYellowCarded(9)) ourYellowCards++;
		if(world().isTeammateYellowCarded(10)) ourYellowCards++;
		if(world().isTeammateYellowCarded(11)) ourYellowCards++;
		//check for opponents
		if(world().isOpponentYellowarded(1)) oppYellowCards++;
		if(world().isOpponentYellowarded(2)) oppYellowCards++;
		if(world().isOpponentYellowarded(3)) oppYellowCards++;
		if(world().isOpponentYellowarded(4)) oppYellowCards++;
		if(world().isOpponentYellowarded(5)) oppYellowCards++;
		if(world().isOpponentYellowarded(6)) oppYellowCards++;
		if(world().isOpponentYellowarded(7)) oppYellowCards++;
		if(world().isOpponentYellowarded(8)) oppYellowCards++;
		if(world().isOpponentYellowarded(9)) oppYellowCards++;
		if(world().isOpponentYellowarded(10)) oppYellowCards++;
		if(world().isOpponentYellowarded(11)) oppYellowCards++;
	}
}
void SampleCoach::updateKickIns() {
	if (kickable==true && world().gameMode().type() == GameMode::KickIn_ && cycleMemory != 4 ) {
		cycleMemory = 4;
		cycleMemoryChanged = true ;
		ourKickIns++;
		passKicker = 0 ;
		passKicker2 = 0 ;
		return;
	} else if (oppKickable==true  && world().gameMode().type() == GameMode::KickIn_ && cycleMemory != 4) {
		cycleMemory = 4;
		cycleMemoryChanged = true ;
		oppKickIns++;
		passKicker = 0 ;
		passKicker2 = 0 ;
		return;
	}
	return;
}
void SampleCoach::updateTackles() {
	//check for team mates
	int wc ;
	if(world().gameMode().type()==GameMode::FoulCharge_ && oppKickable){
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 && wc != 0){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(1)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(2)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(3)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(4)->isTackling()){
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(5)->isTackling()){
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(6)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(7)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(8)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(9)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(10)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	if(world().teammate(11)->isTackling()) {
		wc = world().time().cycle()-ourTackleCycle ;
		if (wc != 1 ){
			ourTackle++;
			ourTackleCycle = world().time().cycle() ;
		}
		else {
			ourTackleCycle = world().time().cycle() ;
		}
	}
	//check for opponents
	int wc2 ;
	if(world().gameMode().type()==GameMode::FoulCharge_ && kickable){
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 && wc2 !=0){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(1)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(2)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(3)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(4)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(5)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(6)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(7)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(8)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(9)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(10)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	if(world().opponent(11)->isTackling()) {
		wc2 = world().time().cycle()-oppTackleCycle ;
		if (wc2 != 1 ){
			oppTackle++;
			oppTackleCycle = world().time().cycle() ;
		}
		else {
			oppTackleCycle = world().time().cycle() ;
		}
	}
	return;
}
void SampleCoach::updateShoots() {
	Vector2D nowBall = world().ball().pos();
	Vector2D newBall = world().ball().pos()+world().ball().vel();
	Line2D shotPoint = Line2D(world().ball().pos(),world().ball().pos() + world().ball().vel());
	Vector2D rightOur = Vector2D(52.5, 8.0);
	Vector2D rightOpp = Vector2D(-52.5, -8.0);
	Vector2D leftOur = Vector2D(52.5, -8.0);
	Vector2D leftOpp = Vector2D(-52.5, 8.0);
	Line2D oppGoalLine = Line2D(rightOur, leftOur);
	Line2D ourGoalLine = Line2D(rightOpp, leftOpp);
	Vector2D ourFinalTarget = shotPoint.intersection(oppGoalLine);
	Vector2D oppFinalTarget = shotPoint.intersection(ourGoalLine);
	bool isInOppGoal;
	bool isInOurGoal;
	if (ourFinalTarget.absY() < 8.0 && newBall.x>nowBall.x){
		isInOppGoal = true;
	}
	else
		isInOppGoal = false;
	if (oppFinalTarget.absY() < 8.0 && newBall.x<nowBall.x){
		isInOurGoal = true;
	}
	else
		isInOurGoal = false;

	if (world().gameMode().type() == GameMode::PlayOn && cycleMemory != 7
			&& world().ball().vel().r() > 2.0 && isInOppGoal && world().ball().pos().x > 30
			&& world().ball().pos().absY() < 20 && weKickedLast){
		cycleMemory = 7;
		cycleMemoryChanged = true ;
		weShot = true ;
		ourShoots++;
		return;
	} else if (world().gameMode().type() == GameMode::PlayOn && cycleMemory != 7
			&& world().ball().vel().r() > 2.0 && isInOurGoal && world().ball().pos().x < -30
			&& world().ball().pos().absY() < 20 && oppKickedLast){
		cycleMemory = 7;
		cycleMemoryChanged = true ;
		oppShot = true ;
		oppShoots++;
		return;
	}
	return;
}
void SampleCoach::updateSaves() {
	if (world().gameMode().side()==world().ourSide()
			&& world().gameMode().type() == GameMode::GoalieCatch_
			&& cycleMemory != 8) {
		int	wc = world().time().cycle()-ourSaveCycle ;
		if (wc > 50){
			ourSaves++;
			ourSaveCycle = world().time().cycle() ;
		}
		else {
			ourSaveCycle = world().time().cycle() ;
		}

		cycleMemory = 8;
		cycleMemoryChanged = true ;
		return;
	} else if (world().gameMode().side()==world().theirSide()
			&& world().gameMode().type() == GameMode::GoalieCatch_
			&& cycleMemory != 8) {
		int	wc2 = world().time().cycle()-oppSaveCycle ;
		if (wc2 >50){
			oppSaves++;
			oppSaveCycle = world().time().cycle() ;
		}
		else {
			oppSaveCycle = world().time().cycle() ;
		}

		cycleMemory = 8;
		cycleMemoryChanged = true ;
		return;
	}
	return;
}
void SampleCoach::updateGoalKicks() {
	if ( world().gameMode().type ()== GameMode::GoalKick_
			&& world().gameMode().side() == world().ourSide() && cycleMemory != 9) {
		int	wc = world().time().cycle()-ourGoalKickCycle ;
				if (wc > 50){
					ourGoalKicks++;
					ourGoalKickCycle = world().time().cycle() ;
				}
				else {
					ourGoalKickCycle = world().time().cycle() ;
				}

				cycleMemory = 9;
				cycleMemoryChanged = true ;
				return;
			} else if (world().gameMode().side()==world().theirSide()
					&& world().gameMode().type() == GameMode::GoalKick_
					&& cycleMemory != 9) {
				int	wc2 = world().time().cycle()-oppGoalKickCycle ;
				if (wc2 >50){
					oppGoalKicks++;
					oppGoalKickCycle = world().time().cycle() ;
				}
				else {
					oppGoalKickCycle = world().time().cycle() ;
				}

				cycleMemory = 9;
				cycleMemoryChanged = true ;
				return;
			}
			return;
}
		void SampleCoach::updateYposs() {
	if (world().gameMode().type() == GameMode::PlayOn && world().ball().pos().y > 0) {
		posYposs++;
		return;
	} else if (world().gameMode().type() == GameMode::PlayOn
			&& world().ball().pos().y <= 0) {
		negYposs++;
		return;
	}
	return;
}
void SampleCoach::updateThirdPoss() {
	if (world().gameMode().type() == GameMode::PlayOn && world().ball().pos().x < -17.5) {
		ourThirdPoss++;
		return;
	}
	else if (world().gameMode().type() == GameMode::PlayOn
			&& world().ball().pos().x >= -17.5 && world().ball().pos().x <= 17.5) {
		middleThirdPoss++;
		return;
	} else if (world().gameMode().type() == GameMode::PlayOn
			&& world().ball().pos().x > 17.5) {
		oppThirdPoss++;
		return;
	}
	return;
}
void SampleCoach::updateRoles() {
	if (world().gameMode().type() == GameMode::PlayOn
			&& owner==true) {
		if (world().ball().pos().x >= -55 && world().ball().pos().x <= -20
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CB_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= -55 && world().ball().pos().x <= -15
				&& world().ball().pos().y >= -35 && world().ball().pos().y <= -20) {
			LB_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= -55 && world().ball().pos().x <= -15
				&& world().ball().pos().y >= 20 && world().ball().pos().y <= 35) {
			RB_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= -20 && world().ball().pos().x <= 0
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			DM_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= 0 && world().ball().pos().x <= 15
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CM_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= 15 && world().ball().pos().x <= 35
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			AM_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= -15 && world().ball().pos().x <= 20
				&& world().ball().pos().y >= -35 && world().ball().pos().y <= -20) {
			LM_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= -15 && world().ball().pos().x <= 20
				&& world().ball().pos().y >= 20 && world().ball().pos().y <= 35) {
			RM_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= 20 && world().ball().pos().x <= 55
				&& world().ball().pos().y >= 20 && world().ball().pos().y <= 35) {
			RW_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= 20 && world().ball().pos().x <= 55
				&& world().ball().pos().y >= -35 && world().ball().pos().y <= -20) {
			LW_ourPoss++;
			return;
		}
		if (world().ball().pos().x >= 35 && world().ball().pos().x <= 55
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CF_ourPoss++;
			return;
		}
		return;
	}
	else if (world().gameMode().type() == GameMode::PlayOn
			&& owner==false) {
		if (world().ball().pos().x <= 55 && world().ball().pos().x >= 20
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CB_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 55 && world().ball().pos().x >= 15
				&& world().ball().pos().y >= -35 && world().ball().pos().y <= -20) {
			RB_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 55 && world().ball().pos().x >= 15
				&& world().ball().pos().y >= 20 && world().ball().pos().y <= 35) {
			LB_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 20 && world().ball().pos().x >= 0
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			DM_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 0 && world().ball().pos().x >= -15
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CM_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= -15 && world().ball().pos().x >= -35
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			AM_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 15 && world().ball().pos().x >= -20
				&& world().ball().pos().y <= 35 && world().ball().pos().y >= 20) {
			LM_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= 15 && world().ball().pos().x >= -20
				&& world().ball().pos().y <= -20 && world().ball().pos().y >= -35) {
			RM_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= -20 && world().ball().pos().x  >= -55
				&& world().ball().pos().y <= -20 && world().ball().pos().y >= -35) {
			LW_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= -20 && world().ball().pos().x >= -55
				&& world().ball().pos().y <= 35 && world().ball().pos().y >= 20) {
			RW_oppPoss++;
			return;
		}
		if (world().ball().pos().x <= -35 && world().ball().pos().x >= -55
				&& world().ball().pos().y >= -20 && world().ball().pos().y <= 20) {
			CF_oppPoss++;
			return;
		}
		return;
	}
}
void SampleCoach::updateRelativePoss() {
	if (world().gameMode().type() == GameMode::PlayOn) {
		playOnPoss++;
		if (owner==true) {
			if (world().time().cycle()==0) {
				ourPoss = 0 ;
				ourRelativePoss = 0;
				oppRelativePoss = 0;
				return ;
			}
			ourPoss++;
			ourRelativePoss = int(double((double(ourPoss) / double(playOnPoss))) * 100);
			oppRelativePoss = 100 - ourRelativePoss ;
			return;
		} else if (owner==false) {
			if (world().time().cycle()==0) {
				ourPoss = 0 ;
				ourRelativePoss = 0;
				oppRelativePoss = 0;
				return ;
			}
			theirPoss++;
			ourRelativePoss = int((double(double(ourPoss) / double(playOnPoss))) * 100);
			oppRelativePoss = 100-ourRelativePoss;
			return;
		}
		return;
	}
	return;
}
void SampleCoach::updateMatchFacts(){
	if (world().time().cycle()==2999 ||world().time().cycle()==5999 ){
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************Friendly Match**********************************"<<endl;
		cout<<"****************************                          ****************************"<<endl;
		cout<<"***  "<<world().ourTeamName()<<"  ***                         Vs                          "<<world().theirTeamName()<<" ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourGoals<<"    ***"<<"                        Goals                       "<<"***    "<<oppGoals<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***  % "<<ourRelativePoss<<"   ***"<<"                      Possession                    "<<"***  % "<<oppRelativePoss<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourShoots<<"    ***"<<"                        Shoots                      "<<"***    "<<oppShoots<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***   "<<ourPasses<<"   ***"<<"                        Passes                      "<<"***   "<<oppPasses<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***  % "<<ourPassesPercentage<<"   ***"<<"                      pass Accuracy                 "<<"***  % "<<oppPassesPercentage<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourSaves<<"    ***"<<"                        Saves                       "<<"***    "<<oppSaves<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourTackle<<"   ***"<<"                       Tackles                      "<<"***    "<<oppTackle<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourFouls<<"   ***"<<"                        Fouls                       "<<"***    "<<oppFouls<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourFreeKick<<"   ***"<<"                      FreeKicks                     "<<"***    "<<oppFreeKick<<"   ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourYellowCards<<"    ***"<<"                   Yellow Cards                     "<<"***    "<<oppYellowCards<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourCornerLeft+ourCornerRight<<"    ***"<<"                      Corners                       "<<"***    "<<oppCornerLeft+oppCornerRight<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourKickIns<<"    ***"<<"                      KickIns                       "<<"***    "<<oppKickIns<<"    ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<ourGoalKicks<<"    ***"<<"                     Goal Kicks                     "<<"***    "<<oppGoalKicks<<"    ***"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"*********************************Field Possession*********************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"******************************=======================*****************************"<<endl;
		cout<<"*****************************|       |       |       |****************************"<<endl;
		cout<<"*****************************| % "<<int((double(double(ourThirdPoss) / double(playOnPoss))) * 100)<<"  | % "<<int((double(double(middleThirdPoss) / double(playOnPoss))) * 100)<<"  | % "<<100-int((double(double(ourThirdPoss) / double(playOnPoss))) * 100)-int((double(double(middleThirdPoss) / double(playOnPoss))) * 100)<<"  |****************************"<<endl;
		cout<<"*****************************|       |       |       |****************************"<<endl;
		cout<<"******************************=======================*****************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"********************************Players Possession********************************"<<endl;
		cout<<"****************************                          ****************************"<<endl;
		cout<<"***  "<<world().ourTeamName()<<"                              Vs                          "<<world().theirTeamName()<<" ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(CB_ourPoss) / double(playOnPoss))) * 100)<<"                                CB                               "<<int((double(double(CB_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(LB_ourPoss) / double(playOnPoss))) * 100)<<"                                LB                               "<<int((double(double(LB_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(RB_ourPoss) / double(playOnPoss))) * 100)<<"                                RB                               "<<int((double(double(RB_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(DM_ourPoss) / double(playOnPoss))) * 100)<<"                                DM                               "<<int((double(double(DM_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(CM_ourPoss) / double(playOnPoss))) * 100)<<"                                CM                               "<<int((double(double(CM_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(LM_ourPoss) / double(playOnPoss))) * 100)<<"                               LM                              "<<int((double(double(LM_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(RM_ourPoss) / double(playOnPoss))) * 100)<<"                                RM                               "<<int((double(double(RM_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(AM_ourPoss) / double(playOnPoss))) * 100)<<"                                AM                               "<<int((double(double(AM_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(CF_ourPoss) / double(playOnPoss))) * 100)<<"                                CF                               "<<int((double(double(CF_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(LW_ourPoss) / double(playOnPoss))) * 100)<<"                                LW                               "<<int((double(double(LW_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"----------------------------------------------------------------------------------"<<endl;
		cout<<"***    "<<int((double(double(RW_ourPoss) / double(playOnPoss))) * 100)<<"                                RW                               "<<int((double(double(RW_oppPoss) / double(playOnPoss))) * 100)<<"     ***"<<endl;
		cout<<"**********************************************************************************"<<endl;
		cout<<"**********************************************************************************"<<endl;
		return ;
	}
}
void SampleCoach::oppIsKickable(){
	double minDist2BallOpp = 1000.0 ;
	//
	//	/////////////// finding the distance of the opponent nearest player to ball ///////////////////
	//
	if (world().opponent(1)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -1 ;
		minDist2BallOpp = world().opponent(1)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(2)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -2 ;
		minDist2BallOpp = world().opponent(2)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(3)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -3;
		minDist2BallOpp = world().opponent(3)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(4)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -4 ;
		minDist2BallOpp = world().opponent(4)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(5)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -5 ;
		minDist2BallOpp = world().opponent(5)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(6)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -6 ;
		minDist2BallOpp = world().opponent(6)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(7)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -7 ;
		minDist2BallOpp = world().opponent(7)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(8)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -8 ;
		minDist2BallOpp = world().opponent(8)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(9)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -9 ;
		minDist2BallOpp = world().opponent(9)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(10)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -10 ;
		minDist2BallOpp = world().opponent(10)->pos().dist(world().ball().pos()) ;
	}
	if (world().opponent(11)->pos().dist(world().ball().pos())<minDist2BallOpp) {
		kickableNum = -11 ;
		minDist2BallOpp = world().opponent(11)->pos().dist(world().ball().pos()) ;
	}
	if (minDist2BallOpp <= 1.1 && world().existKickablePlayer()) {
		oppKickable = true ;
		kickable = false ;
		return ;
	}
	else {
		oppKickable = false ;
		return ;
	}
	return ;
}
void SampleCoach::weAreKickable(){
	double minDist2BallOur = 1000.0 ;
	if (oppKickable) {
		kickable = false ;
		return ;
	}
	/////////////// finding the distance of the our nearest player to ball ///////////////////
	if (world().teammate(1)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 1 ;
		minDist2BallOur = world().teammate(1)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(2)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 2 ;
		minDist2BallOur = world().teammate(2)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(3)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 3 ;
		minDist2BallOur = world().teammate(3)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(4)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 4 ;
		minDist2BallOur = world().teammate(4)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(5)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 5 ;
		minDist2BallOur = world().teammate(5)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(6)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 6 ;
		minDist2BallOur = world().teammate(6)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(7)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 7 ;
		minDist2BallOur = world().teammate(7)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(8)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 8 ;
		minDist2BallOur = world().teammate(8)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(9)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 9 ;
		minDist2BallOur = world().teammate(9)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(10)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 10 ;
		minDist2BallOur = world().teammate(10)->pos().dist(world().ball().pos()) ;
	}
	if (world().teammate(11)->pos().dist(world().ball().pos())<minDist2BallOur) {
		kickableNum = 11 ;
		minDist2BallOur = world().teammate(11)->pos().dist(world().ball().pos()) ;
	}
	if (minDist2BallOur <=1.1 && world().existKickablePlayer()) {
		kickable = true ;
		oppKickable = false ;
		return ;
	}
	kickable = false ;
	return ;
}
void SampleCoach::findBallOwner(){
	if (kickable) {
		owner = true ;
		return ;
	}
	if (oppKickable) {
		owner = false ;
		return ;
	}
	int cycle =0;
	Vector2D ballVel = world().ball().vel();
	Vector2D ball = world().ball().pos();
	Vector2D teamTpos[11];
	teamTpos[0]= world().teammate(1)->pos();
	teamTpos[1]= world().teammate(2)->pos();
	teamTpos[2]= world().teammate(3)->pos();
	teamTpos[3]= world().teammate(4)->pos();
	teamTpos[4]= world().teammate(5)->pos();
	teamTpos[5]= world().teammate(6)->pos();
	teamTpos[6]= world().teammate(7)->pos();
	teamTpos[7]= world().teammate(8)->pos();
	teamTpos[8]= world().teammate(9)->pos();
	teamTpos[9]= world().teammate(10)->pos();
	teamTpos[10]= world().teammate(11)->pos();
	Vector2D oppTpos[11];
	oppTpos[0] = world().opponent(1)->pos();
	oppTpos[1] = world().opponent(2)->pos();
	oppTpos[2] = world().opponent(3)->pos();
	oppTpos[3] = world().opponent(4)->pos();
	oppTpos[4] = world().opponent(5)->pos();
	oppTpos[5] = world().opponent(6)->pos();
	oppTpos[6] = world().opponent(7)->pos();
	oppTpos[7] = world().opponent(8)->pos();
	oppTpos[8] = world().opponent(9)->pos();
	oppTpos[9] = world().opponent(10)->pos();
	oppTpos[10]= world().opponent(11)->pos();
	double teamDist2Ball[11];
	teamDist2Ball[0]= world().teammate(1)->pos().dist(world().ball().pos());
	teamDist2Ball[1]= world().teammate(2)->pos().dist(world().ball().pos());
	teamDist2Ball[2]= world().teammate(3)->pos().dist(world().ball().pos());
	teamDist2Ball[3]= world().teammate(4)->pos().dist(world().ball().pos());
	teamDist2Ball[4]= world().teammate(5)->pos().dist(world().ball().pos());
	teamDist2Ball[5]= world().teammate(6)->pos().dist(world().ball().pos());
	teamDist2Ball[6]= world().teammate(7)->pos().dist(world().ball().pos());
	teamDist2Ball[7]= world().teammate(8)->pos().dist(world().ball().pos());
	teamDist2Ball[8]= world().teammate(9)->pos().dist(world().ball().pos());
	teamDist2Ball[9]= world().teammate(10)->pos().dist(world().ball().pos());
	teamDist2Ball[10]= world().teammate(11)->pos().dist(world().ball().pos());
	double oppDist2Ball[11];
	oppDist2Ball[0]=world().opponent(1)->pos().dist(world().ball().pos());
	oppDist2Ball[1]=world().opponent(2)->pos().dist(world().ball().pos());
	oppDist2Ball[2]=world().opponent(3)->pos().dist(world().ball().pos());
	oppDist2Ball[3]=world().opponent(4)->pos().dist(world().ball().pos());
	oppDist2Ball[4]=world().opponent(5)->pos().dist(world().ball().pos());
	oppDist2Ball[5]=world().opponent(6)->pos().dist(world().ball().pos());
	oppDist2Ball[6]=world().opponent(7)->pos().dist(world().ball().pos());
	oppDist2Ball[7]=world().opponent(8)->pos().dist(world().ball().pos());
	oppDist2Ball[8]=world().opponent(9)->pos().dist(world().ball().pos());
	oppDist2Ball[9]=world().opponent(10)->pos().dist(world().ball().pos());
	oppDist2Ball[10]=world().opponent(11)->pos().dist(world().ball().pos());
	while (true){
		cycle++;
		ballVel = ballVel*0.94 ;
		ball = ball+ballVel;
		if (ball.absX()>52.4 && ballVel.r()>0.1){
			if (weKickedLast){
				owner = false ;
				return ;
			}
			else if (oppKickedLast){
				owner = true ;
				return ;
			}
		}
		else if (ball.absY()>33.9 && ballVel.r()>0.1){
			if (weKickedLast){
				owner = false ;
				return ;
			}
			else if (oppKickedLast){
				owner = true ;
				return ;
			}
		}
		for (int i=0;i<11;i++){
			teamDist2Ball[i]= teamTpos[i].dist(ball);
			oppDist2Ball[i]=oppTpos[i].dist(ball);
			if (teamDist2Ball[i]<cycle ){
				owner = true ;
				return ;
			}
			else if (oppDist2Ball[i]<cycle) {
				owner = false ;
				return ;
			}
		}
	}
	owner = false ;
	return ;
}
void SampleCoach::findLastKicker (){
	if (world().existKickablePlayer()){
		/////////////// finding our last kicker (if exists now) ///////////////////
		if (world().teammate(1)->kicked() || kickableNum == 1) {
			weKickedLast = true ;
			oppKickedLast = false ;
			kicker2 = kicker ;
			kicker = 1 ;
			return;
		}
		if (world().teammate(2)->kicked() || kickableNum == 2) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 2 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(3)->kicked() || kickableNum == 3) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 3 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(4)->kicked() || kickableNum == 4) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 4 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(5)->kicked() || kickableNum == 5) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 5 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(6)->kicked() || kickableNum == 6) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 6 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(7)->kicked() || kickableNum == 7) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 7 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(8)->kicked() || kickableNum == 8) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 8 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(9)->kicked() || kickableNum == 9) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 9 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(10)->kicked() || kickableNum == 10) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 10 ;
			oppKickedLast = false ;
			return;
		}
		if (world().teammate(11)->kicked() || kickableNum == 11) {
			weKickedLast = true ;
			kicker2 = kicker ;
			kicker = 11 ;
			oppKickedLast = false ;
			return;
		}
		/////////////// finding opponent last kicker (if exists now) ///////////////////
		if (world().opponent(1)->kicked() || kickableNum == -1) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -1 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(2)->kicked() || kickableNum == -2) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -2 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(3)->kicked() || kickableNum == -3) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -3 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(4)->kicked() || kickableNum == -4) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -4 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(5)->kicked() || kickableNum == -5) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -5 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(6)->kicked() || kickableNum == -6) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -6 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(7)->kicked() || kickableNum == -7) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -7 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(8)->kicked() || kickableNum == -8) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -8 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(9)->kicked() || kickableNum == -9) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -9 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(10)->kicked() || kickableNum == -10) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -10 ;
			weKickedLast = false ;
			return;
		}
		if (world().opponent(11)->kicked() || kickableNum == -11) {
			oppKickedLast = true ;
			kicker2 = kicker ;
			kicker = -11 ;
			weKickedLast = false ;
			return;
		}
	}
	return ;
}
void SampleCoach::updatePasses (){
	//////////////////////////////////////
	if (kicker == 0 || kicker2 == 0) return ;
	if (kicker == kicker2) return ;
	if (kicker == passKicker || kicker2 == passKicker2) return ;
	if ( kicker2 > 0 && kicker < 0 && !weShot){
		ourPasses++;
		passKicker = kicker ;
		passKicker2 = kicker2 ;
		ourPassesPercentage = int((double(double(ourCorrectPasses) / double(ourPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = true ;
		oppPassed = false;
		return ;
	}
	else if (kicker2 > 0 && (world().gameMode().type() == GameMode::KickIn_ || world().gameMode().type() == GameMode::CornerKick_||world().gameMode().type() == GameMode::GoalKick_)&& !owner && !oppDribbled && !weDribbled){
		ourPasses++;
		ourPassesPercentage = int((double(double(ourCorrectPasses) / double(ourPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = true ;
		oppPassed =false ;
		return ;
	}
	else if (kicker2 > 0 && (world().gameMode().type() == GameMode::KickIn_ || world().gameMode().type() == GameMode::CornerKick_||world().gameMode().type() == GameMode::GoalKick_)&& owner && !oppDribbled && !weDribbled){
		ourPasses++;
		ourPassesPercentage = int((double(double(ourCorrectPasses) / double(ourPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = true ;
		oppPassed =false ;
		return ;
	}
	else if ( kicker2 > 0  && kicker > 0 ){//&& !oppDribbled && !weDribbled){
		ourPasses++;
		ourCorrectPasses++;
		passKicker = kicker ;
		passKicker2 = kicker2 ;
		ourPassesPercentage = int((double(double(ourCorrectPasses) / double(ourPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = true ;
		oppPassed = false ;
		return ;

	}
	else if ( kicker2 < 0 && kicker > 0 && kickable && !oppShot && !oppDribbled && !weDribbled){
		oppPasses++;
		passKicker = kicker ;
		passKicker2 = kicker2 ;
		oppPassesPercentage = int((double(double(oppCorrectPasses) / double(oppPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = false ;
		oppPassed = true ;
		return ;
	}
	else if (kicker2 < 0 && (world().gameMode().type() == GameMode::KickIn_ || world().gameMode().type() == GameMode::CornerKick_||world().gameMode().type() == GameMode::GoalKick_) && owner && !oppShot && !oppDribbled && !weDribbled){
		oppPasses++;
		oppPassesPercentage = int((double(double(oppCorrectPasses) / double(oppPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = false ;
		oppPassed = true ;
		return ;
	}
	else if (kicker2 < 0 && (world().gameMode().type() == GameMode::KickIn_ || world().gameMode().type() == GameMode::CornerKick_||world().gameMode().type() == GameMode::GoalKick_ )&& !owner && !oppShot && !oppDribbled && !weDribbled){
		oppPasses++;
		oppPassesPercentage = int((double(double(oppCorrectPasses) / double(oppPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = false ;
		oppPassed = true ;
		return ;
	}
	else if ( kicker < 0 && kicker2 < 0 ){//&& !oppDribbled ){
		oppPasses++;
		oppCorrectPasses++;
		passKicker = kicker ;
		passKicker2 = kicker2 ;
		oppPassesPercentage = int((double(double(oppCorrectPasses) / double(oppPasses))) * 100);
		cycleMemory = 10 ;
		cycleMemoryChanged = true ;
		wePassed = false ;
		oppPassed = true ;
		return ;

	}
	return ;

}
void SampleCoach::updateDribbles () {
	if (world().gameMode().type()!= GameMode::PlayOn) return ;
	if (kicker2 > 0 && kickable && kicker == kicker2 && !weShot && !wePassed){
		ourDribbles++;
		cycleMemory = 11 ;
		cycleMemoryChanged = true ;
		weDribbled = true ;
		return ;
	}
	else if (kicker2 < 0 && oppKickable && kicker == kicker2 && !oppShot && !oppPassed){
		oppDribbled++;
		cycleMemory = 11 ;
		cycleMemoryChanged = true ;
		oppDribbled = true ;
		return ;
	}
	return ;
}
void SampleCoach::updateAnalyzer() {
//	cout<<"*************** Cycle ::::::: "<<world().time().cycle()<<"  **************"<<endl;
	cycleMemoryChanged = false ;
	oppIsKickable();
	weAreKickable();
	findLastKicker();
	findBallOwner();
	updateRelativePoss();
	updateThirdPoss();
	updateRoles();
	updateYellowCards();
	updateGoals();
	updateMatchFacts();
	if (cycleMemoryChanged == true && cycleMemory == 1) return ;
	updateCorners();
	if (cycleMemoryChanged == true && cycleMemory == 2) return ;
	updateGoalKicks();
	if (cycleMemoryChanged == true && cycleMemory == 9) return ;
	updateKickIns();
	if (cycleMemoryChanged == true && cycleMemory == 4) return ;
	updateFoulsAfreeKicks();
	if (cycleMemoryChanged == true && cycleMemory == 3) return ;
	if (cycleMemoryChanged == true && cycleMemory == 6) return ;
	updateTackles();
	updateSaves();
	if (cycleMemoryChanged == true && cycleMemory == 8) return ;
	updateShoots();
	if (cycleMemoryChanged == true && cycleMemory == 7) return ;
	updatePasses();
	if (cycleMemoryChanged == true && cycleMemory == 10) return ;
	updateDribbles();
	if (cycleMemoryChanged == true && cycleMemory == 11) return ;
}

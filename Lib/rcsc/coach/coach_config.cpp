// -*-c++-*-

/*!
  \file coach_config.cpp
  \brief coach configuration Source File
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

#include "coach_config.h"

#include <rcsc/param/param_map.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
CoachConfig::CoachConfig()
{
    setDefaultParam();
}

/*-------------------------------------------------------------------*/
/*!

*/
CoachConfig::~CoachConfig()
{

    // std::cerr << "delete CoachConfig" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachConfig::setDefaultParam()
{
    // basic setting
    M_team_name = "HELIOS_base";
    M_version = 14;

    M_coach_name = "Coach_base";
    M_use_coach_name = false;

    M_interval_msec = 50;
    M_server_wait_seconds = 5;

    M_rcssserver_host = "localhost";
    M_rcssserver_port = 6002;

    M_compression = -1;

    M_use_eye = true;
    M_hear_say = true;

    M_analyze_player_type = true;

    M_use_advise = true;
    M_use_freeform = true;

    M_use_hetero = true;

    M_use_team_graphic = false;
    M_team_graphic_file = "";

    M_max_team_graphic_per_cycle = 32;

    //
    // debug
    //

    M_debug = false;
    M_log_dir = "/tmp";

    //
    // debug server
    //
    M_debug_server_connect = false;
    M_debug_server_logging = false;
    M_debug_server_host = "localhost";
    M_debug_server_port = 6000 + 32;

    //
    // offline client
    //
    M_offline_logging = false;
    M_offline_log_ext = ".ocl";

    M_offline_client_mode = false;

    //
    // debug logging
    //

    M_debug_log_ext = ".log";

    M_debug_system = false;
    M_debug_sensor = false;
    M_debug_world = false;
    M_debug_action = false;
    M_debug_intercept = false;
    M_debug_kick = false;
    M_debug_hold = false;
    M_debug_dribble = false;
    M_debug_pass = false;
    M_debug_cross = false;
    M_debug_shoot = false;
    M_debug_clear = false;
    M_debug_block = false;
    M_debug_mark = false;
    M_debug_positioning = false;
    M_debug_role = false;
    M_debug_plan = false;
    M_debug_team = false;
    M_debug_communication = false;
    M_debug_analyzer = false;
    M_debug_action_chain = false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
CoachConfig::createParamMap( ParamMap & param_map )
{
    param_map.add()
        ( "team_name", "t", &M_team_name )
        ( "version", "v", &M_version )

        ( "coach_name", "", &M_coach_name )
        ( "use_coach_name", "", &M_use_coach_name )

        ( "interval_msec", "", &M_interval_msec )
        ( "server_wait_seconds", "", &M_server_wait_seconds )

        ( "host", "h",  &M_rcssserver_host )
        ( "port", "p", &M_rcssserver_port )

        ( "compression", "", &M_compression )

        ( "use_eye", "", &M_use_eye )
        ( "hear_say", "", &M_hear_say )

        ( "analyze_player_type", "", &M_analyze_player_type )

        ( "use_advise", "", &M_use_advise )
        ( "use_freeform", "", &M_use_freeform )

        ( "use_hetero", "", &M_use_hetero )

        ( "use_team_graphic", "", &M_use_team_graphic )
        ( "team_graphic_file", "", &M_team_graphic_file )
        ( "max_team_graphic_per_cycle", "", &M_max_team_graphic_per_cycle )

        ( "debug", "", BoolSwitch( &M_debug ) )
        ( "log_dir", "", &M_log_dir )

        ( "debug_server_connect", "", BoolSwitch( &M_debug_server_connect ) )
        ( "debug_server_logging", "", BoolSwitch( &M_debug_server_logging ) )
        ( "debug_server_host", "", &M_debug_server_host )
        ( "debug_server_port", "", &M_debug_server_port )

        ( "offline_logging", "", BoolSwitch( &M_offline_logging ) )
        ( "offline_log_ext", "", &M_offline_log_ext )
        ( "offline_client_mode", "", BoolSwitch( &M_offline_client_mode ) )

        ( "debug_log_ext", "", &M_debug_log_ext )

        ( "debug_system", "", BoolSwitch( &M_debug_system ) )
        ( "debug_sensor", "", BoolSwitch( &M_debug_sensor ) )
        ( "debug_world", "", BoolSwitch( &M_debug_world ) )
        ( "debug_action", "", BoolSwitch( &M_debug_action ) )
        ( "debug_intercept", "", BoolSwitch( &M_debug_intercept ) )
        ( "debug_kick", "", BoolSwitch( &M_debug_kick ) )
        ( "debug_hold", "", BoolSwitch( &M_debug_hold ) )
        ( "debug_dribble", "", BoolSwitch( &M_debug_dribble ) )
        ( "debug_pass", "", BoolSwitch( &M_debug_pass ) )
        ( "debug_cross", "", BoolSwitch( &M_debug_cross ) )
        ( "debug_shoot", "", BoolSwitch( &M_debug_shoot ) )
        ( "debug_clear", "", BoolSwitch( &M_debug_clear ) )
        ( "debug_block", "", BoolSwitch( &M_debug_block ) )
        ( "debug_mark", "", BoolSwitch( &M_debug_mark ) )
        ( "debug_positioning", "", BoolSwitch( &M_debug_positioning ) )
        ( "debug_role", "", BoolSwitch( &M_debug_role ) )
        ( "debug_plan", "", BoolSwitch( &M_debug_plan ) )
        ( "debug_team", "", BoolSwitch( &M_debug_team ) )
        ( "debug_communication", "", BoolSwitch( &M_debug_communication ) )
        ( "debug_analyzer", "", BoolSwitch( &M_debug_analyzer ) )
        ( "debug_action_chain", "", BoolSwitch( &M_debug_action_chain ) )
        ;
}

}

// -*-c++-*-

/*!
  \file trainer_config.cpp
  \brief trainer configuration Source File
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

#include "trainer_config.h"

#include <rcsc/param/param_map.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
TrainerConfig::TrainerConfig()
{
    setDefaultParam();
}

/*-------------------------------------------------------------------*/
/*!

*/
TrainerConfig::~TrainerConfig()
{

    // std::cerr << "delete TrainerConfig" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TrainerConfig::setDefaultParam()
{
    // basic setting
    M_team_name = "HELIOS_base";
    M_version = 14;

    M_server_wait_seconds = 5;

    M_rcssserver_host = "localhost";
    M_rcssserver_port = 6001;

    M_compression = -1;

    M_use_eye = true;
    M_use_ear = false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
TrainerConfig::createParamMap( ParamMap & param_map )
{
    param_map.add()
        ( "team_name", "t", &M_team_name )
        ( "version", "v", &M_version )

        ( "server_wait_seconds", "", &M_server_wait_seconds )

        ( "host", "h", &M_rcssserver_host )
        ( "port", "p", &M_rcssserver_port )

        ( "compression", "", &M_compression )

        ( "use_eye", "", &M_use_eye )
        ( "use_ear", "", &M_use_ear )
        ;
}

}

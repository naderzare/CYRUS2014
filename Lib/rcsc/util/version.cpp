// -*-c++-*-

/*!
  \file version.cpp
  \brief version number Source File
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

#include <rcsc/version.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

 */
const char *
copyright()
{
    /*
      Do NOT change the following copyright notice!
     */
    return
        "******************************************************************\n"
        " "PACKAGE_STRING"\n"
        " Copyright 2000 - 2007. Hidehisa Akiyama.\n"
        " Copyright 2007 - 2011. Hidehisa Akiyama and Hiroki Shimora\n"
        " All rights reserved.\n"
        "******************************************************************\n"
        ;
}

/*-------------------------------------------------------------------*/
/*!

 */
const char *
version()
{
    return VERSION;
}

}

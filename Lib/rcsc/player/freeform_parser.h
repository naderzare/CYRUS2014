// -*-c++-*-

/*!
  \file freeform_parser.h
  \brief coach's freeform message parser Header File
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

#ifndef RCSC_PLAYER_FREEFORM_PARSER_H
#define RCSC_PLAYER_FREEFORM_PARSER_H

#include <rcsc/common/audio_message.h>
#include <rcsc/types.h>

#include <boost/shared_ptr.hpp>

#include <string>

namespace rcsc {

class AudioSensor;
class WorldModel;

/*!
  \class FreeformParser
  \brief coach's freeform message parser
 */
class FreeformParser {
public:

    //! pointer type alias
    typedef boost::shared_ptr< FreeformParser > Ptr;

private:

    // not used
    FreeformParser();
    FreeformParser( const FreeformParser & );
    FreeformParser & operator=( const FreeformParser & );

protected:

    //! reference to the world model
    WorldModel & M_world;

public:

    /*!
      \brief protected constructer
      \param world reference to the world model
     */
    explicit
    FreeformParser( WorldModel & world );

    /*!
      \brief virtual destruct. do nothing.
     */
    virtual
    ~FreeformParser()
      { }

    /*!
      \brief analyzes freeform message.
      \retval bytes read if success
      \retval 0 message ID is not match. other parser should be tried.
      \retval -1 failed to parse
    */
    virtual
    int parse( const char * msg );

};

}

#endif

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

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

#ifndef RCSC_COMMON_PERIODIC_CALLBACK_H
#define RCSC_COMMON_PERIODIC_CALLBACK_H

#include <boost/shared_ptr.hpp>

#include <vector>

namespace rcsc {

/*!
  \class PeriodicCallback
  \brief abstract callback which are called every cycle periodically
*/
class PeriodicCallback {
public:

    //! smart pointer type
    typedef boost::shared_ptr< PeriodicCallback > Ptr;

    //! container type
    typedef std::vector< PeriodicCallback::Ptr > Cont;

private:

    //! flag variable to determine the callback lifetime.
    bool M_finished;

protected:

    /*!
      \brief protected constructor.
     */
    PeriodicCallback()
        : M_finished( false )
      { }

public:

    /*!
      \brief virtual destructor
    */
    virtual
    ~PeriodicCallback()
      { }

protected:

    /*!
      \brief this method will be called every cycle unless setFinished() is called in this method.
    */
    virtual
    void execute() = 0;

    /*!
      \brief kill the callback lifetime.
     */
    void setFinished()
      {
          M_finished = true;
      }

private:

    /*!
      \brief get the lifetime status flag.
      \return callback's lifetime status.
     */
    bool isFinished() const
      {
          return M_finished;
      }

public:


    /*!
      \brief static method for algorithm. just call p->execute().
      \parram p called object.
     */
    static void call_execute( Ptr & p )
      {
          p->execute();
      }

    /*!
      \brief static method for algorithm. just call p->finished().
      \parram p checked object.
      \return the value of p->finished()
     */
    static bool is_finished( const Ptr & p )
      {
          return p->isFinished();
      }

};

}

#endif

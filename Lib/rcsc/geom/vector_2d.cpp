// -*-c++-*-

/*!
  \file vector_2d.cpp
  \brief 2D vector class Source File.
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

#include "vector_2d.h"

#include <limits>

namespace rcsc {

const double Vector2D::EPSILON = 1.0e-6;

const double Vector2D::ERROR_VALUE = std::numeric_limits< double >::max();

const Vector2D Vector2D::INVALIDATED( Vector2D::ERROR_VALUE, Vector2D::ERROR_VALUE );

}

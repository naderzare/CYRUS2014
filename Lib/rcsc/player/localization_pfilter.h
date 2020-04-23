// -*-c++-*-

/*!
  \file localization_pfilter.h
  \brief localization module using particle filter Header File
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

#ifndef RCSC_PLAYER_LOCALIZATION_PFILTER_H
#define RCSC_PLAYER_LOCALIZATION_PFILTER_H

#include <rcsc/player/localization.h>

#include <boost/scoped_ptr.hpp>

namespace rcsc {

/*!
  \class Localization
  \brief localization module
*/
class LocalizationPFilter
    : public Localization {
private:

    class Impl;

    //! implemantion
    boost::scoped_ptr< Impl > M_impl;


    // not used
    LocalizationPFilter( const LocalizationPFilter & );
    LocalizationPFilter & operator=( const LocalizationPFilter & );

public:
    /*!
      \brief create internal implementation
      \param ourside our team side
    */
    LocalizationPFilter();

    /*!
      \brief implicitly delete internal impl
    */
    virtual
    ~LocalizationPFilter();

public:

   /*!
      \brief estimate self facing direction.
      \param body analyzed body information
      \return result
     */
    virtual
    bool updateBySenseBody( const BodySensor & body );

    /*!
      \brief estimate self facing direction.
      \param see analyzed see information
      \param self_face pointer to the result variable
      \param self_face_err pointer to the result variable
      \return result
     */
    virtual
    bool estimateSelfFace( const VisualSensor & see,
                           double * self_face,
                           double * self_face_err );

    /*!
      \brief localize self position.
      \param see analyzed see info
      \param self_face localized face angle
      \param self_face_err localized face angle error
      \param self_pos pointer to the variable to store the localized self position
      \param self_pos_err pointer to the variable to store the localized self position error
      \return if failed, returns false
    */
    virtual
    bool localizeSelf( const VisualSensor & see,
                       const double & self_face,
                       const double & self_face_err,
                       Vector2D * self_pos,
                       Vector2D * self_pos_err );

    /*!
      \brief localze ball relative info
      \param see analyzed see info
      \param self_face localized self face angle
      \param self_face_err localized self face angle error
      \param rpos pointer to the variable to store the localized relative position
      \param rpos_err pointer to the variable to store the localized relative position error
      \param rvel pointer to the variable to store the localized relative velocity
      \param rvel_err pointer to the variable to store the localized relative velocity error
      \return if failed, returns false
    */
    virtual
    bool localizeBallRelative( const VisualSensor & see,
                               const double & self_face,
                               const double & self_face_err,
                               Vector2D * rpos,
                               Vector2D * rpos_err,
                               Vector2D * rvel,
                               Vector2D * rvel_err );

    /*!
      \brief localze other player
      \param from seen player info
      \param self_face localized self face angle
      \param self_face_err localized self face angle error
      \param self_pos localized self position
      \param self_vel localized self velocity
      \param to pointer to the variable to store the localized result
      \return if failed, returns false
    */
    virtual
    bool localizePlayer( const VisualSensor::PlayerT & from,
                         const double & self_face,
                         const double & self_face_err,
                         const Vector2D & self_pos,
                         const Vector2D & self_vel,
                         PlayerT * to );
};

}

#endif

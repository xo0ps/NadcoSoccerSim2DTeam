// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

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
#include <config.h>
#endif

#include "body_kick_to_corner.h"

#include <rcsc/action/body_hold_ball.h>
#include "body_smart_kick.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

/*---------------------------------------------*/
bool
Body_KickToCorner::execute( rcsc::PlayerAgent * agent )
{
    static const rcsc::Vector2D left_corner( rcsc::ServerParam::i().pitchHalfLength() - 10.0,
                                       -rcsc::ServerParam::i().pitchHalfWidth() + 8.0 );
    static const rcsc::Vector2D right_corner( rcsc::ServerParam::i().pitchHalfLength() - 10.0,
                                        rcsc::ServerParam::i().pitchHalfWidth() - 8.0 );

    const rcsc::Vector2D target_point = ( M_to_left ? left_corner : right_corner );
    rcsc::AngleDeg target_angle = ( target_point - agent->world().self().pos() ).th();
    if ( agent->world().dirCount( target_angle ) >= 2 )
    {
        rcsc::Vector2D face_point( 47.0, agent->world().self().pos().y * 0.9 );
        rcsc::Body_HoldBall( true, face_point ).execute( agent );
        return true;
    }


    double first_speed
        = rcsc::calc_first_term_geom_series_last
        ( 0.18,
          agent->world().ball().pos().dist( target_point ),
          rcsc::ServerParam::i().ballDecay() );

    first_speed = std::min( first_speed,
                            rcsc::ServerParam::i().ballSpeedMax() );

    Body_SmartKick( target_point,
                          first_speed,
                          first_speed * 0.96,
                          3 ).execute( agent );

    return true;
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#include "neck_chase_ball.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/view_synch.h>
#include <rcsc/action/view_normal.h>
#include <rcsc/action/view_change_width.h>

#include <rcsc/math_util.h>

namespace rcsc_ext {

/*-------------------------------------------------------------------*/
/*!

*/
Neck_ChaseBall::Neck_ChaseBall()
{ }

/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_ChaseBall::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::ServerParam & param = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D next_ball_pos = agent->effector().queuedNextBallPos();
    const rcsc::Vector2D next_self_pos = agent->effector().queuedNextMyPos();
    const rcsc::AngleDeg next_self_body = agent->effector().queuedNextMyBody();
    const double narrow_half_width = rcsc::ViewWidth::width( rcsc::ViewWidth::NARROW ) / 2.0;
    const double next_view_half_width = agent->effector().queuedNextViewWidth().width() / 2.0;

    const rcsc::AngleDeg angle_diff = ( next_ball_pos - next_self_pos ).th() - next_self_body;

    const rcsc::SeeState & see_state = agent->seeState();
    const int see_cycles = see_state.cyclesTillNextSee() + 1;
    const bool can_see_next_cycle_narrow = ( angle_diff.abs()
                                             < param.maxNeckAngle()
                                               + narrow_half_width );
    const bool can_see_next_cycle = ( angle_diff.abs()
                                      < param.maxNeckAngle()
                                        + next_view_half_width );

    if ( can_see_next_cycle )
    {
        if ( see_cycles != 1
             && can_see_next_cycle_narrow )
        {
            rcsc::View_Synch().execute( agent );
        }
    }
    else
    {
        if ( see_cycles >= 2
             && can_see_next_cycle_narrow )
        {
            rcsc::View_Synch().execute( agent );
        }
        else
        {
            rcsc::View_Normal().execute( agent );
        }
    }

    double target_angle = rcsc::bound( - param.maxNeckAngle(),
                                       angle_diff.degree(),
                                       + param.maxNeckAngle() );

    agent->doTurnNeck( target_angle - wm.self().neck() );

    return true;
}

rcsc::NeckAction *
Neck_ChaseBall::clone() const
{
    return new Neck_ChaseBall();
}

}

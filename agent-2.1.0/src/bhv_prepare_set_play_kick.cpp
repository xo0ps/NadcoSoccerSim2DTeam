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
#include <config.h>
#endif

#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "bhv_set_play.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
 */
bool
Bhv_PrepareSetPlayKick::execute( rcsc::PlayerAgent * agent )
{
    static int S_rest_wait_cycle = -1;

    // not reach the ball side
    if ( Bhv_GoToStaticBall( M_ball_place_angle ).execute( agent ) )
    {
        return true;
    }

    // reach to ball side

    if ( S_rest_wait_cycle < 0 )
    {
        S_rest_wait_cycle = M_wait_cycle;
    }

    if ( S_rest_wait_cycle == 0 )
    {
        if ( //agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.9 ||
			agent->world().seeTime() != agent->world().time() )
        {
            S_rest_wait_cycle = 1;
        }
    }

    if ( S_rest_wait_cycle > 0 )
    {
        if ( agent->world().gameMode().type() == rcsc::GameMode::KickOff_ )
        {
            rcsc::AngleDeg moment( rcsc::ServerParam::i().visibleAngle() );
            agent->doTurn( moment );
        }
        else
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        S_rest_wait_cycle--;
        return true;
    }

    S_rest_wait_cycle = -1;

    return false;
}

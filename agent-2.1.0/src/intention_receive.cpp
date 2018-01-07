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

#include "intention_receive.h"

#include "body_go_to_point.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

 */
IntentionReceive::IntentionReceive( const rcsc::Vector2D & target_point,
                                    const double & dash_power,
                                    const double & buf,
                                    const int max_step,
                                    const rcsc::GameTime & start_time )
    : M_target_point( target_point )
    , M_dash_power( dash_power )
    , M_buffer( buf )
    , M_step( max_step )
    , M_last_execute_time( start_time )
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionReceive::finished( const rcsc::PlayerAgent * agent )
{
    if ( M_step <= 0 )
    {
        return true;
    }

    if ( agent->world().self().isKickable() )
    {
        return true;
    }

    if ( agent->world().existKickableTeammate() )
    {
        return true;
    }

    if ( agent->world().ball().distFromSelf() < 3.0 )
    {
        return true;
    }

    if ( M_last_execute_time.cycle() < agent->world().time().cycle() - 1 )
    {
        return true;
    }

    if ( agent->world().self().pos().dist( M_target_point ) < M_buffer )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionReceive::execute( rcsc::PlayerAgent * agent )
{
    if ( M_step <= 0 )
    {
        return false;
    }

    const rcsc::WorldModel & wm = agent->world();

    M_step -= 1;
    M_last_execute_time = wm.time();

    int self_min = wm.interceptTable()->selfReachCycle();
    //int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( self_min < 6 )
    {
        Body_Intercept().execute( agent );
        //agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    if ( Body_Intercept().execute( agent ) )
    {
        //agent->setNeckAction( new Neck_OffensiveInterceptNeck() );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    Body_GoToPoint( M_target_point,
                    M_buffer,
                    M_dash_power
                    ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );

    return true;
}

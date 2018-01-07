// -*-c++-*-

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

#include "bhv_before_kick_off.h"

#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/neck_turn_to_relative.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/body_turn_to_angle.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_BeforeKickOff::execute( rcsc::PlayerAgent * agent )
{
	
    if ( agent->world().time().cycle() == 0 && agent->world().time().stopped() < 5 )
    {
		static rcsc::AngleDeg deg = 0.0;
        deg += 60.0;
	    if( deg == 360.0 )
			deg = 0.0;
	    rcsc::Body_TurnToAngle( deg ).execute( agent );
	    agent->setNeckAction( new rcsc::Neck_ScanField() );

        //agent->doTurn( 0.0 );
        //agent->setNeckAction( new rcsc::Neck_TurnToRelative( 0.0 ) );
        return false;
    }


    if ( ! agent->world().self().posValid() )
    {
        return rcsc::Bhv_ScanField().execute( agent );
    }

    if ( rcsc::ServerParam::i().kickoffOffside() && M_move_point.x >= 0.0 )
    {
        M_move_point.x = -1.0;
    }

    rcsc::SideID kickoff_side = rcsc::NEUTRAL;

    if ( agent->world().gameMode().type() == rcsc::GameMode::AfterGoal_ )
    {
    }
    else // before_kick_off
    {
        // check half_time count
        if ( rcsc::ServerParam::i().halfTime() > 0 )
        {
            int time_flag = ( ( (agent->world().time().cycle() + 1)
                                / 3000 ) // ServerParam::i().halfTime() )
                              % 2 );
            kickoff_side = ( time_flag == 0
                             ? rcsc::LEFT
                             : rcsc::RIGHT );
        }
        else
        {
            kickoff_side = rcsc::LEFT;
        }
    }

    if ( kickoff_side != agent->world().ourSide()
         && M_move_point.r() < rcsc::ServerParam::i().centerCircleR() + 0.1 )
    {
        M_move_point *= ( rcsc::ServerParam::i().centerCircleR() + 0.5 ) / M_move_point.r();
    }

    // move
    double tmpr = ( M_move_point - agent->world().self().pos() ).r();
    if ( tmpr > 1.0 )
    {
        agent->doMove( M_move_point.x, M_move_point.y );
        agent->setNeckAction( new rcsc::Neck_TurnToRelative( 0.0 ) );
        return true;
    }

    // field scan
    return rcsc::Bhv_ScanField().execute( agent );
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI
 
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

#include "strategy.h"
#include "sample_player.h"
#include "role_goalie.h"
#include "bhv_goalie.h"
#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_goalie_free_kick.h"
#include "body_direct_pass.h"
#include "body_clear_ball.h"
#include "body_kick_one_step.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>

#include <rcsc/player/player_agent.h>

#include <rcsc/player/world_model.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/server_param.h>


/*-------------------------------------------------------------------*/
/*!

*/
void
RoleGoalie::execute( rcsc::PlayerAgent* agent )
{
	if ( Strategy::i().goalie() )
    {
        Bhv_Goalie().execute( agent );
        return;
    }

    static const
        rcsc::Rect2D our_penalty( rcsc::Vector2D( -rcsc::ServerParam::i().pitchHalfLength(),
                                                  -rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                                  rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() - 1.0,
                                                rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );

    //////////////////////////////////////////////////////////////
    // play_on play

    if ( agent->world().time().cycle() > agent->world().self().catchTime().cycle() + rcsc::ServerParam::i().catchBanCycle()
         && agent->world().ball().distFromSelf() < rcsc::ServerParam::i().catchableArea() - 0.05
         && our_penalty.contains( agent->world().ball().pos() )
         && agent->world().lastKickerSide() != agent->world().self().side() )
    {
        agent->doCatch();
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    if ( agent->world().self().isKickable() )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleGoalie::doKick( rcsc::PlayerAgent * agent )
{
	if(! Body_ClearBall().execute( agent ) )
		if( ! Body_DirectPass("playOn").execute( agent ) )
		{
			rcsc::Vector2D pos( 0 , 0 );
			const rcsc::PlayerObject * tm = agent->world().teammatesFromSelf().back();
			if( tm )
				pos = tm->pos();
			Body_KickOneStep( pos , rcsc::ServerParam::i().maxPower() ).execute( agent );
		}
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleGoalie::doMove( rcsc::PlayerAgent * agent )
{
    if ( Bhv_GoalieChaseBall::is_ball_chase_situation( agent ) )
    {
        Bhv_GoalieChaseBall().execute( agent );
    }
    else
    {
        Bhv_GoalieBasicMove().execute( agent );
    }
}

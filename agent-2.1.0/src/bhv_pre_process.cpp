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

#include "bhv_pre_process.h"
#include "strategy.h"
#include "bhv_before_kick_off.h"
#include "body_shoot.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "bhv_after_goal.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_emergency.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PreProcess::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
	
    //////////////////////////////////////////////////////////////
    // freezed by tackle effect
    if ( wm.self().isFrozen() )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    //////////////////////////////////////////////////////////////
      // BeforeKickOff or AfterGoal. should jump to the initial position
    if ( agent->world().gameMode().type() == rcsc::GameMode::BeforeKickOff 
		|| ( agent->world().gameMode().type() == rcsc::GameMode::AfterGoal_ 
			&& agent->world().gameMode().side() != agent->world().ourSide() ) )
    {
        rcsc::Vector2D move_point =  Strategy::i().getPosition( agent->world().self().unum() );
        agent->setViewAction( new rcsc::View_Wide() );
        Bhv_BeforeKickOff( move_point ).execute( agent );
        return true;
    }

	if( agent->world().gameMode().type() == rcsc::GameMode::AfterGoal_ 
		&& agent->world().gameMode().side() == agent->world().ourSide() )
	{
		return Bhv_AfterGoal().execute(agent);
	}

    //////////////////////////////////////////////////////////////
    // my pos is unknown
    if ( ! wm.self().posValid() )
    {
        // included change view
        rcsc::Bhv_Emergency().execute( agent );
        return true;
    }
    //////////////////////////////////////////////////////////////
    // ball search
    // included change view
    int count_thr = 5;
    if ( wm.self().goalie() ) count_thr = 10;
    if ( wm.ball().posCount() > count_thr )
        // || wm.ball().rposCount() > count_thr + 3 )
    {
        rcsc::Bhv_NeckBodyToBall().execute( agent );
        return true;
    }

    //////////////////////////////////////////////////////////////
    // check shoot chance
    
    if ( wm.gameMode().type() != rcsc::GameMode::BackPass_
         && wm.gameMode().type() != rcsc::GameMode::CatchFault_
         && wm.gameMode().type() != rcsc::GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Body_Shoot().execute( agent ) )
    {
        // reset intention
        agent->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );
        return true;
    }
    

    //////////////////////////////////////////////////////////////
    // check queued action
    if ( agent->doIntention() )
    {
        return true;
    }

    //////////////////////////////////////////////////////////////
    // check simultaneous kick
    
    
    if ( wm.gameMode().type() == rcsc::GameMode::PlayOn
         && ! wm.self().goalie()
         && wm.self().isKickable()
         && wm.existKickableOpponent() )
    {
        const rcsc::PlayerObject * kicker = wm.interceptTable()->fastestOpponent();
        if ( kicker
             && ! kicker->isTackling()
             && kicker->isKickable( 0.1 ) )
        {
            rcsc::Vector2D goal_pos( rcsc::ServerParam::i().pitchHalfLength(), 0.0 );

            if ( wm.self().pos().x > 36.0
                 && wm.self().pos().absY() > 10.0 )
            {
                goal_pos.x = 45.0;
            }
            else if ( 10.0 < wm.self().pos().x
                      && wm.self().pos().x < 33.0 )
            {
                goal_pos.x = 45.0;
                goal_pos.y = 23.0;
                if ( wm.ball().pos().y < 0.0 ) goal_pos.y *= -1.0;
            }

            Body_KickOneStep( goal_pos,
                                    rcsc::ServerParam::i().ballSpeedMax(),
                                    true // force mode
                                    ).execute( agent );
            agent->setNeckAction( new rcsc::Neck_TurnToBall );
            return true;
        }
    }
    

    //////////////////////////////////////////////////////////////
    // check communication intention
    if ( wm.audioMemory().passTime() == wm.time()
         && ! wm.audioMemory().pass().empty()
         && ( wm.audioMemory().pass().front().receiver_
              == wm.self().unum() )
         )
    {
        doReceiveMove( agent );

        return true;
    }

    return false;
}


/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_PreProcess::doReceiveMove( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();
    int self_min = wm.interceptTable()->selfReachCycle();

    rcsc::Vector2D self_trap_pos = wm.ball().inertiaPoint( self_min );
    rcsc::Vector2D receive_pos = ( wm.audioMemory().pass().empty()
                                   ? self_trap_pos
                                   : wm.audioMemory().pass().front().receive_pos_ );

    if ( ( ! wm.existKickableTeammate()
           && wm.ball().posCount() <= 1
           && wm.ball().velCount() <= 1
           && self_min < 6
           && self_trap_pos.dist( receive_pos ) < 8.0 )
         || wm.audioMemory().pass().empty() )
    {
        Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return;
    }

    bool back_mode = false;

    rcsc::Vector2D target_rel = receive_pos - wm.self().pos();
    rcsc::AngleDeg target_angle = target_rel.th();
    if ( target_rel.r() < 6.0
         && ( target_angle - wm.self().body() ).abs() > 100.0
         && wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.4 )
    {
        back_mode = true;
    }

    Body_GoToPoint( receive_pos,
                          1.0,
                          rcsc::ServerParam::i().maxDashPower(),
                          100,
                          back_mode
                          ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
}

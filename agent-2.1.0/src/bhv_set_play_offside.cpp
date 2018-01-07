// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

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


#include "bhv_set_play_offside.h"
#include "bhv_set_play_free_kick.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "bhv_mark.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_SetPlayOffside::execute( rcsc::PlayerAgent * agent )
{
    return Bhv_SetPlayFreeKick( M_home_pos ).execute( agent );
    
	if ( isKicker( agent ) )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlayOffside::isKicker( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    long max_wait1 = 30;
    long max_wait2 = 50;

    // goalie catch mode && I'm not a goalie
    if ( wm.gameMode().type() == rcsc::GameMode::GoalieCatch_
         && wm.gameMode().side() == wm.ourSide()
         && ! wm.self().goalie() )
    {
        return false;
    }

    const rcsc::PlayerObject * nearest_mate
        = static_cast< rcsc::PlayerObject * >( 0 );
    nearest_mate = wm.teammatesFromBall().front();

    if ( wm.setplayCount() < max_wait1
         || ( wm.setplayCount() < max_wait2
              && wm.self().pos().dist( M_home_pos ) > 20.0 )
         || ( nearest_mate
              && nearest_mate->distFromBall() < wm.ball().distFromSelf() * 0.9 )
         )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayOffside::doKick( rcsc::PlayerAgent * agent )
{
    // I am kickaer
    static int S_scan_count = -5;

    // go to ball
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }

    // already ball point

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D face_point( 40.0, 0.0 );
    const rcsc::AngleDeg face_angle
        = ( face_point - wm.self().pos() ).th();

    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( S_scan_count < 0 )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( S_scan_count < 10
         && wm.teammatesFromSelf().empty() )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( wm.time().stopped() != 0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    S_scan_count = -5;

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayOffside::doMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D target_point = M_home_pos;

    double dash_power
        = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;
    if ( ! Body_GoToPoint( target_point,
                                 dist_thr,
                                 dash_power
                                 ).execute( agent ) )
    {
		if( Bhv_Mark( "markEscape", M_home_pos ).execute( agent ) )
			return;
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( //wm.self().pos().dist( target_point ) > std::max( wm.ball().pos().dist( target_point ) * 0.2 , 0.5 ) + 6.0 || 
	wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7
       )
    {
        agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

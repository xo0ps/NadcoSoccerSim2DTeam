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


#include "bhv_set_play_backpass.h"
#include "bhv_set_play_indirect_free_kick.h"
#include "body_pass.h"
#include "body_direct_pass.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_SetPlayBackpass::execute( rcsc::PlayerAgent * agent )
{
	
	//return Bhv_SetPlayIndirectFreeKick().execute( agent );
	//return Bhv_SetPlayIndirectFreeKick::doOffenseMove( agent );
	
	if ( agent->world().gameMode().side() == agent->world().ourSide() )
	{
		//doSave( agent );
		Bhv_SetPlayIndirectFreeKick::doDefenseMove( agent );
		return true;
	}
	else
    if ( isKicker( agent ) )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
        Bhv_SetPlayIndirectFreeKick::doOffenseMove( agent );
        return true;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayBackpass::doSave( rcsc::PlayerAgent * agent )
{
	
    const rcsc::WorldModel & wm = agent->world();
	const int unum = wm.self().unum();
	rcsc::Vector2D point = M_home_pos;
	
	switch( unum )
	{
		case 1:
			point = rcsc::Vector2D( -52.5 , 0.0 );
			break;
		case 2:
			point = rcsc::Vector2D( -52.5 , 6.0 );
			break;
		case 3:
			point = rcsc::Vector2D( -52.5 , 4.0 );
			break;
		case 4:
			point = rcsc::Vector2D( -52.5 , 2.0 );
			break;
		case 6:
			point = rcsc::Vector2D( -52.5 , -2.0 );
			break;
		case 7:
			point = rcsc::Vector2D( -52.5 , -4.0 );
			break;
		case 5:
			point = rcsc::Vector2D( -52.5 , -6.0 );
			break;
		default:
			break;
	}

    double dash_power = rcsc::ServerParam::i().maxPower();
    
    if ( ! Body_GoToPoint( point, 0.5 , dash_power ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlayBackpass::isKicker( const rcsc::PlayerAgent * agent )
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
Bhv_SetPlayBackpass::doKick( rcsc::PlayerAgent * agent )
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
	Body_DirectPass("playOn").execute( agent );
	/*
	const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();	
	for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
    {
		if( !(*it) ) continue;
        //if( (*it)->posCount() > 3 ) continue;
        if( (*it)->pos().dist( wm.self().pos() ) < 10.0 ) continue;
        
		const double ball_decay = rcsc::ServerParam::i().ballDecay();
		const double ball_speed_max = rcsc::ServerParam::i().ballSpeedMax();
		int reach_cycle = (int)(log( 1.0 - ( 1.0 - ball_decay ) * wm.self().pos().dist( (*it)->pos() ) / ball_speed_max ) / log( ball_decay ));
		double ball_speed = (*it)->playerTypePtr()->kickableArea() * 1.5 * pow( 1.0 / ball_decay, reach_cycle - 1 );
		double first_speed = std::min( ball_speed_max, ball_speed );
		rcsc::Vector2D target_point = wm.ball().pos() * 0.2 + (*it)->pos() * 0.8;
		if( Body_KickOneStep( target_point, first_speed ).execute( agent ) )
		{
			agent->addSayMessage( new rcsc::PassMessage( (*it)->unum(),
														 target_point,
														 agent->effector().queuedNextBallPos(),
														 agent->effector().queuedNextBallVel() ) );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return;
		}
	}
	*/

	
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayBackpass::doMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D point = M_home_pos;
	//double stepX = 0;
	double offside = wm.offsideLineX();
	int counter = 0;
	
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{	
		if( (! (*tm)) || (*tm)->isGhost() )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().dist( wm.ball().pos() ) > 40 )
			continue;
		
		/*
		if( (*tm)->unum() % 3 == 0 )
		{
			stepX = 1.0;
		}
		else
		if( (*tm)->unum() % 3 == 1 )
		{
			stepX = -1.0;
		}
		else
		{
			stepX = -2.0;
		}
		point = rcsc::Vector2D( wm.ball().pos().x + stepX , wm.ball().pos().y + pow(-1 , (*tm)->unum()) * (*tm)->unum() );
		*/
		
		if( counter > 3 ) break;
		if( (*tm)->pos().x < wm.ball().pos().x && (*tm)->pos().dist( wm.ball().pos() ) < 20.0 )
		{
			point = rcsc::Vector2D( offside - 5.0 , pow( -1 , (*tm)->unum() ) * (*tm)->unum() );
			counter++;
		}
	}
	
    double dash_power = rcsc::ServerParam::i().maxPower();
    
    if ( ! Body_GoToPoint( point, 0.5 , dash_power ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }

    if ( //wm.self().pos().dist( point ) > std::max( wm.ball().pos().dist( point ) * 0.2, 0.5 ) + 6.0  || 
	wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7
       )
    {
        agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

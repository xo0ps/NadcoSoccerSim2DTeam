// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

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

#include "bhv_tactics.h"
#include "strategy.h"
#include "sample_player.h"
#include "body_kick_one_step.h"
#include "body_pass.h"
#include "body_smart_kick.h"
#include "bhv_set_play.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/game_mode.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_Tactics::execute( rcsc::PlayerAgent * agent )
{	
	
	//if ( ! SamplePlayer::instance().tactics() )
	if ( ! Strategy::i().tactics() )
    {
		return false;
    }

	if( M_mode == "oneTwo" )
		return oneTwoPass( agent );
	else
	if( M_mode == "coolDown" )
		return coolDown( agent );
	else
	if( M_mode == "timeKill" )
		return timeKill( agent );
	else
	if( M_mode == "farPass" )
		return farPass( agent );
	else
	if( M_mode == "substitueRequest" )
		return substitueRequest( agent );
	else
	if( M_mode == "substitueKick" )
		return substitueKick( agent );
	else
	{
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return false;
	}
    
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
}

/*----------------------------------------------------------*/
bool
Bhv_Tactics::oneTwoPass( rcsc::PlayerAgent * agent )
{
	
	
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
}


/*----------------------------------------------------------*/
bool
Bhv_Tactics::coolDown( rcsc::PlayerAgent * agent )
{
	
	
	
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
}


/*----------------------------------------------------------*/
bool
Bhv_Tactics::timeKill( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if( wm.gameMode().type() != rcsc::GameMode::PlayOn )
		return false;
	
	//if(! Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
	//	return false;

	if( wm.self().stamina() > 1000 )
		return false;

	if ( wm.time().cycle() < 2500 || ( wm.time().cycle() > 3000 && wm.time().cycle() < 5500 ) )
        return false;
    
    if( wm.self().pos().x > 20.0 )
		return false;

	rcsc::Vector2D out( wm.ball().pos().x , 34.0 );
	if( wm.ball().pos().y < 0.0 )
		out.y *= -1.0;
	double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	while( wm.ball().pos().dist( out ) < 20.0 )
	{
		out.x++;
	}
    
    if( Body_SmartKick( out, rcsc::ServerParam::i().ballSpeedMax() , rcsc::ServerParam::i().ballSpeedMax() * 0.99 , 3 ).execute( agent ) )
    {
		std::cout<<"time kill"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( out ) );
        return true;
	}

	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return false;
	
}

/*----------------------------------------------------------*/
bool
Bhv_Tactics::farPass( rcsc::PlayerAgent * agent )
{
	
	
	
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
}


/*----------------------------------------------------------*/
bool
Bhv_Tactics::substitueRequest( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if( wm.gameMode().type() != rcsc::GameMode::PlayOn )
		return false;
	
	if ( wm.time().cycle() < 2500 || ( wm.time().cycle() > 3000 && wm.time().cycle() < 5500 ) )
        return false;

	//if ( wm.self().recovery() < rcsc::ServerParam::i().recoverInit() - 0.002 )
	if ( wm.self().stamina() < 1000 )
	{
		//const rcsc::PlayerObject * fastest = wm.interceptTable()->fastestTeammate();
		const rcsc::PlayerObject * fastest = wm.getTeammateNearestToBall( 10 );
		if( ( ! fastest ) )
			return false;
		rcsc::Vector2D point( 1000 , 1000 );
		Body_Pass::say_pass( agent , fastest->unum() , point );
		//std::cout<<"["<<wm.time().cycle()<<"] point=("<<point.x<<","<<point.y<<") => "<<fastest->unum()<<std::endl;
		//std::cout<<"subsitue request"<<std::endl;
	}
	
	//agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
}


/*----------------------------------------------------------*/
bool
Bhv_Tactics::substitueKick( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if( wm.gameMode().type() != rcsc::GameMode::PlayOn )
		return false;
	
	if ( wm.time().cycle() < 2500 || ( wm.time().cycle() > 3000 && wm.time().cycle() < 5500 ) )
        return false;

    if( wm.audioMemory().pass().empty()
     || wm.audioMemory().pass().front().receiver_ != wm.self().unum() )
    {
        return false;
    }

    rcsc::Vector2D receive_pos = wm.audioMemory().pass().front().receive_pos_;
    
    if( receive_pos.x < 51.0 )
    {
		return false;
	}
    
    if( receive_pos.y < 33.0 )
    {
		return false;
	}
    
    if( wm.self().pos().x > 20.0 )
    {
		return false;
	}

	rcsc::Vector2D out( wm.ball().pos().x , 34.0 );
	if( wm.ball().pos().y < 0.0 )
		out.y *= -1.0;
	double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	while( wm.ball().pos().dist( out ) < 20.0 )
	{
		out.x++;
	}
    
    if( Body_SmartKick( out, rcsc::ServerParam::i().ballSpeedMax() , rcsc::ServerParam::i().ballSpeedMax() * 0.99 , 3 ).execute( agent ) )
    {
		std::cout<<"substitue kick"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( out ) );
        return true;
	}

	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return false;
}

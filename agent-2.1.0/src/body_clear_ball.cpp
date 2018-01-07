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

#include "body_pass.h"
#include "bhv_predictor.h"
#include "body_clear_ball.h"
#include "body_kick_one_step.h"
#include "body_smart_kick.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/

bool
Body_ClearBall::execute( rcsc::PlayerAgent * agent )
{
	
    if ( ! agent->world().self().isKickable() )
        return false;

	bool cleared = furthestTM( agent );
	
	if( ! cleared )
		cleared = bestAngle( agent );
	else
		return true;
	
	if( ! cleared )
		cleared = clear( agent );
	else
		return true;
	
	if( ! cleared )
		return false;
	
    return false;
}


/*-------------------------------------------------------------------*/

bool
Body_ClearBall::furthestTM( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	rcsc::Vector2D target_point;
	int number = 11;
	
	std::vector< rcsc::PlayerObject * >teammates;
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{	
		if( (! (*tm)) || (*tm)->isGhost() )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < 30 && (*tm)->pos().absY() < 17 )
			continue;
		if( (*tm)->pos().x < wm.ball().pos().x + 5.0 )
			continue;
			
		rcsc::Circle2D danger( wm.self().pos() , 10 );
		if( danger.contains( (*tm)->pos() ) )
			continue;
		
		/*
		rcsc::Circle2D around( (*tm)->pos() , 2 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		*/
		
		//rcsc::AngleDeg deg = ( (*tm)->pos() - wm.ball().pos() ).th();
		//rcsc::Sector2D pass( wm.ball().pos() , 0.5 , 20.0 , deg - 12.5 , deg + 12.5 );
		if( Body_Pass::exist_opponent3( agent , (*tm)->pos() ) )
			continue;
	
		/*
		target_point = (*tm)->pos();
		number = (*tm)->unum();
		if (! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
			if (! Body_KickOneStep( target_point , first_speed , true ).execute( agent ) )
				continue;
		cleared = true;
		*/
		//std::cout<<"["<<wm.time().cycle()<<"] cleared 1"<<std::endl;
		teammates.push_back( *tm );
	}
	
	bool cleared = false;
	
	if( teammates.size() < 1 )
		return false;
		
	for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
	{
		for( uint j = i ; j < teammates.size() ; j++ )
		{
			if( teammates[i]->pos().x < teammates[j]->pos().x )
			{
				rcsc::PlayerObject * tmp = teammates[i];
				teammates[i] = teammates[j];
				teammates[j] = tmp;
			}
		}
	}
	
	for( uint i = 0 ; i < teammates.size() ; i++ )
	{
		target_point = teammates[i]->pos();
		number = teammates[i]->unum();
		double first_speed = Body_Pass::first_speed( agent , target_point , 'd' );
		if( Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
		{
			std::cout<<"["<<wm.time().cycle()<<"] cleared 1"<<std::endl;
			cleared = true;
			break;
		}
		else
		{
			if( agent->world().existKickableOpponent() )
		    {
		        if( Body_KickOneStep( target_point , rcsc::ServerParam::i().ballSpeedMax() ).execute( agent ) )
		        {
					std::cout<<"["<<wm.time().cycle()<<"] cleared 1"<<std::endl;
					cleared = true;
					break;
				}
			}
		}
	}
	
	if( cleared )
	{
	    if ( agent->config().useCommunication() )
	    {
			Body_Pass::say_pass( agent , number , target_point );
	    }
		
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	    return true;
	}
	else
		return false;
}


/*-------------------------------------------------------------------*/

bool
Body_ClearBall::bestAngle( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	rcsc::Vector2D target_point;
	int number = 11;
	bool cleared = false;
	
	rcsc::Vector2D left( -52.5 , -34.0 );
	rcsc::Vector2D right( -52.5 , 34.0 );
	double min = std::max( -110.0 , ( left - wm.ball().pos() ).th().degree() );
	double max = std::min( 110.0 , ( right - wm.ball().pos() ).th().degree() );
	double angle = min;
	/*
	while( angle < max )
	{
		rcsc::Vector2D clear_point( 30.0 , angle );
		if(! Body_Pass::exist_opponent3( agent , clear_point ) )
		{
			const rcsc::PlayerObject * teammate = wm.getTeammateNearestTo( clear_point , 10 , NULL );
			if( teammate )
				number = teammate->unum();
			double first_speed = rcsc::ServerParam::i().maxPower();
			if( Body_SmartKick( clear_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
			{
				std::cout<<"["<<wm.time().cycle()<<"] cleared 2"<<std::endl;
				cleared = true;
				break;
			}
			else
			{
				if( agent->world().existKickableOpponent() )
			    {
			        if( Body_KickOneStep( clear_point , rcsc::ServerParam::i().ballSpeedMax() ).execute( agent ) )
			        {
						std::cout<<"["<<wm.time().cycle()<<"] cleared 2"<<std::endl;
						cleared = true;
						break;
					}
				}
			}
		}
		angle += 1.0;
	}
	*/
	std::vector< std::pair< rcsc::Vector2D , double > >points;
	
	while( angle < max )
	{
		rcsc::Vector2D clear_point( 30.0 , angle );
		if(! Body_Pass::exist_opponent3( agent , clear_point ) )
		{
			double dist = -1000.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( clear_point , 5 , &dist );
			const rcsc::PlayerObject * nearestTM = wm.getTeammateNearestTo( clear_point , 5 , NULL );
			if( nearest && dist > 0.0 && nearestTM &&
				Bhv_Predictor::predict_player_reach_cycle( nearest, clear_point, 0.5, 0.0, 1, 1, 0, false )
				>= Bhv_Predictor::predict_player_reach_cycle( nearestTM , clear_point, 0.5, 0.0, 1, 1, 0, false ) )
			{
				points.push_back( std::make_pair( clear_point , dist / ( clear_point.absY() + 1.0 ) ) );
			}
		}
		angle += 10.0;
	}

	if( points.size() < 1 )
		return false;
	
	rcsc::Vector2D best;
	double Max = -1000.0;
	for( uint i = 0 ; i < points.size() ; i++ )
	{
		if( points[i].second > Max )
		{
			Max = points[i].second;
			best = points[i].first;
		}
	}

	const rcsc::PlayerObject * teammate = wm.getTeammateNearestTo( best , 10 , NULL );
	if( teammate )
		number = teammate->unum();
	double first_speed = rcsc::ServerParam::i().maxPower();
	if( Body_SmartKick( best , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
	{
		std::cout<<"["<<wm.time().cycle()<<"] cleared 2"<<std::endl;
		cleared = true;
	}
	else
	{
		if( agent->world().existKickableOpponent() )
	    {
	        if( Body_KickOneStep( best , first_speed ).execute( agent ) )
	        {
				std::cout<<"["<<wm.time().cycle()<<"] cleared 2"<<std::endl;
				cleared = true;
			}
		}
	}
	
	if( cleared )
	{
		if ( agent->config().useCommunication() )
		{
			Body_Pass::say_pass( agent , number , target_point );
		}
		
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}
	else
		return false;
}

/*-------------------------------------------------------------------*/

bool
Body_ClearBall::clear( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	rcsc::Vector2D target_point;
	int number = 11;
	bool cleared = false;
	
	rcsc::AngleDeg theta = 30;
	if( wm.ball().pos().y > 0 )
		theta = -30;
	target_point = wm.ball().pos() + rcsc::Vector2D::polar2vector( 25 , theta );
	const rcsc::PlayerObject * teammate = wm.getTeammateNearestTo( target_point , 10 , NULL );
	if( teammate )
		number = teammate->unum();
	double first_speed = rcsc::ServerParam::i().maxPower();
	if( Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
	{
		std::cout<<"["<<wm.time().cycle()<<"] cleared 3"<<std::endl;
		cleared = true;
	}
	else
	{
		if( agent->world().existKickableOpponent() )
	    {
	        if( Body_KickOneStep( target_point , first_speed ).execute( agent ) )
	        {
				std::cout<<"["<<wm.time().cycle()<<"] cleared 3"<<std::endl;
				cleared = true;
			}
		}
	}
    
    if( cleared )
    {
	    if ( agent->config().useCommunication() )
	    {
			Body_Pass::say_pass( agent , number , target_point );
	    }
		
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	    return true;
	}
	else
		return false;
}

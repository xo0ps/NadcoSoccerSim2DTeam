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
#include "body_direct_pass2.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "bhv_predictor.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/

bool
Body_DirectPass2::execute( rcsc::PlayerAgent * agent )
{

	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return false;
        
    rcsc::Vector2D target_point;
    int receiver = 0;
    
    if( ! evaluate( agent , &target_point , &receiver ) )
		return false;
    
    double first_speed = Body_Pass::first_speed( agent , target_point , 't' );
    
	//if(! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 2 ).execute( agent ) )
	if(! Body_KickOneStep( target_point , first_speed ).execute( agent ) )
	{
		//if( agent->world().existKickableOpponent() )
	    {
	        Body_KickOneStep( target_point , first_speed ).execute( agent );
	    }
	}

    Body_Pass::say_pass( agent , receiver , target_point );
    //std::cout<<"["<<wm.time().cycle()<<"] point=("<<target_point.x<<","<<target_point.y<<") => "<<receiver<<std::endl;
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    return true;
}

/*-------------------------------------------------------------------*/

bool
Body_DirectPass2::evaluate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , int * unum )
{

	const rcsc::WorldModel & wm = agent->world();
	
	if(! ( target_point && unum ) )
		return false;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	int receiver;

	std::vector< pass >candidates;
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 20 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.absY() > 31.0 )
			continue;
		if( tm_pos.x < my_pos.x - 25.0 )
			continue;
		receiver = (*tm)->unum();		
		double dist = my_pos.dist( tm_pos );
		if( dist < 7.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( Body_Pass::exist_opponent3( agent , tm_pos ,true ) )
			continue;
		double value = 10.0;
		double distance = -1.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		if( distance > 0.0 )
			value *= distance;
		value *= tm_pos.x;
		value /= ( std::fabs( tm_pos.y - my_pos.y ) + 1.0 );

		p.point = tm_pos;
		p.value = value;
		p.unum = receiver;
		candidates.push_back( p );
	}
	
	if( candidates.size() < 1 )
		return false;
		
	pass best;
	best.point = rcsc::Vector2D( 0.0 , 0.0 );
	best.value = -10000.0;
	best.unum = 11;
	
	for( uint i = 0 ; i < candidates.size() ; i++ )
	{
		if( candidates[i].value > best.value )
		{
			best.point = candidates[i].point;
			best.value = candidates[i].value;
			best.unum = candidates[i].unum;
		}
	}
	
    * target_point = best.point;
	* unum = best.unum;
	std::cout<<wm.time().cycle()<<" back pass => "<<best.unum<<std::endl;
	return true;
}


/*--------------------------------------------------------------*/

bool
Body_DirectPass2::test( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point )
{		

}

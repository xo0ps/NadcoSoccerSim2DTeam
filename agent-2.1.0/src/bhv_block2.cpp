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

#include "strategy.h"
#include "sample_player.h"
#include "body_pass.h"
#include "bhv_block2.h"
#include "body_go_to_point.h"
#include "bhv_cross_move.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_Block2::execute( rcsc::PlayerAgent * agent )
{	

    //std::cout<<"block"<<std::endl;
    const rcsc::WorldModel & wm = agent->world();
	
	
	if( ! wm.existKickableOpponent() )
		return false;
			
	const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

	if( self_min < mate_min && self_min < opp_min - 6 )
		return false;
	
	if( mate_min < opp_min - 6 && mate_min < self_min )
		return false;

	rcsc::Vector2D final( -100.0 , -100.0 );
	rcsc::Vector2D target( -100.0 , -100.0 );
	rcsc::Vector2D block_point = wm.ball().pos();
	int unum = 0;
	std::vector< Block >teammates;
	
	if( ( wm.ball().pos().y < 0.0 && wm.ball().pos().x < 20.0 && wm.self().unum() == 2 ) ||
		( wm.ball().pos().y < 0.0 && wm.ball().pos().x > 20.0 && wm.self().unum() == 11 ) ||
		( wm.ball().pos().y >= 0.0 && wm.ball().pos().x <= 20.0 && wm.self().unum() == 3 ) ||
		( wm.ball().pos().y >= 0.0 && wm.ball().pos().x >= 20.0 && wm.self().unum() == 10 ) )
	{
		const rcsc::AbstractPlayerCont & team = wm.allTeammates();
		const rcsc::AbstractPlayerCont::const_iterator team_end = team.end();
		for( rcsc::AbstractPlayerCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
		{
			if( (! (*tm)) )
				continue;
			if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
				continue;
			if( (*tm)->pos().dist( target ) > 30.0 )
				continue;
			if( (*tm)->pos().x > target.x + 5.0 )
				continue;
			
			rcsc::Vector2D end( -52.5 , block_point.y );
			if( wm.ball().pos().x < -37.0 )
				end.assign( -52.5 , 0.0 );
			
			rcsc::Line2D route( end , block_point );
			rcsc::Line2D dist( route.perpendicular( (*tm)->pos() ) );
			target = dist.intersection( route );
			if( target.absX() > 50.0 || target.absY() > 25.0 )
				continue;
			while( target.x > block_point.x )
				target.x--;
			
			double diff = fabs( (*tm)->pos().y - target.y );
			double score = 10.0;
			score /= ( diff + 1.0 );
			
			diff = target.absX() - (*tm)->pos().absX();
			score *= ( 10 - diff );
			block.target = target;
			block.unum = (*tm)->unum();
			block.value = score;
			teammates.push_back( block );
		}
		
		if( teammates.size() < 1 )
		{
			return false;
		}
		
		for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
		{
			for( uint j = i ; j < teammates.size() ; j++ )
			{
				if( teammates[i].value < teammates[j].value )
				{
					Block tmp;
					tmp.target = teammates[i].target;
					tmp.unum = teammates[i].unum;
					tmp.value = teammates[i].value;
					teammates[i].target = teammates[j].target;
					teammates[i].unum = teammates[j].unum;
					teammates[i].value = teammates[j].value;
					teammates[j].target = tmp.target;
					teammates[j].unum = tmp.unum;
					teammates[j].value = tmp.value;
				}
			}	
		}
		
		unum = teammates[0].unum;
		final = teammates[0].target;
		if( unum == 0 )
		{
			return false;
		}
			
		if( unum != wm.self().unum() )
		{
			std::cout<<"Blocker => "<<unum<<std::endl;
			Body_Pass::say_pass( agent , unum , final );
			return false;
		}
	}
	else
	{
		//not calculator and blocker
		return false;
	}
	
	if( ( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.25 ) || wm.self().isFrozen() )
	{
		std::cout<<"Blocker stamina error => "<<teammates[1].unum<<std::endl;
		Body_Pass::say_pass( agent , teammates[1].unum , teammates[1].target );
		return false;
	}
	
	/*
	if( wm.self().pos().x > target.x + 1.5 )
	{
		std::vector< rcsc::PlayerObject * >guards;
		guards.clear();
		const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
		const rcsc::PlayerPtrCont::const_iterator end = team.end();
		for ( rcsc::PlayerPtrCont::const_iterator it = team.begin(); it != end; ++it )
		{
			if(! (*it) )
				continue;
			if( (*it)->goalie() || (*it)->unum() > 11 || (*it)->unum() < 2 )
				continue;
			if( (*it)->pos().x < wm.ball().pos().x )
				guards.push_back( (*it) );
		}
		
		if( guards.size() > 0 )
		{
			for( uint i = 0 ; i < guards.size() - 1 ; i++ )
			{
				for( uint j = i ; j < guards.size() ; j++ )
				{
					if( fabs( guards[i]->pos().y - target.y ) > fabs( guards[j]->pos().y - target.y ) )
					{
						rcsc::PlayerObject * tmp = guards[i];
						guards[i] = guards[j];
						guards[j] = tmp;
					}
				}
			}
			
			Body_Pass::say_pass( agent , guards[0]->unum() , target );
			std::cout<<"guard is "<<guards[0]->unum()<<std::endl;
		}
	}
	*/
	double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
	static int wait = 0;
	
	std::cout<<"["<<wm.time().cycle()<<"] blocker is "<<wm.self().unum()<<std::endl;

	if( Body_GoToPoint( final , 1.0 , dash_power , 1 , false , true , 40.0  ).execute( agent ) )
	{
		wait = 0;
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}
	
	wait++;
	
	if( wait >= 1 )
	{
		Body_Intercept().execute( agent );
		wait = 0;
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //agent->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );
        return true;
	}
	
	return false;
}

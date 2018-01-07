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
#include "bhv_block.h"
#include "bhv_block2.h"
#include "body_go_to_point.h"
#include "bhv_cross_move.h"
#include "bhv_basic_tackle.h"
#include "body_intercept.h"
#include "intention_receive.h"

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
Bhv_Block::execute( rcsc::PlayerAgent * agent )
{	

	//if ( ! SamplePlayer::i().block() )
	if ( ! Strategy::i().block() )
    {
		return false;
    }
    
    //return Bhv_Block2().execute( agent );
    
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

	if( newBlock( agent ) )
		return true;
	
	static bool wasBlocking = false;
	double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
	static rcsc::Vector2D target = wm.ball().pos();
	static double score = 10.0;
	static int wait = 0;
	
	int unum = 0;
	std::vector< std::pair< int , double > >teammates;
	teammates.clear();
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
		if( (*tm)->pos().x < target.x - 25.0 )
			continue;
		double diff = fabs( (*tm)->pos().y - target.y );
		if( diff > 11.0 )
			continue;
		
		score /= ( diff + 1.0 );
		diff = target.absX() - (*tm)->pos().absX();
		score *= diff;
		teammates.push_back( std::make_pair( (*tm)->unum() , score ) );
	}
	
	if( teammates.size() < 1 )
		return false;
	
	for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
	{
		for( uint j = i ; j < teammates.size() ; j++ )
		{
			if( teammates[i].second < teammates[j].second )
			{
				std::pair< int , double >tmp = teammates[i];
				teammates[i] = teammates[j];
				teammates[j] = tmp;
			}
		}	
	}
	/*
	std::vector< std::pair< int , double > >bests;
	bests.push_back( teammates[0] );
	bests.push_back( teammates[1] );
	bests.push_back( teammates[2] );
	
	for( uint i = 0 ; i < bests.size() ; i++ )
	{
		double score = bests[i].second;
		const rcsc::AbstractPlayerObject * tm = wm.teammate( bests[i].first );
		double diff;
		if( tm )
			diff = target.absX() - tm->pos().absX();
		else
			continue;
		score *= ( 10.0 - diff );
		bests[i].second = score;
	}
	
	std::cout<<"block2"<<std::endl;
	
	for( uint i = 0 ; i < bests.size() - 1 ; i++ )
	{
		for( uint j = i ; j < bests.size() ; j++ )
		{
			if( bests[i].second < bests[j].second )
			{
				std::pair< int , double >tmp = bests[i];
				bests[i] = bests[j];
				bests[j] = tmp;
			}
		}	
	}
	*/
	
	//unum = bests[0].first;
	unum = teammates[0].first;
	
	if( unum == wm.self().unum() && wasBlocking )
	{
		if( Body_GoToPoint( target , 1.0 , dash_power , 1 , false , true , 40.0  ).execute( agent ) )
		{
			wait = 0;
			agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
			return true;
		}
		
		wait++;
		
		if( wait >= 1 )
		{
			if( wm.self().pos().dist( wm.ball().pos() ) > 8.0 )
			{
				std::cout<<"blocker far "<<wm.self().unum()<<std::endl;
			}
			Body_Intercept().execute( agent );
			wait = 0;
	        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	        return true;
		}
	}
	else
	if( unum == wm.self().unum() )
	{
		if( teammates[0].second > score * 0.8 )
		{
			if( Body_GoToPoint( target , 1.0 , dash_power , 1 , false , true , 40.0  ).execute( agent ) )
			{
				wait = 0;
				wasBlocking = true;
				agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
				return true;
			}
			
			wait++;
			
			if( wait >= 1 )
			{
				if( wm.self().pos().dist( wm.ball().pos() ) > 8.0 )
				{
					std::cout<<"blocker far "<<wm.self().unum()<<std::endl;
				}
				Body_Intercept().execute( agent );
				wait = 0;
		        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		        return true;
			}
		}
	}
	else
	{
		return false;
	}
	
	score = teammates[0].second;

	/*
	double dist = -0.0003 * std::pow( wm.ball().pos().absX() , 3 )
				 + 0.0099 * std::pow( wm.ball().pos().absX() , 2 )
				 + 0.1109 * std::pow( wm.ball().pos().absX() , 1 )
				 + 5.0909;
	
	if( wm.ball().pos().x > -20 )
	{
		//target += rcsc::Vector2D( -dist * 0.9 , 0.0 );
		target += rcsc::Vector2D( -4.0 , 0.0 );
	}
	else
	if( wm.ball().pos().x <= -20.0 && wm.ball().pos().x > -36.0 )
	{
		//target += rcsc::Vector2D( -dist * 0.6 , 0.0 );
		target += rcsc::Vector2D( -3.0 , 0.0 );
	}
	else
	{
		rcsc::Vector2D end( -52 , 0.0 );
		rcsc::AngleDeg deg = ( end - block_point ).th();
		//target += rcsc::Vector2D::polar2vector( dist * 0.6 , deg );
		//target += rcsc::Vector2D::polar2vector( 2.0 , deg );
		//target += rcsc::Vector2D::polar2vector( 4.0 , deg );
		target = block_point;
		std::cout<<"block => "<<target.x<<" "<<target.y<<std::endl;
	}
	*/	
	
	rcsc::Vector2D end( -52.5 , wm.ball().pos().y );
	if( wm.ball().pos().x < -37.0 )
		end.assign( -52.5 , 0.0 );
	rcsc::Line2D route( end , wm.ball().pos() );
	
	rcsc::Line2D my_dist( route.perpendicular( wm.self().pos() ) );
	my_dist = route.perpendicular( wm.self().pos() );
	target = my_dist.intersection( route );
	if( target.absX() > 52.0 || target.absY() > 28.0 )
		target.assign( wm.ball().pos().x - 4.0 , wm.ball().pos().y );
	while( target.x > wm.ball().pos().x )
		target.x--;
	
	if( ( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.25 ) || wm.self().isFrozen() )
	{
		const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
		const rcsc::PlayerPtrCont::const_iterator end = team.end();
		for ( rcsc::PlayerPtrCont::const_iterator it = team.begin(); it != end; ++it )
		{
			if(! (*it) )
				continue;
			if( (*it)->goalie() || (*it)->unum() > 11 || (*it)->unum() < 2 )
				continue;
			if( (*it)->pos().x < wm.ball().pos().x )
			{
				std::cout<<"Blocker stamina error => "<<(*it)->unum()<<std::endl;
				Body_Pass::say_pass( agent , (*it)->unum() , target );
				return false;
			}
		}
		
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
	
	if( fabs( wm.self().pos().y - target.y ) < 5.0 && wm.self().pos().x < wm.ball().pos().x - 2.0 )
	{
		//Body_Intercept().execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}
	
	if( wm.self().pos().absY() > wm.ball().pos().absY() && wm.ball().pos().x < -30.0 )
	{
		//Body_Intercept( false ).execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}
	
	//double dash_power = rcsc::ServerParam::i().maxDashPower();
	//double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
	//static int wait = 0;
	
	std::cout<<"["<<wm.time().cycle()<<"] blocker is "<<wm.self().unum()<<std::endl;

	if( Body_GoToPoint( target , 1.0 , dash_power , 1 , false , true , 40.0  ).execute( agent ) )
	{
		wait = 0;
		wasBlocking = true;
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}
	
	wait++;
	
	if( wait >= 1 )
	{
		if( wm.self().pos().dist( wm.ball().pos() ) > 8.0 )
		{
			std::cout<<"blocker far "<<wm.self().unum()<<std::endl;
		}
		Body_Intercept().execute( agent );
		wasBlocking = true;
		wait = 0;
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
	}
	
	return false;
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_Block::newBlock( rcsc::PlayerAgent * agent )
{	

	/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return false;
	
}

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
#include "bhv_cross_move.h"
#include "bhv_through_pass_cut.h"
#include "body_pass.h"
#include "body_go_to_point.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_player_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_ThroughPassCut::execute( rcsc::PlayerAgent * agent )
{	

	//if ( ! SamplePlayer::instance().hassle() )
	if ( ! Strategy::i().th_cut() )
    {
		return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();
			
	if( wm.self().isFrozen() )
		return false;

	//if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.55 )
	//	return false;

	std::vector< std::pair< double , int > >teammates;
	teammates.clear();
	const rcsc::AbstractPlayerCont & team = wm.allTeammates();
	const rcsc::AbstractPlayerCont::const_iterator team_end = team.end();
	for( rcsc::AbstractPlayerCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->unum() > 6 )
			continue;
		if( fabs( (*tm)->pos().x - wm.self().pos().x ) > 20.0 )
			continue;
		teammates.push_back( std::make_pair( (*tm)->pos().y , (*tm)->unum() ) );
	}
	
	if( teammates.size() < 2 )
		return false;
	
	for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
	{
		for( uint j = i ; j < teammates.size() ; j++ )
		{
			if( fabs( teammates[i].first ) < fabs( teammates[j].first ) )
			{
				std::pair< double , int >tmp;
				tmp.first = teammates[i].first;
				teammates[i].first = teammates[j].first;
				teammates[j].first = tmp.first;
				tmp.second = teammates[i].second;
				teammates[i].second = teammates[j].second;
				teammates[j].second = tmp.second;
			}
		}
	}
	
	int unum1 = teammates[0].second;
	int unum2 = teammates[1].second;
	
	if( unum1 != wm.self().unum() && unum2 != wm.self().unum() )
	{
		return false;
	}	
	
	std::vector< rcsc::Vector2D >opponents;
	opponents.clear();
	const rcsc::PlayerPtrCont & opp_team = wm.opponentsFromSelf();
	const rcsc::PlayerPtrCont::const_iterator opp_end = opp_team.end();
	for( rcsc::PlayerPtrCont::const_iterator opp = opp_team.begin(); opp != opp_end; opp++ )
	{
		if( (! (*opp)) )
			continue;
		if( (*opp)->pos().x - wm.self().pos().x > 20.0 )
		//if( (*opp)->pos().x > wm.self().pos().x + 30.0 )
			continue;
		opponents.push_back( (*opp)->pos() );
	}
	
	if( opponents.size() < 2 )
		return false;
	
	for( uint i = 0 ; i < opponents.size() - 1 ; i++ )
	{
		for( uint j = i ; j < opponents.size() ; j++ )
		{
			if( fabs( opponents[i].y ) < fabs( opponents[j].y ) )
			{
				rcsc::Vector2D tmp = opponents[i];
				opponents[i] = opponents[j];
				opponents[j] = tmp;
			}
		}
	}
	
	//const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( point , 5 , NULL );
	//const rcsc::PlayerObject * opp = wm.getOpponentNearestToSelf( 5 );
	//if( ! opp )
	//	return false;
	
	//const rcsc::AbstractPlayerObject * opp1 = wm.opponent( opponents[0].second );
	//const rcsc::AbstractPlayerObject * opp2 = wm.opponent( opponents[1].second );

	const rcsc::PlayerObject * opp1 = wm.getOpponentNearestTo( opponents[0] , 10 , NULL );
	const rcsc::PlayerObject * opp2 = wm.getOpponentNearestTo( opponents[1] , 10 , NULL );
	
	bool first = false;
	if( wm.self().pos().dist( opponents[0] ) < wm.self().pos().dist( opponents[1] ) )
		first = true;
	
	if( first )
	{
		if( ! opp1 )
		{
			Body_GoToPoint( wm.self().pos() , 0.5 , rcsc::ServerParam::i().maxDashPower() ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToPoint( opponents[0] ) );
			return false;
		}
	}
	else
	{
		if( ! opp2 )
		{
			Body_GoToPoint( wm.self().pos() , 0.5 , rcsc::ServerParam::i().maxDashPower() ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToPoint( opponents[1] ) );
			return false;
		}
	}
	
	rcsc::Vector2D point = opp1->pos();
	if( ! first )
		point = opp2->pos();
	
	const rcsc::PlayerPtrCont & team1 = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team1_end = team1.end();
	for( rcsc::PlayerPtrCont::const_iterator tm = team1.begin(); tm != team1_end; tm++ )
	{
		if( (! (*tm)) )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x > point.x + 10.0 )
			continue;
		if( (*tm)->pos().dist( point ) < wm.self().pos().dist( point ) )
		{
			//return false;
		}		
	}
	
	rcsc::AngleDeg body = opp1->body();
	if( ! first )
		body = opp2->body();
	
	double dist = -0.0003 * std::pow( wm.ball().pos().absX() , 3 )
				 + 0.0099 * std::pow( wm.ball().pos().absX() , 2 )
				 + 0.1109 * std::pow( wm.ball().pos().absX() , 1 )
				 + 5.0909;

	

	/*
	if( point.x > 0.0 )
	{
		Body_GoToPoint( wm.self().pos() , 0.5 , rcsc::ServerParam::i().maxDashPower() ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( point ) );
		return false;
	}
	*/
	if( fabs( wm.self().pos().y - point.y ) > 5.0 )
	{
		if( wm.self().pos().x > point.x - 1.0 )
		{
			rcsc::Vector2D target( -52.5 , 0.0 );
			rcsc::Line2D route( target , point );
			rcsc::Line2D my_dist( route.perpendicular( wm.self().pos() ) );
			point = my_dist.intersection( route );
		}
		if( wm.self().pos().x < point.x )
		{
			rcsc::Vector2D target( -52.5 , point.y );
			rcsc::Line2D route( target , point );
			rcsc::Line2D my_dist( route.perpendicular( wm.self().pos() ) );
			point = my_dist.intersection( route );
			while( point.x > wm.self().pos().x )
				point += rcsc::Vector2D( -1.0 , 0.0 );
		}
	}
	//else
	{
		//point += rcsc::Vector2D( -dist * 0.7 , 0.0 );
		point += rcsc::Vector2D( -5.0 , 0.0 );
	}
	
	
	if( point.x > 5.0 )
		point.x = -1.0;
			
	
	double offside = wm.ourDefenseLineX();
	if( fabs( wm.self().pos().x - offside ) < 2.0 )
	{
		point += rcsc::Vector2D::polar2vector( 1.0 , 0.0 );
	}
			
	//double dash_power = rcsc::ServerParam::i().maxDashPower();
	double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
	static int wait = 0;
	
	//std::cout<<"["<<wm.time().cycle()<<"] "<<wm.self().unum()<<" marking"<<std::endl;
	if( Body_GoToPoint( point , 0.8 , dash_power , 1 , false , true , 40.0  ).execute( agent ) )
	{
		wait = 0;
		if( first )
			agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp1 ) );
		else
			agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp2 ) );
		return true;
	}
	
	wait++;
	
	if( wait >= 1 )
	{
		wait = 0;
		//rcsc::Body_TurnToAngle( body ).execute( agent );
		rcsc::Body_TurnToBall().execute( agent );
		if( first )
			agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp1 ) );
		else
			agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp2 ) );
        return true;
	}
	
	if( first )
		agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp1 ) );
	else
		agent->setNeckAction( new rcsc::Neck_TurnToPlayerOrScan( opp2 ) );
	
	return false;
}

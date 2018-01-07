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

#include "sample_player.h"
#include "strategy.h"
#include "bhv_cross_move.h"
#include "bhv_hassle.h"
#include "body_pass.h"
#include "body_go_to_point.h"
#include "intention_receive.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_Hassle::execute( rcsc::PlayerAgent * agent )
{	

	//if ( ! SamplePlayer::instance().hassle() )
	if ( ! Strategy::i().hassle() )
    {
		return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();
	
	/*
	rcsc::Vector2D point( 100 , 100 );
    if ( wm.audioMemory().passTime().cycle() >= wm.time().cycle() - 1
         && ( ! wm.audioMemory().pass().empty() )
       )
    {
		point = wm.audioMemory().pass().front().receive_pos_;
		if( wm.audioMemory().pass().front().receiver_ == wm.self().unum() )
		{
			std::cout<<wm.self().unum()<<" hassle say"<<std::endl;
			Body_GoToPoint( point, wm.self().pos().dist( point ) * 0.05 , rcsc::ServerParam::i().maxDashPower() ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
			return true;
		}
	}
	*/
	
	
	//if( wm.existKickableTeammate() )
	//	return false;
			
	if( wm.self().isFrozen() )
		return false;

	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.55 )
		return false;

	std::vector< std::pair< rcsc::Vector2D , int > >opponents;
	
	const rcsc::PlayerPtrCont & team = wm.opponentsFromBall();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  opp = team.begin() ; opp != team_end; opp++ )
	{
		if( (! (*opp)) )
			continue;
		if( (*opp)->goalie() || (*opp)->unum() > 11 || (*opp)->unum() < 2 )
			continue;
		if( (*opp)->pos().x > -30 )
			continue;
		if( (*opp)->pos().dist( wm.ball().pos() ) < 5.0 )
			continue;
		if( (*opp)->isKickable() )
			continue;
		opponents.push_back( std::make_pair( (*opp)->pos() , (*opp)->unum() ) );
	}

/*
	std::vector< std::pair< int , double > >table;
	const std::vector< std::pair< rcsc::Vector2D , int > >::const_iterator team_end1 = opponents.end();
	for ( std::vector< std::pair< rcsc::Vector2D , int > >::const_iterator  opp = opponents.begin() ; opp != team_end1; opp++ )
	{
		rcsc::Vector2D pos = opp->first;
		int unum = opp->second;
		double value = 0;
		value += std::max( 1000.0 / pos.dist( wm.self().pos() ) , 1000.0 );
		//value += wm.self().stamina();
		rcsc::Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
		value += std::max( 1000.0 / pos.dist( home_pos ) , 1000.0 );
		//value += std::max( 1000.0 / pos.dist( wm.ball().pos() ) , 1000.0 );
		
		table.push_back( std::make_pair( unum , value ) );
	}


	std::vector< std::pair< int , int > >pair;
	const std::vector::const_iterator team_end = table.end();
	for ( std::vector::const_iterator  opp = table.begin() ; opp != team_end; opp++ )
	{
		int opp = opp->first;
		double value = opp->second;
		const rcsc::PlayerPtrCont & t = wm.teammatesFromSelf();
		const rcsc::PlayerPtrCont::const_iterator t_end = t.end();
		for( rcsc::PlayerPtrCont::const_iterator  tm = t.begin(); tm != t_end; tm++ )
		{
			if( (! (*tm)) )
				continue;
			if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
				continue;
			

		
		table.push_back( std::make_pair( unum , value ) );
	}


	int tmp = 0;
	double val = -1000.0;
	for ( std::vector< std::pair< int , double > >::const_iterator  opp = table.begin() ; opp != table.end() ; opp++ )
		if( opp->second > val )
		{
			val = opp->second;
			tmp = opp->first;
		}
*/

	std::vector< std::pair< int , int > >pair;
	std::vector< double >teammates;
	std::vector< int >opponent;
	std::vector< int >unums;
	const std::vector< std::pair< rcsc::Vector2D , int > >::const_iterator team_end1 = opponents.end();
	for ( std::vector< std::pair< rcsc::Vector2D , int > >::const_iterator opp = opponents.begin(); opp != team_end1; opp++ )
	{
		rcsc::Vector2D pos = opp->first;
		int unum = opp->second;
		teammates.clear();
		const rcsc::AbstractPlayerCont & t = wm.allTeammates();
		const rcsc::AbstractPlayerCont::const_iterator t_end = t.end();
		for ( rcsc::AbstractPlayerCont::const_iterator  tm = t.begin(); tm != t_end; tm++ )
		{
			if( (! (*tm)) )
				continue;
			if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
				continue;
			teammates.push_back( (*tm)->pos().dist( pos ) );
			unums.push_back( (*tm)->unum() );
		}
		if( teammates.size() < 2 )	
			continue;
		double tmp;
		int utmp;
		for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
			for( uint j = i ; j < teammates.size() ; j++ )
				if( teammates[i] > teammates[j] )
				{
					tmp = teammates[i];
					utmp = unums[i];
					teammates[i] = teammates[j];
					unums[i] = unums[j];
					teammates[j] = tmp;
					unums[j] = utmp;
				}
		opponent.push_back( unum );
		pair.push_back( std::make_pair( unums[0] , unums[1] ) );
	}
	
	bool found = false;
	int tmp = 0;
	rcsc::Vector2D nearest_opp_pos( 100 , 100 );
	
	if( wm.self().unum() == 2 )
	{
		for( uint i = 0 ; i < opponent.size() ; i++ )
		{
			std::cout<<"( "<<pair[i].first<<" , "<<pair[i].second<<" ) => "<<opponent[i]<<std::endl;
		}
	}
	
	for( uint i = 0 ; i < opponent.size() ; i++ )
	{
		int first = pair[i].first;
		int second = pair[i].second;
		const rcsc::AbstractPlayerObject * nearest_tm1 = wm.teammate( first );
		const rcsc::AbstractPlayerObject * nearest_tm2 = wm.teammate( second );
		if( ! nearest_tm1 )
			continue;
		if( ! nearest_tm2 )
			continue;
		if( wm.self().unum() != nearest_tm1->unum() && wm.self().unum() != nearest_tm2->unum() )
			continue;
		const rcsc::AbstractPlayerObject * nearest_opp = wm.opponent( opponent[i] );
		if( ! nearest_opp )
			continue;
		nearest_opp_pos = nearest_opp->pos();
		tmp = nearest_opp->unum();
		found = true;
		break;
	}
	if( !found )
		return false;
	
	//std::cout<<wm.self().unum()<<" hassle=> "<<tmp<<std::endl;
	rcsc::AngleDeg block_angle = ( wm.ball().pos() - nearest_opp_pos ).th();
    rcsc::Vector2D block_point = nearest_opp_pos + rcsc::Vector2D::polar2vector( 2.0 , block_angle );
    //block_point += rcsc::Vector2D( -0.5 , 0.0 );

    double dash_power = rcsc::ServerParam::i().maxDashPower();
    //rcsc::Vector2D target( 52.5 , 0.0 );
	//rcsc::AngleDeg deg = ( wm.self().pos() - target ).th();
	Body_Pass::say_pass( agent , wm.self().unum() , block_point );
    
    static int wait = 0;
	if( Body_GoToPoint( block_point , 1.0 , dash_power , 2.0 , false , true , 40.0  ).execute( agent ) )
	{
		wait = 0;
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		agent->setIntention( new IntentionReceive( block_point , dash_power , 0.9, 5, wm.time() ) );
		return true;
	}
	
	wait++;
	
	if( wait >= 1 )
	{
		//std::cout<<wm.self().unum()<<" hassle2"<<std::endl;
		rcsc::Body_TurnToBall().execute( agent );
		wait = 0;
		agent->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
	}
	
	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	return false;
}

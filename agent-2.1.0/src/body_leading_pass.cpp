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

#include "body_leading_pass.h"
#include "body_pass.h"
#include "body_kick_multi_step.h"
#include "body_kick_one_step.h"
#include "body_smart_kick.h"
#include "body_intercept.h"
#include "bhv_predictor.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>

#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>


/*-------------------------------------------------------------------*/

bool Body_LeadingPass::execute( rcsc::PlayerAgent * agent )
{
	return false;
	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return false;
	
    rcsc::Vector2D target_point(50.0, 0.0);
    double first_speed = 0.0;
    int receiver = 0;

	 if( M_mode == "offendResponse" )
	 {
		if ( ! offendResponsePass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	 }
	 if( M_mode == "toCorners" )
	 {
		if ( ! toCornersPass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	 }
	 if( M_mode == "playOn" )
	 {
		if ( ! evaluate( agent , &target_point, &first_speed, &receiver ) )
			return false;
	 }
	
	if( receiver == 0 )
		return false;
		
	/*
    Body_KickMultiStep( target_point, first_speed, false ).execute( agent );
	
    int kick_step = ( wm.gameMode().type() != rcsc::GameMode::PlayOn
                      && wm.gameMode().type() != rcsc::GameMode::GoalKick_
                      ? 1
                      : 3 );
	
    if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, kick_step ).execute( agent ) )
    {
            first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
            Body_KickOneStep( target_point, first_speed ).execute( agent );

        if ( wm.gameMode().type() != rcsc::GameMode::PlayOn
             && wm.gameMode().type() != rcsc::GameMode::GoalKick_ )
        {
            first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
            Body_KickOneStep( target_point, first_speed ).execute( agent );

        }
        else
            return false;
    }
    */
    
    if (! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
		Body_KickOneStep( target_point , first_speed , true ).execute( agent );
	
	
	if ( agent->config().useCommunication() && receiver != rcsc::Unum_Unknown )
    {
		Body_Pass::say_pass( agent , receiver , target_point );
    }
	
	
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    return true;
}
/*-----------------------------------------------------------------------*/

bool Body_LeadingPass::evaluate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;

    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );

    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
	rcsc::Vector2D pass( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = pass;
		tm_value[i] = 0;
    }
    bool found = false;
    double receiver_to_target = 100.0;
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		rcsc::Vector2D ball_pos = wm.ball().pos();
		double dist = ball_pos.dist( tmpos );
		if( dist > max_dp_dist )
			continue;
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 48 || tmpos.absY() > 32 )
			continue;
	    //if ( tmpos.x < -10.0 && std::fabs( tmpos.y - ball_pos.y ) > 15.0 )
	    //    continue;
		//if( ( tmpos.x < ball_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 100.0 )
		//	continue;
		int number = (*tm)->unum();
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double dist1 = 15.0 * ball_pos.dist( tmpos ) / 20.0 - 7.0;
		if( ball_pos.dist( tmpos ) > 30.0 )
			dist1 = 15;
		if( ball_pos.dist( tmpos ) < 10.0 )
			dist1 = 0;
		double threshold = std::min( 46.0 , tmpos.x + dist1 );
		bool cont = false;
		rcsc::Vector2D front_tm( 48 , tmpos.y );
		rcsc::Line2D front( tmpos , front_tm );
		while( (! end) && ( checkFront.x <= threshold ) && ( checkFront.x <= tmpos.x + 28.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points[ number ] = pass.intersection( front );
			//rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points[ number ] ) , -30 , 30 );
			//if( ! wm.existOpponentIn( safe_area , 10 , true ) )
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( pass_points[ number ] , 5 , &dist1 );
			if( nearest
			&& Bhv_Predictor::predict_player_reach_cycle( nearest, pass_points[ number ], 1.0, 1.0, 3, 1, 1, false )
			< wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( pass_points[ number ] ) ) + 1 )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		found = true;
		/*
		rcsc::Vector2D pass_point( x , y );
		bool safe = false;
		do
		{
			pass_points[ number ] = tmpos + pass_point;
			rcsc::Sector2D safe_area( tmpos , 0.5 , 1.5 * tmpos.dist( pass_points[ number ] ) , -45 , 45 );
			if( wm.existOpponentIn( safe_area , 10 , true ) )
				pass_point.x -= 1.0;
			else
				safe = true;
		}
		while( (!safe) && (pass_point.x >= tmpos.x) );
		*/
		
		pass_points[ number ] = tmpos + checkFront;
		if( pass_points[ number ].x < ball_pos.x - 1.0 )
			continue;
		//if( pass_points[ number ].x < -35 && pass_points[ number ].absY() < 17 )
		//	continue;
		//pass_points[ number ].x = std::min( offside , pass_points[ number ].x );
		//if( pass_points[ number ].x < tmpos.x )
		//	continue;
		rcsc::Circle2D around( pass_points[ number ] , 1.0 );
		//if( wm.existOpponentIn( around , 10 , true ) )
		//	continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points[ number ] , 20 , &distance );
		tm_value[ number ] += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points[ number ].y > 31 )
			pass_points[ number ].y = 30;
		if( pass_points[ number ].y < -31 )
			pass_points[ number ].y = -30;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		rcsc::Sector2D front1( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front1 , 10 , true ) )
			tm_value[ number ] += 300;
		//tm_value[ number ] += 10.0 * (*tm)->posCount();
		/*
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 2.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		*/
		tm_value[ number ] += 10.0 * std::max( 5.0, std::fabs( pass_points[ number ].y - ball_pos.y ) );
		//rcsc::Circle2D region( ball_pos , 5 );
		//if( region.contains( tmpos ) )
		//	tm_value[ number ] -= 100;
		rcsc::AngleDeg deg = ( ball_pos - tmpos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		tm_value[ number ] +=  10.0 * count;
		
		if( pass_points[ number ].absY() > 26 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].absY() > 18 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			tm_value[ number ] -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points[ number ] += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 150 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 110 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 70 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;
	
	    receiver_to_target = tmpos.dist( pass_points[ number ] );
	}
	
	
	if( !found )
		return false;
	
	rcsc::Vector2D tempVec;
	int tempVal;
	for( int i = 0; i < 11 ; i++ )
		for( int j = i + 1; j < 12 ; j++ )
			if( tm_value [i] < tm_value[j] )
			{
				tempVec = pass_points[i];
				tempVal = tm_value[i];
				pass_points[i] = pass_points[j];
				tm_value[i] = tm_value[j];
				pass_points[j] = tempVec;
				tm_value[j] = tempVal;
			}
	
	/*
	for( int i = 0 ; i < 6 ; i++ )
		std::cout<<"["<<wm.time().cycle()<<"] => "<<tm_value[i]<<std::endl;
	*/
	
	for( int i = 0; i < 6 ; i++ )
	{
		rcsc::AngleDeg deg = ( pass_points[i] - wm.ball().pos() ).th();
		double first_speed1 = rcsc::calc_first_term_geom_series_last( 0.8 , wm.ball().pos().dist( pass_points[i] ), rcsc::ServerParam::i().ballDecay() );
		
		double dist_to_point = 1000.0;
		const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( pass_points[i] , 10 , &dist_to_point );
		if( (! receiver_tm) )
			continue;
		if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
			continue;
		if(! Body_Pass::exist_opponent3( agent , pass_points[i] ) )
		{
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			std::cout<<"leading pass => "<<receiver_tm->unum()<<std::endl;
			return true;
		}
		/*

		if(! Body_Pass::exist_opponent( agent , pass_points[i] , deg.degree() ) )
		{
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"leading pass => "<<receiver_tm->unum()<<std::endl;
			return true;
		}
		*/
		
	}

    //rcsc::Body_HoldBall().execute( agent );
	return false;
}

/*--------------------------------------------------------------*/

bool Body_LeadingPass::offendResponsePass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{
    const rcsc::WorldModel & wm = agent->world();

	if( ! wm.self().isKickable() )
		return false;
	
	if( wm.ball().pos().x > -25 )
		return false;

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
		
	double offside = wm.offsideLineX();
    
    std::vector< rcsc::Vector2D > pass_points ( 12 );
    bool found = false;
    rcsc::Vector2D tmpos( 100,100 );
    for( int i = 0 ; i < 12 ; i++ )
		pass_points[ i ] = tmpos;
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < wm.self().pos().x )
			continue;
		if( (*tm)->pos().x < offside - 20 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
			
		tmpos = (*tm)->pos();
		pass_points.push_back( tmpos );
		found = true;
	}
	
	if( ! found )
		return false;
		
	rcsc::Vector2D tempVec;
	for( int i = 0; i < 11 ; i++ )
		for( int j = i + 1; j < 12 ; j++ )
			if( wm.ball().pos().dist( pass_points[i] ) > wm.ball().pos().dist( pass_points[j] ) )
			{
				tempVec = pass_points[i];
				pass_points[i] = pass_points[j];
				pass_points[j] = tempVec;
			}
	
	for( int i = 0; i < 3 ; i++ )
	{				
		double end_speed = 1.0;
	    double first_speed1 = 10.0;
	    rcsc::Vector2D ball_pos = wm.ball().pos();
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( pass_points[i] ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.05;
	    }
	    while ( end_speed > 0.8 );

		first_speed1 = std::min( first_speed1 , rcsc::ServerParam::i().ballSpeedMax() ); 
		//first_speed1 *= ServerParam::i().ballDecay();
		
		rcsc::AngleDeg deg = ( pass_points[i] - ball_pos ).th();
		
		/*
		rcsc::Vector2D first = Vector2D::polar2vector( first_speed1, deg );
	    double next_speed = first_speed1 * ServerParam::i().ballDecay();	
		bool pass = true;
	    const rcsc::PlayerPtrCont::const_iterator opp_end = wm.opponentsFromSelf().end();
	    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(); it != opp_end; ++it )
	    {
	        if ( ! (*it) || (*it)->posCount() > 10 )
				continue;
	        if ( (*it)->isGhost() && (*it)->posCount() > 3 )
				continue;
			if( (*it)->goalie() || (*it)->unum() > 11 || (*it)->unum() < 2 )
				continue;
	        double dash = std::min( 2, (*it)->posCount() );
	        double opp2target = (*it)->pos().dist( pass_points[ i ] );
	        if ( opp2target - dash < tmpos.dist( pass_points[ number ] ) * 0.9 + 1.25 )
	            continue;
			rcsc::Vector2D dist2pos = (*it)->pos() - wm.ball().pos() - first;
			dist2pos.rotate( -deg );
			if ( dist2pos.x <= 0.0 || dist2pos.x >= ball_pos.dist( pass_points[ i ] ) )
				continue;
			double opp_dist = dist2pos.absY() - dash - ServerParam::i().defaultKickableArea() - 0.1;
			if ( opp_dist >= 0.0 )
				continue;
			else
				pass = false;
			double projection = calc_length_geom_series( next_speed, dist2pos.x, ServerParam::i().ballDecay() );
			if ( projection >= 0.0 && ( ( opp_dist / 1.0 ) > projection ) )
				continue;
			pass = false;
	    }
	    
		if( pass )
		{
			double dist_to_point = 1000.0;
			const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( pass_points[i] , 5 , &dist_to_point );
			if( (! receiver_tm) || receiver_tm->isGhost() || receiver_tm->posCount() > 3  )
				continue;
			if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
				continue;	
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"offendResponse pass => "<<receiver_tm->unum()<<std::endl;
			return true;
		}
		*/
		
		double dist_to_point = 1000.0;
		const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( pass_points[i] , 10 , &dist_to_point );
		if( (! receiver_tm) )
			continue;
		if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
			continue;
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		double dist = ball_pos.dist( pass_points[i] );
		//rcsc::Sector2D pass( ball_pos , 0.0 , dist , deg - 10 , deg + 10 );
		//rcsc::Triangle2D pass(  ball_pos,
		//						pass_points[i] + rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) , 
		//						pass_points[i] - rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , pass_points[i] ) )
		{
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			std::cout<<"offendResponse pass => "<<receiver_tm->unum()<<std::endl;
			return true;
		}
		
	}

    //rcsc::Body_HoldBall().execute( agent );
	return false;
}

/*----------------------------------------------------------------*/

bool Body_LeadingPass::toCornersPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{
	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
		
		
    rcsc::Vector2D top_left( 30 , -33 );
    rcsc::Vector2D bottom_right( 52 , -10 );
    rcsc::Rect2D left_side( top_left , bottom_right );
    top_left = rcsc::Vector2D( 30 , 10 );
    bottom_right = rcsc::Vector2D( 52 , 33 );
    rcsc::Rect2D right_side( top_left , bottom_right );
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Rect2D target_side = ( my_pos.y >= 0 ? right_side : left_side );
	rcsc::Vector2D target_point1( 52 , 0 );
	
	std::vector< rcsc::Vector2D > teammates( 12 );
	rcsc::Vector2D dummy( 100 , 100 );
	int unum = 0;
	for( int i = 0 ; i < 12 ; i++ )
		teammates[ i ] = dummy;
		
	rcsc::Vector2D tmpos( 100,100 );
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		tmpos = (*tm)->pos();
		if( ! target_side.contains( tmpos ) )
			continue;
		if( tmpos.x < my_pos.x - 1.0 )
			continue;
		if( tmpos.x > 46.0 || tmpos.absY() > 32.0 )
			continue;
			
		unum = (*tm)->unum();
		
		//target_point1.y *= 0.9;
		//rcsc::Sector2D front( tmpos , 0.5 , dist , -45 , 45 );
		//while( wm.existOpponentIn( front , 10 , true ) && dist >= 5.0 )
		target_point1 = tmpos;
		double dist = std::min( target_point1.x + 20.0 , 46.0 ) - target_point1.x;
		rcsc::Vector2D target = target_point1 + rcsc::Vector2D::polar2vector( dist , 0.0 );
		do
		{
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( target , 5 , &dist1 );
			if( nearest && nearest->pos().dist( target ) <= tmpos.dist( target ) + 1.0 )
			{
				dist -= 1.0;
				//front = rcsc::Sector2D( tmpos , 0.5 , dist , -45 , 45 );
				target = target_point1 + rcsc::Vector2D::polar2vector( dist , 0.0 );
			}
			else
				break;
		}
		while( dist > 0 );
		
		//target_point1.x += dist;
		teammates[ unum ] = target;
	}
	
	if( unum == 0 )
		return false;
	
	for( int i = 2 ; i < 12 ; i++ )
	{
		if( teammates[ i ].x > 52 )
			continue;
		double end_speed = 0.9;
	    double first_speed1 = 100.0;
	    rcsc::Vector2D ball_pos = wm.ball().pos();
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( teammates[ i ] ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.05;
	    }
	    while ( end_speed > 0.8 );
	
		first_speed1 = std::min( first_speed1 , rcsc::ServerParam::i().ballSpeedMax() ); 
		//first_speed1 *= ServerParam::i().ballDecay();
		
		rcsc::AngleDeg deg = ( teammates[ i ] - ball_pos ).th();
		/*
		rcsc::Vector2D first = Vector2D::polar2vector( first_speed1, deg );
	    double next_speed = first_speed1 * ServerParam::i().ballDecay();	
		bool pass = true;
	    const rcsc::PlayerPtrCont::const_iterator opp_end = wm.opponentsFromSelf().end();
	    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(); it != opp_end; ++it )
	    {
	        if ( ! (*it) || (*it)->posCount() > 10 )
				continue;
	        if ( (*it)->isGhost() && (*it)->posCount() > 3 )
				continue;
	        double dash = std::min( 2, (*it)->posCount() );
	        double opp2target = (*it)->pos().dist( teammates[ i ] );
	        if ( opp2target - dash < tmpos.dist( teammates[ i ] ) * 0.9 + 1.25 )
	            continue;
			rcsc::Vector2D dist2pos = (*it)->pos() - ball_pos - first;
			dist2pos.rotate( -deg );
			if ( dist2pos.x <= 0.0 || dist2pos.x >= ball_pos.dist( teammates[ i ] ) )
				continue;
			double opp_dist = dist2pos.absY() - dash - ServerParam::i().defaultKickableArea() - 0.1;
			if ( opp_dist >= 0.0 )
				continue;
			else
				pass = false;
			double projection = calc_length_geom_series( next_speed, dist2pos.x, ServerParam::i().ballDecay() );
			if ( projection >= 0.0 && opp_dist > projection )
				continue;
			pass = false;
	    }
	    
		if( pass )
		{
			* target_point = teammates[ i ];
			* receiver = i;
			* first_speed = first_speed1;
			//std::cout<<"corner pass => "<<i<<std::endl;
			return true;
		}
		*/
		
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		double dist = ball_pos.dist( teammates[i] );
		//rcsc::Sector2D pass( ball_pos , 0.0 , dist , deg - 10 , deg + 10 );
		//rcsc::Triangle2D pass(  ball_pos,
		//						teammates[i] + rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) , 
		//						teammates[i] - rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , teammates[i] ) )
		{
			* target_point = teammates[i];
			* receiver = i;
			* first_speed = first_speed1;
			std::cout<<"corner pass => "<<i<<std::endl;
			return true;
		}
		
	}

    //rcsc::Body_HoldBall().execute( agent );
	return false;
}



/*-----------------------------------------------------------------------*/

bool Body_LeadingPass::old_leading( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
		
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );

    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
    int value = -1000;
	rcsc::Vector2D pass( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = pass;
		tm_value[i] = value;
    }
    
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < wm.self().pos().x - 4.0 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
			
		rcsc::Vector2D tmpos = (*tm)->pos();
		int number = (*tm)->unum();
		double x = 5.0;
		double y = ( x * ( 34 - tmpos.absY() ) ) / ( 52.5 - tmpos.x );
		rcsc::Vector2D pass_point( x , y );
		if( tmpos.y >=0 )
			pass_point.y *= -1.0;
		
		bool safe = false;
		while( (!safe) && (pass_point.x >= tmpos.x) )
		{
			pass_points[ number ] = tmpos + pass_point;
			rcsc::Sector2D safe_area( tmpos , 0.5 , tmpos.dist( pass_points[ number ] ) , -20 , 20 );
			if( wm.existOpponentIn( safe_area , 10 , true ) )
				pass_point.x -= 1.0;
			else
				safe = true;
		}
		
		if( pass_point.x - tmpos.x < 1.0 )
			return false;
		
		rcsc::Vector2D ball_pos = wm.ball().pos();

		if( pass_points[ number ].x < ball_pos.x - 25.0 )
			continue;

		pass_points[ number ].x = std::min( offside - 1.0 , pass_points[ number ].x );
		
		if( pass_points[ number ].y > 33 )
			pass_points[ number ]. y = 33;
		if( pass_points[ number ].y < - 33 )
			pass_points[ number ]. y = -33;

		rcsc::Circle2D around( pass_points[ number ] , 2 );
		if( wm.existOpponentIn( around , 10 , true ) )
			tm_value[ number ] -= 1000;
				
		rcsc::Sector2D front( tmpos , 0.5 , 5.0 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			tm_value[ number ] += 200;
			
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 2.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		
		rcsc::Circle2D region( ball_pos , 10 );

		if( region.contains( tmpos ) )
			tm_value[ number ] -= 100;
		else
			tm_value[ number ] += 100;
		
		double dist = ball_pos.dist( pass_points[ number ] );
		if( dist > max_dp_dist )
			tm_value[ number ] -= 300;

		if( tmpos.absY() > 30 )
			tm_value[ number ] += 300;
		else
		if( tmpos.absY() > 20 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		
		if( tm_reach < opp_reach )
			tm_value[ number ] -= 500;
			
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 90 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 60 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 30 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;

	}
	
	rcsc::Vector2D tempVec;
	int tempVal;
	//int unum;
	for( int i = 0; i < 11 ; i++ )
		for( int j = i+1 ; j < 12 ; j++ )
			if( tm_value [i] < tm_value[j] )
			{
				tempVec = pass_points[i];
				tempVal = tm_value[i];
		//		unum = i;
				pass_points[i] = pass_points[j];
				tm_value[i] = tm_value[j];
			//	i = j;
				pass_points[j] = tempVec;
				tm_value[j] = tempVal;
				//j = unum;
			}
	
	for( int i = 0; i < 12 ; i++ )
	{				
		double end_speed = 1.3;
	    double first_speed1 = 100.0;
	    rcsc::Vector2D ball_pos = wm.ball().pos();
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( pass_points[i] ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.1;
	    }
	    while ( end_speed > 0.8 );

		first_speed1 = std::min( first_speed1 , rcsc::ServerParam::i().ballSpeedMax() ); 
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		
		double dist_to_point = 1000.0;
		const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( pass_points[i] , 5 , &dist_to_point );
		if( (! receiver_tm) || receiver_tm->isGhost() || receiver_tm->posCount() > 3  )
			continue;
		if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
			continue;
		
		rcsc::AngleDeg deg = ( pass_points[i] - ball_pos ).th();
		//rcsc::Sector2D pass( ball_pos , 0.0 , ball_pos.dist( pass_points[i] ) , deg - 12.5 , deg + 12.5 );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , pass_points[i] ) )
		{
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			//cout<<"leading pass => "<<receiver_tm->unum()<<endl;
			return true;
		}		
	}
  
	return false;
}


/*-------------------------------------------*/
bool Body_LeadingPass::test( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point )
{	
    const rcsc::WorldModel & wm = agent->world();

    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );

    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
	rcsc::Vector2D pass( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = pass;
		tm_value[i] = 0;
    }
    bool found = false;
    double receiver_to_target = 100.0;
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		rcsc::Vector2D ball_pos = wm.ball().pos();
		double dist = ball_pos.dist( tmpos );
		if( dist > max_dp_dist )
			continue;
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 48 || tmpos.absY() > 32 )
			continue;
	    //if ( tmpos.x < -10.0 && std::fabs( tmpos.y - ball_pos.y ) > 15.0 )
	    //    continue;
		//if( ( tmpos.x < ball_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 100.0 )
		//	continue;
		int number = (*tm)->unum();
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double dist1 = 15.0 * ball_pos.dist( tmpos ) / 20.0 - 7.0;
		if( ball_pos.dist( tmpos ) > 30.0 )
			dist1 = 15;
		if( ball_pos.dist( tmpos ) < 10.0 )
			dist1 = 0;
		double threshold = std::min( 46.0 , tmpos.x + dist1 );
		bool cont = false;
		rcsc::Vector2D front_tm( 48 , tmpos.y );
		rcsc::Line2D front( tmpos , front_tm );
		while( (! end) && ( checkFront.x <= threshold ) && ( checkFront.x <= tmpos.x + 28.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points[ number ] = pass.intersection( front );
			//rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points[ number ] ) , -30 , 30 );
			//if( ! wm.existOpponentIn( safe_area , 10 , true ) )
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( pass_points[ number ] , 5 , &dist1 );
			if( nearest
			&& Bhv_Predictor::predict_player_reach_cycle( nearest, pass_points[ number ], 1.0, 1.0, 3, 1, 1, false )
			< wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( pass_points[ number ] ) ) + 1 )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		found = true;
		/*
		rcsc::Vector2D pass_point( x , y );
		bool safe = false;
		do
		{
			pass_points[ number ] = tmpos + pass_point;
			rcsc::Sector2D safe_area( tmpos , 0.5 , 1.5 * tmpos.dist( pass_points[ number ] ) , -45 , 45 );
			if( wm.existOpponentIn( safe_area , 10 , true ) )
				pass_point.x -= 1.0;
			else
				safe = true;
		}
		while( (!safe) && (pass_point.x >= tmpos.x) );
		*/
		
		pass_points[ number ] = tmpos + checkFront;
		if( pass_points[ number ].x < ball_pos.x - 1.0 )
			continue;
		//if( pass_points[ number ].x < -35 && pass_points[ number ].absY() < 17 )
		//	continue;
		//pass_points[ number ].x = std::min( offside , pass_points[ number ].x );
		//if( pass_points[ number ].x < tmpos.x )
		//	continue;
		rcsc::Circle2D around( pass_points[ number ] , 1.0 );
		//if( wm.existOpponentIn( around , 10 , true ) )
		//	continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points[ number ] , 20 , &distance );
		tm_value[ number ] += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points[ number ].y > 31 )
			pass_points[ number ].y = 30;
		if( pass_points[ number ].y < -31 )
			pass_points[ number ].y = -30;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		rcsc::Sector2D front1( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front1 , 10 , true ) )
			tm_value[ number ] += 300;
		//tm_value[ number ] += 10.0 * (*tm)->posCount();
		/*
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 2.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		*/
		tm_value[ number ] += 10.0 * std::max( 5.0, std::fabs( pass_points[ number ].y - ball_pos.y ) );
		//rcsc::Circle2D region( ball_pos , 5 );
		//if( region.contains( tmpos ) )
		//	tm_value[ number ] -= 100;
		rcsc::AngleDeg deg = ( ball_pos - tmpos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		tm_value[ number ] +=  10.0 * count;
		
		if( pass_points[ number ].absY() > 26 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].absY() > 18 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			tm_value[ number ] -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points[ number ] += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 150 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 110 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 70 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;
	
	    receiver_to_target = tmpos.dist( pass_points[ number ] );
	}

	if( !found )
		return false;
	
	rcsc::Vector2D tempVec;
	int tempVal;
	for( int i = 0; i < 11 ; i++ )
		for( int j = i + 1; j < 12 ; j++ )
			if( tm_value [i] < tm_value[j] )
			{
				tempVec = pass_points[i];
				tempVal = tm_value[i];
				pass_points[i] = pass_points[j];
				tm_value[i] = tm_value[j];
				pass_points[j] = tempVec;
				tm_value[j] = tempVal;
			}
	
	rcsc::AngleDeg deg = ( pass_points[0] - wm.ball().pos() ).th();
	double first_speed1 = rcsc::calc_first_term_geom_series_last( 0.8 , wm.ball().pos().dist( pass_points[0] ), rcsc::ServerParam::i().ballDecay() );
	
	double dist_to_point = 1000.0;
	const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( pass_points[0] , 10 , &dist_to_point );
	if( (! receiver_tm) )
		return false;
	if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
		return false;
	if(! Body_Pass::exist_opponent3( agent , pass_points[0] ) )
	{
		* target_point = pass_points[0];
		return true;
	}
		
    //rcsc::Body_HoldBall().execute( agent );
	return false;
}

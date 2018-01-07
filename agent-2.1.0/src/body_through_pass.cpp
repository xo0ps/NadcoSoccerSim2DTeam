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

#include "body_through_pass.h"
#include "body_pass.h"
#include "body_kick_multi_step.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "bhv_predictor.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/

bool Body_ThroughPass::execute( rcsc::PlayerAgent * agent )
{
	//return false;
	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return false;
	
    rcsc::Vector2D target_point(50.0, 0.0);
    double first_speed = 0.0;
    int receiver = 0;

	if( M_mode == "toBehindDefenders" )
	{
		if ( ! toBehindDefendersPass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	}
	if( M_mode == "pointTo" )
	{
		if ( ! pointToPass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	}

	if( receiver == 0 )
		return false;

	/*
    Body_KickMultiStep( target_point, first_speed, false ).execute( agent );
     
    if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, 3 ).execute( agent ) )
    {
        first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
        if(! Body_KickOneStep( target_point, first_speed ).execute( agent ) )
			return false;
    }
    
    */
    
    if (! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
    //if (! Body_KickMultiStep( target_point , first_speed , false ).execute( agent ) )
		Body_KickOneStep( target_point , first_speed , true ).execute( agent );
	
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	Body_Pass::say_pass( agent , receiver , target_point );
    return true;
}
/*-----------------------------------------------------------------------*/

bool Body_ThroughPass::toBehindDefendersPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
		
	if( ! wm.self().isKickable() )
		return false;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );

	/*
	if( wm.self().pos().x < offside - 30 )
		return false;
	*/
    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
	rcsc::Vector2D dummy( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = dummy;
		tm_value[i] = 0;
    }
    
    double receiver_to_target = 100;
    bool found = false;
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
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 51 && tmpos.absY() > 33 )
			continue;
		if( tmpos.x < offside - 15 )
			continue;
		if ( std::fabs( tmpos.y - wm.self().pos().y ) > 25.0 )
			continue;
	    //if ( offside < 30.0 && tmpos.x < offside - 15.0 )
		//	continue;
	    //if ( (*tm)->angleFromSelf().abs() > 135.0 )
		//	continue;
				
		int number = (*tm)->unum();
		rcsc::Vector2D front_tm( 48 , tmpos.y );
		rcsc::Line2D front( tmpos , front_tm );
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double threshold = std::min( 46.0 , tmpos.x + 25.0 );
		bool cont = false;
		while( (! end) && ( checkFront.x <= threshold ) && ( checkFront.x <= tmpos.x + 28.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points[ number ] = pass.intersection( front );
			//rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points[ number ] ) , -30 , 30 );
			//if( ! wm.existOpponentIn( safe_area , 10 , true ) )
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( pass_points[ number ] , 5 , &dist1 );
			

			//if( nearest && nearest->pos().dist( pass_points[ number ] ) < wm.self().pos().dist( pass_points[ number ] ) + 1.0 )
			if( nearest
			&& Bhv_Predictor::predict_player_reach_cycle( nearest,
															pass_points[ number ],
															1.0,
															1.0,
															3,
															1,
															1,
															false )
			< wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( pass_points[ number ] ) ) + 1 )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		found = true;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points[ number ] , 20 , &distance );
		tm_value[ number ] += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points[ number ].y > 31 )
			pass_points[ number ].y = 31;
		if( pass_points[ number ].y < -31 )
			pass_points[ number ].y = -31;
		tm_value[ number ] += 10.0 * (*tm)->posCount();
		if( pass_points[ number ].x - tmpos.x > 10.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		tm_value[ number ] += 10.0 * std::max( 5.0, std::fabs( tmpos.y - ball_pos.y ) );
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points[ number ] += first;
	    }
		if( pass_points[ number ].absY() > 30 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].absY() > 20 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 150 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 100 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 60 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;
		receiver_to_target = tmpos.dist( pass_points[ number ] );
	}
	
	if( ! found )
		return false;
		
	rcsc::Vector2D tempVec;
	int tempVal;
	for( int i = 0; i < 11 ; i++ )
		for( int j = i + 1 ; j < 12 ; j++ )
			if( tm_value [i] < tm_value[j] )
			{
				tempVec = pass_points[i];
				tempVal = tm_value[i];
				pass_points[i] = pass_points[j];
				tm_value[i] = tm_value[j];
				pass_points[j] = tempVec;
				tm_value[j] = tempVal;
			}


	
	//for( int i = 0 ; i < 6 ; i++ )
	//	std::cout<<"["<<wm.time().cycle()<<"] => "<<tm_value[i]<<std::endl;
	//std::cout<<"\n";
	
	for( int i = 0; i < 6 ; i++ )
	{		
		if( pass_points[i].x < offside - 5.0 )
			continue;
		if( pass_points[i].x > 36 && pass_points[i].absY() < 17 )
			continue;
		if( pass_points[i].dist( wm.self().pos() ) < 5.0 )
			continue;
		if( pass_points[i].x < wm.self().pos().x + 3.0 )
			continue;
		if( pass_points[i].dist( wm.ball().pos() ) > max_dp_dist )
			continue;
		if( pass_points[i].x > 48.0 )
			continue;
		//if( tm_value[ i ] < 800 )
		//	return false;
		
		double end_speed = 0.8;
	    double first_speed1 = 4.0;
	    rcsc::Vector2D ball_pos = wm.ball().pos();
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( pass_points[i] ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
				end_speed -= 0.1;
	            continue;
	        }
	        end_speed -= 0.02;
	    }
	    while ( end_speed > 0.5 );
	
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
	        double dash = std::min( 2, (*it)->posCount() );
	        double opp2target = (*it)->pos().dist( pass_points[ i ] );
	        if ( opp2target - dash < receiver_to_target * 0.9 + 1.25 )
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
			if ( projection >= 0.0 && opp_dist > projection )
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
			std::cout<<"through pass => "<<receiver_tm->unum()<<std::endl;
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
		rcsc::Sector2D pass( ball_pos , 0.0 , dist , deg - 10 , deg + 10 );
		//rcsc::Triangle2D pass(  ball_pos,
		//						pass_points[i] + rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) , 
		//						pass_points[i] - rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , pass_points[i] , true ) )
		{
			* target_point = pass_points[i];
			* receiver = receiver_tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"through pass => "<<receiver_tm->unum()<<std::endl;
			return true;
		}
		
	}

    //rcsc::Body_HoldBall().execute( agent );
	return false;

}

/*------------------------------------------------------------*/

bool Body_ThroughPass::pointToPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{
	return false;
	
	const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
		
	double offside = wm.offsideLineX();
	
	if( ! wm.self().isKickable() )
		return false;
	
	if( wm.self().pos().x < offside - 20 )
		return false;

	rcsc::Vector2D target_point1 = agent->effector().getPointtoPos();
	double end_speed = 0.85;
    double first_speed1 = 100.0;
    rcsc::Vector2D ball_pos = wm.ball().pos();
    do
    {
        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( target_point1 ) , rcsc::ServerParam::i().ballDecay() );
        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        end_speed -= 0.05;
    }
    while ( end_speed > 0.5 );
    
    int unum = 0;
	rcsc::AngleDeg deg = ( target_point1 - ball_pos ).th();
	rcsc::Vector2D first = rcsc::Vector2D::polar2vector( first_speed1, deg );
    double next_speed = first_speed1 * rcsc::ServerParam::i().ballDecay();	
	bool pass = true;
    const rcsc::PlayerPtrCont::const_iterator opp_end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(); it != opp_end; ++it )
    {
        if ( ! (*it) || (*it)->posCount() > 10 )
			continue;
        if ( (*it)->isGhost() && (*it)->posCount() > 3 )
			continue;
        double dash = std::min( 2, (*it)->posCount() );
        double opp2target = (*it)->pos().dist( target_point1 );
		double dist_to_point = 1000.0;
		const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( target_point1 , 5 , &dist_to_point );
		if( (! receiver_tm) || receiver_tm->isGhost() || receiver_tm->posCount() > 3  )
			continue;
		if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
			continue;
        if ( opp2target - dash < receiver_tm->pos().dist( target_point1 ) * 0.9 + 1.25 )
            continue;
        unum = receiver_tm->unum();
		rcsc::Vector2D dist2pos = (*it)->pos() - wm.ball().pos() - first;
		dist2pos.rotate( -deg );
		if ( dist2pos.x <= 0.0 || dist2pos.x >= ball_pos.dist( target_point1 ) )
			continue;
		double opp_dist = dist2pos.absY() - dash - rcsc::ServerParam::i().defaultKickableArea() - 0.1;
		if ( opp_dist >= 0.0 )
			continue;
		else
			pass = false;
		double projection = rcsc::calc_length_geom_series( next_speed, dist2pos.x, rcsc::ServerParam::i().ballDecay() );
		if ( projection >= 0.0 && opp_dist > projection )
			continue;
		pass = false;
    }
    
	/*
	rcsc::Sector2D pass( ball_pos , 0.0 , ball_pos.dist( pass_points[i] ) , deg - 12.5 , deg + 12.5 );
	if(! wm.existOpponentIn( pass, 10 , false ) )
	{
		* target_point = pass_points[i];
		* receiver = receiver_tm->unum();
		* first_speed = first_speed1;
		//std::cout<<"corner pass => "<<receiver_tm->unum()<<std::endl;
		return true;
	}
	*/
	
	if( pass )
	{
		* target_point = target_point1;
		* receiver = unum;
		* first_speed = first_speed1;
		//std::cout<<"pointTo pass => "<<unum<<std::endl;
		return true;
	}
    //rcsc::Body_HoldBall().execute( agent );
	return false;
}



bool
Body_ThroughPass::test( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
		
	if( ! wm.self().isKickable() )
		return false;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );

	/*
	if( wm.self().pos().x < offside - 30 )
		return false;
	*/
    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
	rcsc::Vector2D dummy( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = dummy;
		tm_value[i] = 0;
    }
    
    double receiver_to_target = 100;
    bool found = false;
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
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 51 && tmpos.absY() > 33 )
			continue;
		if( tmpos.x < offside - 10 )
			continue;
		if ( std::fabs( tmpos.y - wm.self().pos().y ) > 25.0 )
			continue;
	    //if ( offside < 30.0 && tmpos.x < offside - 15.0 )
		//	continue;
	    //if ( (*tm)->angleFromSelf().abs() > 135.0 )
		//	continue;
				
		int number = (*tm)->unum();
		rcsc::Vector2D front_tm( 48 , tmpos.y );
		rcsc::Line2D front( tmpos , front_tm );
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double threshold = std::min( 46.0 , tmpos.x + 25.0 );
		bool cont = false;
		while( (! end) && ( checkFront.x <= threshold ) && ( checkFront.x <= tmpos.x + 28.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points[ number ] = pass.intersection( front );
			//rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points[ number ] ) , -30 , 30 );
			//if( ! wm.existOpponentIn( safe_area , 10 , true ) )
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( pass_points[ number ] , 5 , &dist1 );
			//if( nearest && nearest->pos().dist( pass_points[ number ] ) < wm.self().pos().dist( pass_points[ number ] ) + 1.0 )
			if( nearest
			&& Bhv_Predictor::predict_player_reach_cycle( nearest,
															pass_points[ number ],
															1.0,
															1.0,
															3,
															1,
															1,
															false )
			< wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( pass_points[ number ] ) ) + 1 )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		found = true;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points[ number ] , 20 , &distance );
		tm_value[ number ] += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points[ number ].y > 31 )
			pass_points[ number ].y = 31;
		if( pass_points[ number ].y < -31 )
			pass_points[ number ].y = -31;
		tm_value[ number ] += 10.0 * (*tm)->posCount();
		if( pass_points[ number ].x - tmpos.x > 10.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		tm_value[ number ] += 10.0 * std::max( 5.0, std::fabs( tmpos.y - ball_pos.y ) );
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points[ number ] += first;
	    }
		if( pass_points[ number ].absY() > 30 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].absY() > 20 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 150 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 100 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 60 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;
		receiver_to_target = tmpos.dist( pass_points[ number ] );
	}
	
	if( ! found )
		return false;
		
	rcsc::Vector2D tempVec = pass_points[0];
	int tempVal = -1000;
	for( int i = 0; i < 12 ; i++ )
		if( tm_value [i] > tempVal )
		{
			tempVal = tm_value[i];
			tempVec = pass_points[i];
		}

	{
		if( tempVec.x < offside - 5.0 )
			return false;
		if( tempVec.x > 36 && tempVec.absY() < 17 )
			return false;
		if( tempVec.dist( wm.self().pos() ) < 5.0 )
			return false;
		if( tempVec.x < wm.self().pos().x + 3.0 )
			return false;
		if( tempVec.dist( wm.ball().pos() ) > max_dp_dist )
			return false;
		if( tempVec.x > 48.0 )
			return false;
		double dist = wm.ball().pos().dist( tempVec );
		rcsc::AngleDeg deg = ( tempVec - wm.ball().pos() ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12 , deg + 12 );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , tempVec , true ) )
		{
			return true;
		}
		
	}
	
	return false;
}


/*----------------------------------------------*/
bool
Body_ThroughPass::test1( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{
	const rcsc::WorldModel & wm = agent->world();
		
	if( ! wm.self().isKickable() )
		return false;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );

	/*
	if( wm.self().pos().x < offside - 30 )
		return false;
	*/
    std::vector< rcsc::Vector2D > pass_points ( 12 );
    std::vector< int > tm_value ( 12 );
	rcsc::Vector2D dummy( 0,0 );
	
    for( int i = 0; i < 12; i++ )
    {
		pass_points[i] = dummy;
		tm_value[i] = 0;
    }
    
    double receiver_to_target = 100;
    bool found = false;
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
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 51 && tmpos.absY() > 33 )
			continue;
		if( tmpos.x < offside - 10 )
			continue;
		if ( std::fabs( tmpos.y - wm.self().pos().y ) > 25.0 )
			continue;
	    //if ( offside < 30.0 && tmpos.x < offside - 15.0 )
		//	continue;
	    //if ( (*tm)->angleFromSelf().abs() > 135.0 )
		//	continue;
				
		int number = (*tm)->unum();
		rcsc::Vector2D front_tm( 48 , tmpos.y );
		rcsc::Line2D front( tmpos , front_tm );
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double threshold = std::min( 46.0 , tmpos.x + 25.0 );
		bool cont = false;
		while( (! end) && ( checkFront.x <= threshold ) && ( checkFront.x <= tmpos.x + 28.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points[ number ] = pass.intersection( front );
			//rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points[ number ] ) , -30 , 30 );
			//if( ! wm.existOpponentIn( safe_area , 10 , true ) )
			double dist1 = 100.0;
			const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( pass_points[ number ] , 5 , &dist1 );
			//if( nearest && nearest->pos().dist( pass_points[ number ] ) < wm.self().pos().dist( pass_points[ number ] ) + 1.0 )
			if( nearest
			&& Bhv_Predictor::predict_player_reach_cycle( nearest,
															pass_points[ number ],
															1.0,
															1.0,
															3,
															1,
															1,
															false )
			< wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( pass_points[ number ] ) ) + 1 )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		found = true;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points[ number ] , 20 , &distance );
		tm_value[ number ] += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points[ number ].y > 31 )
			pass_points[ number ].y = 31;
		if( pass_points[ number ].y < -31 )
			pass_points[ number ].y = -31;
		tm_value[ number ] += 10.0 * (*tm)->posCount();
		if( pass_points[ number ].x - tmpos.x > 10.0 )
			tm_value[ number ] += 400;
		else
		if( pass_points[ number ].x - tmpos.x > 8.0 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].x - tmpos.x > 6.0 )
			tm_value[ number ] += 200;
		else
		if( pass_points[ number ].x - tmpos.x > 4.0 )
			tm_value[ number ] += 100;
		else
			tm_value[ number ] += 0;
		tm_value[ number ] += 10.0 * std::max( 5.0, std::fabs( tmpos.y - ball_pos.y ) );
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points[ number ] += first;
	    }
		if( pass_points[ number ].absY() > 30 )
			tm_value[ number ] += 300;
		else
		if( pass_points[ number ].absY() > 20 )
			tm_value[ number ] += 200;
		else
			tm_value[ number ] += 100;
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 150 )
			tm_value[ number ] -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 100 )
			tm_value[ number ] -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points[ number ] ).th() ).abs() > 60 )
			tm_value[ number ] -= 10;
		else
			tm_value[ number ] -= 0;
		receiver_to_target = tmpos.dist( pass_points[ number ] );
	}
	
	if( ! found )
		return false;
		
	rcsc::Vector2D tempVec = pass_points[0];
	int tempVal = -1000;
	for( int i = 0; i < 12 ; i++ )
		if( tm_value [i] > tempVal )
		{
			tempVal = tm_value[i];
			tempVec = pass_points[i];
		}

	{
		if( tempVec.x < offside - 5.0 )
			return false;
		if( tempVec.x > 42 && tempVec.absY() < 17 )
			return false;
		if( tempVec.dist( wm.self().pos() ) < 5.0 )
			return false;
		if( tempVec.x < wm.self().pos().x + 3.0 )
			return false;
		if( tempVec.dist( wm.ball().pos() ) > max_dp_dist )
			return false;
		if( tempVec.x > 48.0 )
			return false;
		double dist = wm.ball().pos().dist( tempVec );
		rcsc::AngleDeg deg = ( tempVec - wm.ball().pos() ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12 , deg + 12 );
		//if(! wm.existOpponentIn( pass, 10 , false ) )
		if(! Body_Pass::exist_opponent3( agent , tempVec , true ) )
		{
			double end_speed = 0.8;
		    double first_speed1 = 4.0;
		    rcsc::Vector2D ball_pos = wm.ball().pos();
		    do
		    {
		        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, wm.ball().pos().dist( tempVec ) , rcsc::ServerParam::i().ballDecay() );
		        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
		        {
					end_speed -= 0.1;
		            break;
		        }
		        end_speed -= 0.02;
		    }
		    while ( end_speed > 0.5 );
		
			first_speed1 = std::min( first_speed1 , rcsc::ServerParam::i().ballSpeedMax() ); 
			first_speed1 *= rcsc::ServerParam::i().ballDecay();
			* target_point = tempVec;
			double dist = 100.0;
			const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( tempVec , 10 , &dist );
			if( receiver_tm )
				* receiver = receiver_tm->unum();
			else
				* receiver = 10;
			* first_speed = first_speed1;
			return true;
		}
		
	}
	
	return false;
}

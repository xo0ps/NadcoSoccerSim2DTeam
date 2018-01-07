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
#include "sample_player.h"
#include "strategy.h"
#include "body_selection_pass.h"
#include "body_kick_one_step.h"
#include "body_kick_multi_step.h"
#include "body_smart_kick.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/

bool Body_SelectionPass::execute( rcsc::PlayerAgent * agent )
{
    
    //if ( ! SamplePlayer::instance().selection_pass() )
    if ( ! Strategy::i().selection_pass() )
    {
		return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    if ( ! wm.self().isKickable() )
        return false;
        
	std::vector< pass >pass;
	
	/*
	if( ! select_pass( agent , &pass ) )
		return false;
	*/
	
	/*----------------------------------------------------------------------*/
	
	pass.clear();
	
	double offside = wm.offsideLineX();
    double ball_d = sp.ballDecay();
    double max_ball_s = sp.ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	rcsc::Vector2D pass_points( 100 , 100 );
	double value = 0;
	int unum = 0;
	char type = 'a';
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.absX() > 50 || tm_pos.absY() > 32 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 2.0 )
			continue;
		if( ( tm_pos.x < my_pos.x + 5.0 )
			&& (*tm)->angleFromSelf().abs() > 40.0
			&& ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) )
		  )
			continue;
		if( tm_pos.x < my_pos.x - 8.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		pass_points = tm_pos;
		double distance = 100.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		value += 10.0 * (*tm)->posCount();
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		value += 10.0 * std::max( 5.0, std::fabs( tm_pos.y - my_pos.y ) );
		if( tm_pos.x > my_pos.x + 30.0 )
			value += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value += 200;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value += 100;
		else
		if( tm_pos.x > my_pos.x )
			value += 50;
		else
			value += 0;
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
	    //M_pass.push_back( std::make_pair( teammates[ unum ] , unum ) );
	    //pass pass_route( teammates[ unum ] , value[ unum ] , unum );
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'd';
	    pass.push_back( pass_route );
	}

	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( tmpos.x < ball_pos.x - 5.0 )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 48 || tmpos.absY() > 32 )
			continue;
	    if ( tmpos.x < -10.0 && std::fabs( tmpos.y - ball_pos.y ) > 15.0 )
	        continue;
		//if( ( tmpos.x < ball_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 100.0 )
		//	continue;
		unum = (*tm)->unum();
		double x = 5.0;
		double y = ( x * ( 34 - tmpos.absY() ) ) / ( 52.5 - tmpos.x );
		if( tmpos.y >= 0 )
			y *= -1.0;
		y *= 0.8;
		y = std::min( y , 32.0 );
		
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
		
		pass_points = tmpos + rcsc::Vector2D( x , y );
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		//if( pass_points[ number ].x < -35 && pass_points[ number ].absY() < 17 )
		//	continue;
		//pass_points[ number ].x = std::min( offside , pass_points[ number ].x );
		//if( pass_points[ number ].x < tmpos.x )
		//	continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
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
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
		
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    pass.push_back( pass_route );
	}
	
	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x > -25 )
			continue;
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
		unum = (*tm)->unum();
		pass_points = (*tm)->pos();
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
			
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    pass.push_back( pass_route );
	}

	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
    rcsc::Vector2D top_left( 30 , -33 );
    rcsc::Vector2D bottom_right( 52 , -10 );
    rcsc::Rect2D left_side( top_left , bottom_right );
    top_left = rcsc::Vector2D( 30 , 10 );
    bottom_right = rcsc::Vector2D( 52 , 33 );
    rcsc::Rect2D right_side( top_left , bottom_right );
	rcsc::Rect2D target_side = ( my_pos.y >= 0 ? right_side : left_side );
	rcsc::Vector2D target_point1( 52 , 0 );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x < 0 )
			continue;
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( ! target_side.contains( tmpos ) )
			continue;
		if( tmpos.x < my_pos.x - 1.0 )
			continue;
		if( tmpos.x > 46.0 || tmpos.absY() > 32.0 )
			continue;
		unum = (*tm)->unum();
		pass_points = (*tm)->pos();
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
		target_point1 = tmpos;
		//target_point1.y *= 0.9;
		dist = std::min( target_point1.x + 10.0 , 46.0 );
		rcsc::Sector2D front1( tmpos , 0.5 , dist , -45 , 45 );
		while( wm.existOpponentIn( front1 , 10 , true ) && dist >= 5.0 )
		{
			dist -= 1.0;
			front1 = rcsc::Sector2D( tmpos , 0.5 , dist , -45 , 45 );
		}
		target_point1.x += dist;
		
	    pass_route.point = target_point1;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    pass.push_back( pass_route );
	}

	max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x < offside - 30 )
			continue;
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 51 && tmpos.absY() > 33 )
			continue;
		if( tmpos.x < offside - 15 )
			continue;
		if ( std::fabs( tmpos.y - wm.self().pos().y ) > 15.0 )
			continue;
	    //if ( offside < 30.0 && tmpos.x < offside - 15.0 )
		//	continue;
	    //if ( (*tm)->angleFromSelf().abs() > 135.0 )
		//	continue;
				
		unum = (*tm)->unum();
		rcsc::Vector2D front_tm( 48 , tmpos.y * 0.9 );
		rcsc::Line2D front( tmpos , front_tm );
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double threshold = std::min( 46.0 , tmpos.x + 10.0 );
		bool cont = false;
		while( (! end) && ( checkFront.x < threshold ) && ( checkFront.x < tmpos.x + 13.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points = pass.intersection( front );
			rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points ) , -30 , 30 );
			if( ! wm.existOpponentIn( safe_area , 10 , true ) )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;
		if( pass_points.x < offside - 5.0 )
			continue;
		if( pass_points.x > 36 && pass_points.absY() < 17 )
			continue;
		if( pass_points.dist( my_pos ) < 5.0 )
			continue;
		if( pass_points.x < my_pos.x + 3.0 )
			continue;
		if( pass_points.dist( ball_pos ) > max_dp_dist )
			continue;
		if( pass_points.x > 48.0 )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points , 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		value += 10.0 * (*tm)->posCount();
		if( pass_points.x - tmpos.x > 10.0 )
			value += 400;
		else
		if( pass_points.x - tmpos.x > 8.0 )
			value += 300;
		else
		if( pass_points.x - tmpos.x > 6.0 )
			value += 200;
		else
		if( pass_points.x - tmpos.x > 4.0 )
			value += 100;
		else
			value += 0;
		value += 10.0 * std::max( 5.0, std::fabs( tmpos.y - ball_pos.y ) );
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( pass_points.absY() > 30 )
			value += 300;
		else
		if( pass_points.absY() > 20 )
			value += 200;
		else
			value += 100;
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 100 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 60 )
			value -= 10;
		else
			value -= 0;

	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 't';
	    pass.push_back( pass_route );

	}
	
	/*----------------------------------------------------------------------*/
	
	if( pass.empty() )
	{
		std::cout<<"hold ball"<<std::endl;
		rcsc::Body_HoldBall().execute( agent );
		return false;
	}
	
	double vtmp;
	rcsc::Vector2D ptmp;
	int utmp;
	char ttmp;
    for(unsigned int i = 0 ; i < pass.size() - 1 ; i++)
		for(unsigned int j = i + 1 ; j < pass.size() ; j++)
			if( pass[i].value < pass[j].value )
		    {
				vtmp = pass[i].value;
				ptmp = pass[i].point;
				utmp = pass[i].unum;
				ttmp = pass[i].type;
				pass[i].value = pass[j].value;
				pass[i].point = pass[j].point;
				pass[i].unum = pass[j].unum;
				pass[i].type = pass[j].type;
				pass[j].value = vtmp;
				pass[j].point = ptmp;
				pass[j].unum = utmp;
				pass[j].type = ttmp;
		    }
	
	/*
	for( int i = 0 ; i < 10 ; i++ )
		std::cout<<"["<<wm.time().cycle()<<"] => "<<pass[i].value<<std::endl;
	std::cout<<"\n";
	*/
		
	for( unsigned int i = 0 ; i < pass.size() ; i++ )
	{
		rcsc::Vector2D target_point = pass[i].point;
		int receiver = pass[i].unum;
		
		/*
		double dist = ball_pos.dist( target_point );
		rcsc::AngleDeg deg = ( ball_pos - target_point ).th();
		//rcsc::Sector2D pass( ball_pos , 0.0 , dist , deg - 10 , deg + 10 );
		rcsc::Triangle2D pass1(  ball_pos,
								target_point + rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) , 
								target_point - rcsc::Vector2D::polar2vector( dist * std::tan( 10 ) , deg + 90 ) );
		if( wm.existOpponentIn( pass1, 10 , true ) )
		{
			continue;
		}
		*/
		/*
		double end_speed = 1.0;
		if( pass[i].type == 'd' )
			end_speed = 1.6;
		else
		if( pass[i].type == 'l' )
			end_speed = 1.3;
		else
			end_speed = 1.0;
	    double first_speed1 = 10.0;
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, ball_pos.dist( target_point ) , sp.ballDecay() );
	        if ( first_speed1 <= sp.ballSpeedMax() )
	            break;
	        end_speed -= 0.05;
	    }
	    while ( end_speed > 0.3 );
	
		first_speed1 = std::min( first_speed1 , sp.ballSpeedMax() ); 
		//first_speed1 *= sp.ballDecay();
		Body_KickMultiStep( target_point, first_speed1, false ).execute( agent );
	    int kick_step = ( wm.gameMode().type() != rcsc::GameMode::PlayOn
	                      && wm.gameMode().type() != rcsc::GameMode::GoalKick_
	                      ? 1
	                      : 3 );
	    if ( ! Body_SmartKick( target_point, first_speed1 , first_speed1 * 0.96, kick_step ).execute( agent ) )
	    {
			first_speed1 = std::min( wm.self().kickRate() * sp.maxPower(), first_speed1 );
	        if( ! Body_KickOneStep( target_point, first_speed1 , true ).execute( agent ) )
				return false;
	    }
	    */
	    rcsc::AngleDeg deg = ( ball_pos - target_point ).th();
	    if( Body_Pass::exist_opponent3( agent , target_point ) )
			continue;
	    double first_speed = Body_Pass::first_speed( agent , target_point , pass[i].type );
	    
	    if (! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
			Body_KickOneStep( target_point , first_speed , true ).execute( agent );
	
	    if ( agent->config().useCommunication()
			&& receiver != rcsc::Unum_Unknown )
		{
			Body_Pass::say_pass( agent , receiver , target_point );
		}
	    
	    return true;
	}
	
	return false;
}

/*--------------------------------------------*/
bool
Body_SelectionPass::select_pass( rcsc::PlayerAgent * agent , std::vector< pass > * pass )
{
				
	if(! pass )
		return false;
	//pass.clear();
	
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
	double offside = wm.offsideLineX();
    double ball_d = sp.ballDecay();
    double max_ball_s = sp.ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	rcsc::Vector2D pass_points( 100 , 100 );
	double value = 0;
	int unum = 0;
	char type = 'a';
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.absX() > 50 || tm_pos.absY() > 32 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 2.0 )
			continue;
		if( ( tm_pos.x < my_pos.x + 5.0 )
			&& (*tm)->angleFromSelf().abs() > 40.0
			&& ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) )
		  )
			continue;
		if( tm_pos.x < my_pos.x - 8.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		pass_points = tm_pos;
		double distance = 100.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		value += 10.0 * (*tm)->posCount();
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		value += 10.0 * std::max( 5.0, std::fabs( tm_pos.y - my_pos.y ) );
		if( tm_pos.x > my_pos.x + 30.0 )
			value += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value += 200;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value += 100;
		else
		if( tm_pos.x > my_pos.x )
			value += 50;
		else
			value += 0;
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
	    //M_pass.push_back( std::make_pair( teammates[ unum ] , unum ) );
	    //pass pass_route( teammates[ unum ] , value[ unum ] , unum );
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'd';
	    //pass.push_back( pass_route );
	}

	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( tmpos.x < ball_pos.x - 5.0 )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 48 || tmpos.absY() > 32 )
			continue;
	    if ( tmpos.x < -10.0 && std::fabs( tmpos.y - ball_pos.y ) > 15.0 )
	        continue;
		//if( ( tmpos.x < ball_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 100.0 )
		//	continue;
		unum = (*tm)->unum();
		double x = 5.0;
		double y = ( x * ( 34 - tmpos.absY() ) ) / ( 52.5 - tmpos.x );
		if( tmpos.y >= 0 )
			y *= -1.0;
		y *= 0.8;
		y = std::min( y , 32.0 );
		
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
		
		pass_points = tmpos + rcsc::Vector2D( x , y );
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		//if( pass_points[ number ].x < -35 && pass_points[ number ].absY() < 17 )
		//	continue;
		//pass_points[ number ].x = std::min( offside , pass_points[ number ].x );
		//if( pass_points[ number ].x < tmpos.x )
		//	continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
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
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
		
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    //pass.push_back( pass_route );
	}
	
	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x > -25 )
			continue;
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
		unum = (*tm)->unum();
		pass_points = (*tm)->pos();
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
			
	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    //pass.push_back( pass_route );
	}

	max_dp_dist = 0.7 * rcsc::inertia_final_distance( max_ball_s,ball_d );
    rcsc::Vector2D top_left( 30 , -33 );
    rcsc::Vector2D bottom_right( 52 , -10 );
    rcsc::Rect2D left_side( top_left , bottom_right );
    top_left = rcsc::Vector2D( 30 , 10 );
    bottom_right = rcsc::Vector2D( 52 , 33 );
    rcsc::Rect2D right_side( top_left , bottom_right );
	rcsc::Rect2D target_side = ( my_pos.y >= 0 ? right_side : left_side );
	rcsc::Vector2D target_point1( 52 , 0 );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x < 0 )
			continue;
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		if( ! target_side.contains( tmpos ) )
			continue;
		if( tmpos.x < my_pos.x - 1.0 )
			continue;
		if( tmpos.x > 46.0 || tmpos.absY() > 32.0 )
			continue;
		unum = (*tm)->unum();
		pass_points = (*tm)->pos();
		if( pass_points.x < ball_pos.x - 1.0 )
			continue;
		rcsc::Circle2D around( pass_points , 1.0 );
		if( wm.existOpponentIn( around , 10 , true ) )
			continue;
		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points, 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		rcsc::Sector2D front( tmpos , 0.5 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value += 300;
		value += 10.0 * (*tm)->posCount();
		rcsc::Circle2D region( ball_pos , 5 );
		if( region.contains( tmpos ) )
			value -= 100;
		value += 10.0 * std::max( 5.0, std::fabs( pass_points.y - ball_pos.y ) );
		rcsc::AngleDeg deg = ( ball_pos - pass_points ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value +=  10.0 * count;
		double dist = ball_pos.dist( pass_points );
		if( dist > max_dp_dist )
			value -= 300;
		if( pass_points.absY() > 26 )
			value += 300;
		else
		if( pass_points.absY() > 18 )
			value += 200;
		else
			value += 100;
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		if( tm_reach < opp_reach )
			value -= 500;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 110 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 70 )
			value -= 10;
		else
			value -= 0;
		target_point1 = tmpos;
		//target_point1.y *= 0.9;
		dist = std::min( target_point1.x + 10.0 , 46.0 );
		rcsc::Sector2D front1( tmpos , 0.5 , dist , -45 , 45 );
		while( wm.existOpponentIn( front1 , 10 , true ) && dist >= 5.0 )
		{
			dist -= 1.0;
			front1 = rcsc::Sector2D( tmpos , 0.5 , dist , -45 , 45 );
		}
		target_point1.x += dist;
		
	    pass_route.point = target_point1;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 'l';
	    //pass.push_back( pass_route );
	}

	max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		pass_points.assign( 100 , 100 );
		value = 0;
		unum = 0;
		type = 'a';

		if( my_pos.x < offside - 30 )
			continue;
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < ball_pos.x )
			continue;
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.absX() > 51 && tmpos.absY() > 33 )
			continue;
		if( tmpos.x < offside - 15 )
			continue;
		if ( std::fabs( tmpos.y - wm.self().pos().y ) > 15.0 )
			continue;
	    //if ( offside < 30.0 && tmpos.x < offside - 15.0 )
		//	continue;
	    //if ( (*tm)->angleFromSelf().abs() > 135.0 )
		//	continue;
				
		unum = (*tm)->unum();
		rcsc::Vector2D front_tm( 48 , tmpos.y * 0.9 );
		rcsc::Line2D front( tmpos , front_tm );
		
		bool end = false;
		rcsc::Vector2D checkFront = tmpos;
		double threshold = std::min( 46.0 , tmpos.x + 10.0 );
		bool cont = false;
		while( (! end) && ( checkFront.x < threshold ) && ( checkFront.x < tmpos.x + 13.0 ) )
		{
			rcsc::AngleDeg theta = ( ball_pos - checkFront ).th();
			rcsc::Line2D pass( ball_pos , theta );
			pass_points = pass.intersection( front );
			if( pass_points.x > 48.0 )
			{
				cont = true;
				break;
			}
			/*
			if( pass_points[ number ].x > 36.0 && pass_points[ number ].absY() < 17.0 )
			{
				cont = true;
				break;
			}
			*/
			if( pass_points.dist( ball_pos ) > max_dp_dist )
			{
				cont = true;
				break;
			}
			if( pass_points.x < my_pos.x + 3.0 )
			{
				cont = true;
				break;
			}
			/*
			int max = 100 , ave = 100;
            wm.dirRangeCount( theta , 20.0, &max, NULL, &ave );
            if( max > 9 || ave > 3 )
            {
				cont = true;
				break;
            }
            */
            if( pass_points.dist( ball_pos ) < 5.0 )
            {	
				cont = true;
				break;
            }
            if( pass_points.x < tmpos.x + 2.0 )
            {
				cont = true;
				break;
			}	
			rcsc::Sector2D safe_area( tmpos , 0.0 , 1.2 * tmpos.dist( pass_points ) , -30 , 30 );
			if( ! wm.existOpponentIn( safe_area , 10 , true ) )
				checkFront.x += 1.0;
			else
				end = true;
    	}
		if( cont )
			continue;

		double distance = 100.0;
		wm.getOpponentNearestTo( pass_points , 20 , &distance );
		value += 10.0 * std::max( 0.0, 30.0 - distance );
		if( pass_points.y > 31 )
			pass_points.y = 31;
		if( pass_points.y < -31 )
			pass_points.y = -31;
		value += 10.0 * (*tm)->posCount();
		if( pass_points.x - tmpos.x > 10.0 )
			value += 400;
		else
		if( pass_points.x - tmpos.x > 8.0 )
			value += 300;
		else
		if( pass_points.x - tmpos.x > 6.0 )
			value += 200;
		else
		if( pass_points.x - tmpos.x > 4.0 )
			value += 100;
		else
			value += 0;
		value += 10.0 * std::max( 5.0, std::fabs( tmpos.y - ball_pos.y ) );
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / sp.defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        pass_points += first;
	    }
		if( pass_points.absY() > 30 )
			value += 300;
		else
		if( pass_points.absY() > 20 )
			value += 200;
		else
			value += 100;
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 150 )
			value -= 50;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 100 )
			value -= 30;
		else
		if( ( (*tm)->body().degree() - ( ball_pos - pass_points ).th() ).abs() > 60 )
			value -= 10;
		else
			value -= 0;

	    pass_route.point = pass_points;
	    pass_route.value = value;
	    pass_route.unum = unum;
	    pass_route.type = 't';
	    //pass.push_back( pass_route );

	}
	return true;
}

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

#include "body_pass.h"
#include "strategy.h"
#include "body_dash.h"
#include "body_kick_two_step.h"
#include "body_smart_kick.h"
#include "body_kick_multi_step.h"
#include "body_kick_one_step.h"
#include "body_go_to_point.h"
#include "intention_receive.h"
#include "bhv_predictor.h"

#include <rcsc/player/player_agent.h>

#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Dash::execute( rcsc::PlayerAgent * agent )
{
	if ( ! Strategy::i().dash() )
    {
		return false;
    }


	const rcsc::WorldModel & wm = agent->world();

    if ( ! wm.self().isKickable() )
        return false;
	
	if( wm.self().pos().x < 0.0 )
		return false;
			
	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.55 )
	{
		//rcsc::Body_StopBall().execute( agent );
		return false;
	}
	
	//if(  )
	
	/*
	static int count = -5;
	
	if( count < 0 )
	{
		count++;
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return false;
	}
	
	if( ! ( wm.existKickableOpponent() && wm.existKickableTeammate() ) )
		count = -5;
	*/
	
	rcsc::Sector2D around( wm.self().pos() , 0.5 , 8.0 , -50 , 50 );
	if( wm.existOpponentIn( around , 10 , true ) || wm.existTeammateIn( around , 10 , false ) )
	{
		//rcsc::Body_StopBall().execute( agent );
		return false;
	}
	if( wm.self().body().abs() > 20 )
	{
		/*
	    rcsc::AngleDeg deg = wm.ball().rpos().th();
		if( wm.time().cycle() % 3 == 0 )
			deg += 110;
		else
		if( wm.time().cycle() % 3 == 1 )
			deg -= 110;
			
		if( deg.degree() > 180 )
			deg -= (int)( ( deg.degree() + 180 ) / 360 ) * 360.0;
		else
		if( deg.degree() < -180 )
			deg += (int)( ( deg.degree() - 180 ) / 360 ) * 360.0;
			
		Body_TurnToAngle( deg ).execute( agent );
		*/
		
		//std::cout<<"dash turn"<<std::endl;
		//rcsc::Body_TurnToAngle( 0.0 ).execute( agent );
		//rcsc::Body_TurnToBall().execute( agent );
		//agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return false;
	}
	
	double dist = 2.0;
    rcsc::Sector2D space( wm.ball().pos() , 0.5 , 17.0 , -40 , 40 );
    if(! wm.existOpponentIn(space , 10 , true) && ! wm.existTeammateIn( around , 10 , false ) )
		dist = 5.0;
	else
	{
		rcsc::Sector2D space( wm.ball().pos() , 0.5 , 11.0 , -40 , 40 );
		if( wm.existOpponentIn(space , 10 , true) || wm.existTeammateIn( around , 10 , false ) )
		{
			//rcsc::Body_StopBall().execute( agent );
			return false;
		}
	}
	rcsc::Vector2D end( 46.0 , wm.ball().pos().y );
	rcsc::Sector2D space1( wm.ball().pos() , 0.5 , wm.ball().pos().dist( end ), -40 , 40 );
	if( ! wm.existOpponentIn(space1 , 10 , true) && ! wm.existTeammateIn( around , 10 , false ) )
		dist = 7.0;
	
	if( wm.self().vel().r() < 0.2 )
		dist = 3.0;
	
	rcsc::Vector2D target_point = wm.ball().pos() + rcsc::Vector2D::polar2vector( dist , 0.0 );
	
	if( target_point.x > 40 && target_point.absY() < 15 )
	{
		/*
		rcsc::Vector2D goalie_pos;
		const rcsc::PlayerObject * goalie = wm.getOpponentGoalie();
		if( ! goalie )
			goalie_pos.assign( 51 , 0 );
		else
			goalie_pos = goalie->pos();
		if( goalie_pos.dist( wm.ball().pos() ) > 5.0 )
			dist = 3.0;
		*/
		//rcsc::Body_StopBall().execute( agent );
		return false;
	}
	/*
	int predict_player_reach_cycle( const rcsc::AbstractPlayerObject * player,
								const rcsc::Vector2D & target_point,
								const double & dist_thr,
								const double & penalty_distance,
								const int body_count_thr,
								const int default_n_turn,
								const int wait_cycle,
								const bool use_back_dash );

    */
    
    if( target_point.absX() > 45 || target_point.absY() > 31 )
    {
		//rcsc::Body_StopBall().execute( agent );
		return false;
	}

	double end_speed = 0.8;
    double first_speed = 2.0;
    do
    {
        first_speed = rcsc::calc_first_term_geom_series_last( end_speed, wm.ball().pos().dist( target_point ) , rcsc::ServerParam::i().ballDecay() );
        if ( first_speed < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        end_speed -= 0.05;
    }
    while ( end_speed > 0.1 );

	first_speed = std::min( first_speed , rcsc::ServerParam::i().ballSpeedMax() ); 
	first_speed *= rcsc::ServerParam::i().ballDecay();
    
	//if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, 3 ).execute( agent ) )
	if ( ! Body_KickOneStep( target_point, first_speed ).execute( agent ) )
	{
		//rcsc::Body_StopBall().execute( agent );
		return false;
	}
	std::cout<<"dash"<<std::endl;
	Body_Pass::say_pass( agent , wm.self().unum() , target_point );
    //agent->setIntention( new IntentionReceive( target_point, rcsc::ServerParam::i().maxDashPower(), 0.9, 5, wm.time() ) );
	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    return true;
}

/*------------------------------------------*/
bool
Body_Dash::test( rcsc::PlayerAgent * agent )
{
	if ( ! Strategy::i().dash() )
    {
		return false;
    }

	const rcsc::WorldModel & wm = agent->world();
		
    if ( ! wm.self().isKickable() )
        return false;
	
	if( wm.self().pos().x < 0.0 )
		return false;
	
	
	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.55 )
	{
		return false;
	}
	
	if( wm.self().body().abs() > 20 )
	{
		//rcsc::Body_TurnToAngle( 0.0 ).execute( agent );
		//agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		//return true;
		return false;
	}
	
	rcsc::Sector2D around( wm.self().pos() , 0.5 , 8.0 , -50 , 50 );
	if( wm.existOpponentIn( around , 10 , true ) || wm.existTeammateIn( around , 10 , false ) )
	{
		return false;
	}
	
	double dist = 2.0;
    rcsc::Sector2D space( wm.ball().pos() , 0.5 , 17.0 , -40 , 40 );
    if(! wm.existOpponentIn(space , 10 , true) && ! wm.existTeammateIn( around , 10 , false ) )
		dist = 5.0;
	else
	{
		rcsc::Sector2D space( wm.ball().pos() , 0.5 , 11.0 , -40 , 40 );
		if( wm.existOpponentIn(space , 10 , true) || wm.existTeammateIn( around , 10 , false ) )
		{
			return false;
		}
	}
	rcsc::Vector2D end( 46.0 , wm.ball().pos().y );
	rcsc::Sector2D space1( wm.ball().pos() , 0.5 , wm.ball().pos().dist( end ), -40 , 40 );
	if( ! wm.existOpponentIn(space1 , 10 , true) && ! wm.existTeammateIn( around , 10 , false ) )
		dist = 7.0;
	
	
	//std::cout<<wm.self().vel().r()<<std::endl;
	if( wm.self().vel().r() < 0.2 )
		dist = 3.0;
		
    rcsc::Vector2D target_point = wm.ball().pos() + rcsc::Vector2D::polar2vector( dist , 0.0 );

	if( target_point.x > 40 && target_point.absY() < 15 )
	{
		return false;
	}
	
    if( target_point.absX() > 45 || target_point.absY() > 31 )
    {
		return false;
	}
	
    return true;
}

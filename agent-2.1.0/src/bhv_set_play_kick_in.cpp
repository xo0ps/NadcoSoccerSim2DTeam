// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

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

#include "bhv_set_play_kick_in.h"
#include "bhv_mark.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "body_direct_pass.h"
#include "body_leading_pass.h"
#include "body_clear_ball.h"
#include "body_pass.h"

#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
bool
Bhv_SetPlayKickIn::execute( rcsc::PlayerAgent * agent )
{
    //if ( isKicker( agent ) )
    if ( Bhv_SetPlay::is_kicker( agent ) )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlayKickIn::isKicker( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.teammatesFromBall().empty() )
    {
        return true;
    }

    long max_wait1 = 30;
    long max_wait2 = 50;

    const rcsc::PlayerObject * nearest_mate = static_cast< rcsc::PlayerObject * >( 0 );
    nearest_mate = wm.teammatesFromBall().front();

    if ( wm.setplayCount() < max_wait1
         || ( wm.setplayCount() < max_wait2
              && wm.self().pos().dist( M_home_pos ) > 20.0 )
         || ( nearest_mate
              && nearest_mate->distFromBall() < wm.ball().distFromSelf() * 0.9 )
         )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayKickIn::doKick( rcsc::PlayerAgent * agent )
{
	/*
    const rcsc::WorldModel & wm = agent->world();

    rcsc::AngleDeg ball_place_angle = ( wm.ball().pos().y > 0.0 ? -90.0 : 90.0 );

    if ( Bhv_PrepareSetPlayKick( ball_place_angle, 10 ).execute( agent ) )
    //if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        // go to kick point
        return;
    }
	
    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }
    
    const rcsc::Vector2D face_point( 40.0, 0.0 );
    const rcsc::AngleDeg face_angle = ( face_point - wm.self().pos() ).th();
    
    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

	static int S_scan_count = -5;

    
    if ( S_scan_count < 10 && wm.teammatesFromSelf().empty() )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( wm.time().stopped() != 0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

	if ( S_scan_count < rcsc::ServerParam::i().dropBallTime() - 5 )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    S_scan_count = -5;
    */
    
    const rcsc::WorldModel & wm = agent->world();

    rcsc::AngleDeg ball_place_angle = ( wm.ball().pos().y > 0.0 ? -90.0 : 90.0 );

    if ( Bhv_PrepareSetPlayKick( ball_place_angle, 10 ).execute( agent ) )
    {
        // go to kick point
        return;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

	rcsc::Vector2D point( -100 , -100 );
	Body_DirectPass::test_new( agent , &point );
	if( point.x > -36.0 
	|| ( point.x < -36.0 && point.absY() > 20 ) )
	{
        //double ball_speed = rcsc::calc_first_term_geom_series_last( 1.2, wm.ball().pos().dist( point ), rcsc::ServerParam::i().ballDecay() );
        double ball_speed = Body_Pass::first_speed( agent , point , 'd' );
        Body_KickOneStep( point, ball_speed ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
	}

    const rcsc::PlayerObject * receiver = wm.getTeammateNearestToBall( 10 );
    if ( receiver
         && receiver->distFromBall() < 10.0
         && receiver->pos().absX() < rcsc::ServerParam::i().pitchHalfLength()
         && receiver->pos().absY() < rcsc::ServerParam::i().pitchHalfWidth() )
    {
        rcsc::Vector2D target_point = receiver->pos();
        target_point.x += 0.6;

        //double ball_speed = rcsc::calc_first_term_geom_series_last( 1.2, wm.ball().pos().dist( target_point ), rcsc::ServerParam::i().ballDecay() );
        double ball_speed = Body_Pass::first_speed( agent , target_point , 'd' );
        
        Body_KickOneStep( target_point, ball_speed ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

	/*
    if ( wm.self().pos().x < 20.0 )
    {
        rcsc::Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }
    */

	//rcsc::Vector2D target_point( 48.0, 34.0 - ( 34.0 * wm.self().pos().x / 52.5 ) );
	rcsc::Vector2D target_point( 48.0, 25.0 );
	if ( wm.self().pos().y < 0.0 )
		target_point.y *= -1.0;
	
	//double ball_speed = rcsc::calc_first_term_geom_series_last( 1.0 , wm.ball().pos().dist( target_point ), rcsc::ServerParam::i().ballDecay() );
	double ball_speed = Body_Pass::first_speed( agent , target_point , 'd' );
	Body_KickOneStep( target_point, ball_speed ).execute( agent );
	agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayKickIn::doMove( rcsc::PlayerAgent * agent )
{
	if( Bhv_Mark( "markEscape", M_home_pos ).execute( agent ) )
		return;

	rcsc::Vector2D target_point = M_home_pos;
    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = agent->world().ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
    }
    
    if ( //agent->world().self().pos().dist( target_point ) > std::max( agent->world().ball().pos().dist( target_point ) * 0.2 , 0.5 ) + 6.0  || 
	agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.5
       )
    {
        //agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

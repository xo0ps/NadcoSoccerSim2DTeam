// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

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

#include "bhv_set_play_free_kick.h"
#include "bhv_set_play_goalie_catch.h"
#include "body_direct_pass.h"
#include "body_leading_pass.h"
#include "body_clear_ball.h"
#include "bhv_mark.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"

#include <rcsc/player/say_message_builder.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/body_turn_to_angle.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_SetPlayFreeKick::execute( rcsc::PlayerAgent * agent )
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
Bhv_SetPlayFreeKick::isKicker( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    long max_wait1 = 30;
    long max_wait2 = 50;

    // goalie catch mode && I'm not a goalie
    if ( wm.gameMode().type() == rcsc::GameMode::GoalieCatch_
         && wm.gameMode().side() == wm.ourSide()
         && ! wm.self().goalie() )
    {
        return false;
    }

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
Bhv_SetPlayFreeKick::doKick( rcsc::PlayerAgent * agent )
{

    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }

    if ( doKickWait( agent ) )
    {
        return;
    }

    const rcsc::WorldModel & wm = agent->world();
    const double max_kick_speed = wm.self().kickRate() * rcsc::ServerParam::i().maxPower();
    const rcsc::PlayerObject * nearest_teammate = wm.getTeammateNearestToSelf( 10, false ); // without goalie

    double ball_speed = 0.0;
    rcsc::Vector2D target_point( -100 , -100 );
	Body_DirectPass::test_new( agent , &target_point );
	if( target_point.x > -25.0 )
	{
        ball_speed = rcsc::calc_first_term_geom_series_last( 1.4, target_point.dist( wm.ball().pos() ), rcsc::ServerParam::i().ballDecay() );
    }
    else if ( nearest_teammate
              && nearest_teammate->distFromSelf() < 35.0
              && ( nearest_teammate->pos().x > -30.0
                   || nearest_teammate->distFromSelf() < 9.0 ) )
    {
        double dist = wm.teammatesFromSelf().front()->distFromSelf();
        target_point = wm.teammatesFromSelf().front()->pos();
        ball_speed= rcsc::calc_first_term_geom_series_last( 1.4, dist, rcsc::ServerParam::i().ballDecay() );
        ball_speed = std::min( ball_speed, max_kick_speed );
    }
    else
    {
        target_point = rcsc::Vector2D( rcsc::ServerParam::i().pitchHalfLength(),
                              static_cast< double >( -1 + 2 * wm.time().cycle() % 2 )
                              * 0.8 * rcsc::ServerParam::i().goalHalfWidth() );
        ball_speed = max_kick_speed;
    }
    
	Body_KickOneStep( target_point, ball_speed ).execute( agent );
	agent->setNeckAction( new rcsc::Neck_ScanField() );
	return;

    if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 1.5 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    Body_ClearBall().execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayFreeKick::doMove( rcsc::PlayerAgent * agent )
{		
    const rcsc::WorldModel & wm = agent->world();
	
	if( Bhv_Mark( "markEscape", M_home_pos ).execute( agent ) )
		return;

    rcsc::Vector2D target_point = M_home_pos;
    //rcsc::Vector2D target_point = Bhv_SetPlayGoalieCatch::getMovePoint( agent );
    
    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 0.5;

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
    {
	    rcsc::Body_TurnToBall().execute( agent );
    }

    if ( //wm.self().pos().dist( target_point ) > std::max( wm.ball().pos().dist( target_point ) * 0.2 , 0.5 ) + 6.0 || 
	wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7 
       )
    {
        //agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}


/*------------------------------------------------------------*/
bool
Bhv_SetPlayFreeKick::doKickWait( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const int real_set_play_count = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

    if ( real_set_play_count >= rcsc::ServerParam::i().dropBallTime() - 5 )
    {
        return false;
    }

    const rcsc::Vector2D face_point( 40.0, 0.0 );
    const rcsc::AngleDeg face_angle = ( face_point - wm.self().pos() ).th();

    if ( wm.time().stopped() != 0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.teammatesFromBall().empty() )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() <= 3 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() >= 15
         && wm.seeTime() == wm.time()
         && wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.6 )
    {
        return false;
    }

    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

	/*
    if ( wm.seeTime() != wm.time() || wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.9 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }
    */

    return false;
}

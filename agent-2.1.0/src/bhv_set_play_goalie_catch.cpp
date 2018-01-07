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

#include "bhv_set_play_goalie_catch.h"

#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "bhv_mark.h"

#include <rcsc/player/say_message_builder.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_SetPlayGoalieCatch::execute( rcsc::PlayerAgent * agent )
{

    if ( isKicker( agent ) )
    //if ( Bhv_SetPlay::is_kicker( agent ) )
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
Bhv_SetPlayGoalieCatch::isKicker( const rcsc::PlayerAgent * agent )
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
Bhv_SetPlayGoalieCatch::doKick( rcsc::PlayerAgent * agent )
{
    // I am kickaer
    static int S_scan_count = -5;

    // go to ball
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }

    // already ball point

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D face_point( 40.0, 0.0 );
    const rcsc::AngleDeg face_angle = ( face_point - wm.self().pos() ).th();


    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( S_scan_count < 0 )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

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

    S_scan_count = -5;

    const double max_kick_speed = wm.self().kickRate() * rcsc::ServerParam::i().maxPower();

    rcsc::Vector2D target_point;
    double ball_speed = 0.0;

    {
        double dist = wm.teammatesFromSelf().front()->distFromSelf();
        target_point = wm.teammatesFromSelf().front()->pos();
        ball_speed = rcsc::calc_first_term_geom_series_last( 1.4, dist, rcsc::ServerParam::i().ballDecay() );
        ball_speed = std::max( ball_speed, wm.self().playerType().playerSize() + wm.self().playerType().kickableArea() );
        ball_speed = std::min( ball_speed, max_kick_speed );
        ball_speed = std::min( ball_speed , rcsc::ServerParam::i().maxPower() );
    }

    Body_KickOneStep( target_point, ball_speed ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayGoalieCatch::doMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
	
	//if( agent->world().self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.9 )
	if( Bhv_Mark( "markEscape" , M_home_pos ).execute( agent ) )
		return;

    rcsc::Vector2D target_point = getMovePoint( agent );
    /*
      if ( wm.gameMode().type() == rcsc::GameMode::GoalieCatch_
      && wm.gameMode().side() == wm.ourSide()
      && wm.self().unum() <= 8 )
      {
      target_point.x += 15.0;
      }
    */

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;
    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Body_TurnToBall().execute( agent );
    }
    if ( //agent->world().self().pos().dist( target_point ) > std::max( agent->world().ball().pos().dist( target_point ) * 0.2 , 0.5 ) + 6.0  || 
	agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7
       )
    {
        //agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Vector2D
Bhv_SetPlayGoalieCatch::getMovePoint( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    if ( wm.ball().pos().dist( M_home_pos ) > 20.0 )
    {
        return M_home_pos;
    }

    if ( wm.ball().pos().x < 0.0 && M_home_pos.x < wm.ball().pos().x + 5.0 )
    {
        return M_home_pos;
    }

    // prepare target point set
    rcsc::Vector2D home_ball_rel = M_home_pos - wm.ball().pos();
    rcsc::AngleDeg home_ball_angle = home_ball_rel.th();
    double home_ball_dist = home_ball_rel.r();

    const double circum_div = 7.0;
    double angle_div = circum_div / ( 2.0 * home_ball_dist * M_PI ) * 360.0;

    std::vector< rcsc::Vector2D > targets;
    targets.push_back( M_home_pos );
    targets.push_back( wm.ball().pos() + rcsc::Vector2D::polar2vector( home_ball_dist, home_ball_angle + angle_div ) );
    targets.push_back( wm.ball().pos() + rcsc::Vector2D::polar2vector( home_ball_dist, home_ball_angle + angle_div ) );

    rcsc::Vector2D target_point = M_home_pos;
    double max_opp_dist = 0.0;

    const rcsc::PlayerPtrCont & opps = wm.opponentsFromBall();
    const rcsc::PlayerPtrCont::const_iterator o_end = opps.end();

    // check opponet distance from each point
    for ( std::vector< rcsc::Vector2D >::iterator it = targets.begin(); it != targets.end(); ++it )
    {
        if ( it->x < -40.0 || 48.0 < it->x || it->y < -30.0 || 30.0 < it->y )
        {
            continue;
        }

        double min_dist = 1000.0;
        for ( rcsc::PlayerPtrCont::const_iterator opp = opps.begin(); opp != o_end; ++opp )
        {
            if ( (*opp)->distFromBall() > 40.0 )
            {
                break;
            }

            double dtmp = (*opp)->pos().dist( *it );
            if ( dtmp < min_dist )
            {
                min_dist = dtmp;
            }
        }
       if ( min_dist > max_opp_dist )
        {
            target_point = *it;
            max_opp_dist = min_dist;
        }
    }

    return target_point;
}

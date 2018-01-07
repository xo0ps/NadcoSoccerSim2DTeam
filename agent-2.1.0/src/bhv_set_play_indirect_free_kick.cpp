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

#include "bhv_set_play_indirect_free_kick.h"
#include "strategy.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_pass.h"
#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "body_direct_pass.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_kick_collide_with_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/math_util.h>


/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayIndirectFreeKick::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const bool our_kick = ( ( wm.gameMode().type() == rcsc::GameMode::BackPass_
                              && wm.gameMode().side() == wm.theirSide() )
                            || ( wm.gameMode().type() == rcsc::GameMode::IndFreeKick_
                                 && wm.gameMode().side() == wm.ourSide() )
                            || ( wm.gameMode().type() == rcsc::GameMode::FoulCharge_
                                 && wm.gameMode().side() == wm.theirSide() )
                            || ( wm.gameMode().type() == rcsc::GameMode::FoulPush_
                                 && wm.gameMode().side() == wm.theirSide() )
                            );

    if ( our_kick )
    {
        if ( Bhv_SetPlay::is_kicker( agent ) )
        {
            doKicker( agent );
        }
        else
        {
            doOffenseMove( agent );
        }
    }
    else
    {   
		doDefenseMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayIndirectFreeKick::doKicker( rcsc::PlayerAgent * agent )
{
    // go to ball
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }

    //
    // wait
    //

    if ( doKickWait( agent ) )
    {
        return;
    }

    //
    // kick to the teammate exist at the front of their goal
    //

    if ( doKickToShooter( agent ) )
    {
        return;
    }

    const rcsc::WorldModel & wm = agent->world();

    const double max_kick_speed = wm.self().kickRate() * rcsc::ServerParam::i().maxPower();

    //
    // pass
    //
    //rcsc::Vector2D point( 100 ,100 );
    //Body_DirectPass::test_new( agent , &point );
    //if( Body_DirectPass("playOn").execute( agent ) )
    //{
	//	agent->setNeckAction( new rcsc::Neck_ScanField() );
    //    return;
	//}
	
	if ( ! wm.teammatesFromBall().empty() )
	{
		rcsc::Vector2D point = wm.teammatesFromBall().front()->pos();
		if( Body_KickOneStep( point, 2.0 ).execute( agent ) )
		{
			agent->setNeckAction( new rcsc::Neck_ScanField() );
			return;
		}
	}
    
    //
    // wait(2)
    //
    if ( wm.setplayCount() <= 3 )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 50.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    //
    // no teammate
    //
    if ( wm.teammatesFromBall().empty()
         || wm.teammatesFromBall().front()->distFromSelf() > 35.0
         || wm.teammatesFromBall().front()->pos().x < -30.0 )
    {
        const int real_set_play_count = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

        if ( real_set_play_count <= rcsc::ServerParam::i().dropBallTime() - 3 )
        {
            rcsc::Body_TurnToPoint( rcsc::Vector2D( 50.0, 0.0 ) ).execute( agent );
            agent->setNeckAction( new rcsc::Neck_ScanField() );
            return;
        }

        rcsc::Vector2D target_point( rcsc::ServerParam::i().pitchHalfLength(), static_cast< double >( -1 + 2 * wm.time().cycle() % 2 ) * ( rcsc::ServerParam::i().goalHalfWidth() - 0.8 ) );
        double ball_speed = max_kick_speed;

        Body_KickOneStep( target_point, ball_speed ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    //
    // kick to the teammate nearest to opponent goal
    //

    const rcsc::Vector2D goal( rcsc::ServerParam::i().pitchHalfLength(), wm.self().pos().y * 0.8 );

    double min_dist = 100000.0;
    const rcsc::PlayerObject * receiver = static_cast< const rcsc::PlayerObject * >( 0 );

    const rcsc::PlayerPtrCont::const_iterator t_end = wm.teammatesFromBall().end();
    for ( rcsc::PlayerPtrCont::const_iterator t = wm.teammatesFromBall().begin(); t != t_end; ++t )
    {
		if( !(*t) ) continue;
        if ( (*t)->posCount() > 5 ) continue;
        if ( (*t)->distFromBall() < 1.5 ) continue;
        if ( (*t)->distFromBall() > 20.0 ) continue;
        if ( (*t)->pos().x > wm.offsideLineX() ) continue;

        double dist = (*t)->pos().dist( goal ) + (*t)->distFromBall();
        if ( dist < min_dist )
        {
            min_dist = dist;
            receiver = (*t);
        }
    }

    rcsc::Vector2D target_point = goal;
    double target_dist = 10.0;
    bool no_say = false;
    if ( ! receiver )
    {
        target_dist = wm.teammatesFromSelf().front()->distFromSelf();
        target_point = wm.teammatesFromSelf().front()->pos();
        no_say = true;
    }
    else
    {
        target_dist = receiver->distFromSelf();
        target_point = receiver->pos();
        target_point.x += 0.6;
    }

    double ball_speed = rcsc::calc_first_term_geom_series_last( 1.8, target_dist, rcsc::ServerParam::i().ballDecay() );
    ball_speed = std::min( ball_speed, max_kick_speed );

    Body_KickOneStep( target_point, ball_speed ).execute( agent );
    if( ! no_say )
		Body_Pass::say_pass( agent , receiver->unum() , target_point );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayIndirectFreeKick::doKickWait( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D face_point( 50.0, 0.0 );
    const rcsc::AngleDeg face_angle = ( face_point - wm.self().pos() ).th();

    if ( wm.time().stopped() > 0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() <= 10 && wm.teammatesFromSelf().empty() )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayIndirectFreeKick::doKickToShooter( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::Vector2D goal( rcsc::ServerParam::i().pitchHalfLength(), wm.self().pos().y * 0.8 );

    double min_dist = 100000.0;
    const rcsc::PlayerObject * receiver = static_cast< const rcsc::PlayerObject * >( 0 );

    const rcsc::PlayerPtrCont::const_iterator t_end = wm.teammatesFromBall().end();
    for ( rcsc::PlayerPtrCont::const_iterator t = wm.teammatesFromBall().begin(); t != t_end; ++t )
    {
		if( ! (*t) ) continue;
        if ( (*t)->posCount() > 5 ) continue;
        if ( (*t)->distFromBall() > 20.0 ) continue;
        if ( (*t)->pos().x > wm.offsideLineX() ) continue;
        if ( (*t)->pos().x < wm.ball().pos().x - 3.0 ) continue;
        double goal_dist = (*t)->pos().dist( goal );
        if ( goal_dist > 16.0 )
        {
            continue;
        }

        double dist = goal_dist * 0.4 + (*t)->distFromBall() * 0.6;

        if ( dist < min_dist )
        {
            min_dist = dist;
            receiver = (*t);
        }
    }

    if ( ! receiver )
    {
        return false;
    }

    const double max_ball_speed = wm.self().kickRate() * rcsc::ServerParam::i().maxPower();

    rcsc::Vector2D target_point = receiver->pos() + receiver->vel();
    target_point.x += 0.6;

    double target_dist = wm.ball().pos().dist( target_point );

    int ball_reach_step = static_cast< int >( std::ceil( rcsc::calc_length_geom_series( max_ball_speed, target_dist, rcsc::ServerParam::i().ballDecay() ) ) );
    double ball_speed = rcsc::calc_first_term_geom_series( target_dist, rcsc::ServerParam::i().ballDecay(), ball_reach_step );

    ball_speed = std::min( ball_speed, max_ball_speed );
    Body_KickOneStep( target_point, ball_speed ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
	Body_Pass::say_pass( agent , receiver->unum() , target_point );
    return true;

}

namespace {

rcsc::Vector2D
get_avoid_circle_point( const rcsc::WorldModel & wm, rcsc::Vector2D point )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    const double circle_r
        = wm.gameMode().type() == rcsc::GameMode::BackPass_
        ? SP.goalAreaLength() + 0.5
        : SP.centerCircleR() + 0.5;
    const double circle_r2 = std::pow( circle_r, 2 );


    if ( point.x < -SP.pitchHalfLength() + 3.0 && point.absY() < SP.goalHalfWidth() )
    {
        while ( point.x < wm.ball().pos().x
                && point.x > - SP.pitchHalfLength()
                && wm.ball().pos().dist2( point ) < circle_r2 )
        {
            //point.x -= 0.2;
            point.x = ( point.x - SP.pitchHalfLength() ) * 0.5 - 0.01;
        }
    }

    if ( point.x < -SP.pitchHalfLength() + 0.5
         && point.absY() < SP.goalHalfWidth() + 0.5
         && wm.self().pos().x < -SP.pitchHalfLength()
         && wm.self().pos().absY() < SP.goalHalfWidth() )
    {
        return point;
    }

    if ( wm.ball().pos().dist2( point ) < circle_r2 )
    {
        rcsc::Vector2D rel = point - wm.ball().pos();
        rel.setLength( circle_r );
        point = wm.ball().pos() + rel;
    }

    return Bhv_SetPlay::get_avoid_circle_point2( wm, point );
}

} // end noname namespace

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayIndirectFreeKick::doOffenseMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
    target_point.x = std::min( wm.offsideLineX() - 1.0, target_point.x );

    double nearest_dist = 1000.0;
    const rcsc::PlayerObject * teammate = wm.getTeammateNearestTo( target_point, 10, &nearest_dist );
    if ( nearest_dist < 2.5 )
    {
        target_point += ( target_point - teammate->pos() ).setLengthVector( 2.5 );
        target_point.x = std::min( wm.offsideLineX() - 1.0, target_point.x );
    }

    //double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
    double dash_power = rcsc::ServerParam::i().maxDashPower();

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Vector2D turn_point = ( rcsc::ServerParam::i().theirTeamGoalPos() + wm.ball().pos() ) * 0.5;

        rcsc::Body_TurnToPoint( turn_point ).execute( agent );
    }
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayIndirectFreeKick::doDefenseMove( rcsc::PlayerAgent * agent )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
    rcsc::Vector2D adjusted_point = get_avoid_circle_point( wm, target_point );

    //double dash_power = wm.self().getSafetyDashPower( SP.maxDashPower() );
    double dash_power = rcsc::ServerParam::i().maxDashPower();

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    if ( adjusted_point != target_point
         && wm.ball().pos().dist( target_point ) > 10.0
         && wm.self().inertiaFinalPoint().dist( adjusted_point ) < dist_thr )
    {
        adjusted_point = target_point;
    }

    {
        const double collision_dist
            = wm.self().playerType().playerSize()
            + SP.goalPostRadius()
            + 0.2;

        rcsc::Vector2D goal_post_l( -SP.pitchHalfLength() + SP.goalPostRadius(),
                              -SP.goalHalfWidth() - SP.goalPostRadius() );
        rcsc::Vector2D goal_post_r( -SP.pitchHalfLength() + SP.goalPostRadius(),
                              +SP.goalHalfWidth() + SP.goalPostRadius() );
        double dist_post_l = wm.self().pos().dist( goal_post_l );
        double dist_post_r = wm.self().pos().dist( goal_post_r );

        const rcsc::Vector2D & nearest_post = ( dist_post_l < dist_post_r ? goal_post_l : goal_post_r );
        double dist_post = std::min( dist_post_l, dist_post_r );

        if ( dist_post < collision_dist + wm.self().playerType().realSpeedMax() + 0.5 )
        {
            rcsc::Circle2D post_circle( nearest_post, collision_dist );
            rcsc::Segment2D move_line( wm.self().pos(), adjusted_point );

            if ( post_circle.intersection( move_line, NULL, NULL ) > 0 )
            {
                rcsc::AngleDeg post_angle = ( nearest_post - wm.self().pos() ).th();
                if ( nearest_post.y < wm.self().pos().y )
                {
                    adjusted_point = nearest_post;
                    adjusted_point += rcsc::Vector2D::from_polar( collision_dist + 0.1, post_angle - 90.0 );
                }
                else
                {
                    adjusted_point = nearest_post;
                    adjusted_point += rcsc::Vector2D::from_polar( collision_dist + 0.1, post_angle + 90.0 );
                }
                dist_thr = 0.05;
            }
        }
    }

    if ( ! Body_GoToPoint( adjusted_point, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Body_TurnToBall().execute( agent );
    }

    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
}

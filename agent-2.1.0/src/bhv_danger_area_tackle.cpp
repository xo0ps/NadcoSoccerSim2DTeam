// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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
#include "bhv_danger_area_tackle.h"

#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DangerAreaTackle::execute( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    if ( clearGoal( agent ) )
    {
        return true;
    }

    if ( wm.self().tackleProbability() < M_min_probability )
    {
        return false;
    }
    bool ball_shall_be_in_our_goal = false;

    const double goal_half_width = sp.goalHalfWidth();

    const rcsc::Vector2D goal_center = sp.ourTeamGoalPos();
    const rcsc::Vector2D goal_left_post( goal_center.x, +goal_half_width );
    const rcsc::Vector2D goal_right_post( goal_center.x, -goal_half_width );
    bool is_shoot_ball = ( ( (goal_left_post - wm.ball().pos() ).th()
                             - wm.ball().vel().th() ).degree() < 0
                           && ( ( goal_right_post - wm.ball().pos() ).th()
                                - wm.ball().vel().th() ).degree() > 0 );

    const int self_reach_cycle = wm.interceptTable()->selfReachCycle();

    if ( is_shoot_ball
         && wm.ball().inertiaPoint( self_reach_cycle ).x
            <= sp.ourTeamGoalLineX() )
    {
        ball_shall_be_in_our_goal = true;
    }

    return executeV12( agent );
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DangerAreaTackle::clearGoal( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.self().tackleProbability() <= 0.0 )
    {
        return false;
    }

    const rcsc::ServerParam & param = rcsc::ServerParam::i();

    const int self_min = wm.interceptTable()->selfReachCycle();

    const rcsc::Vector2D self_trap_pos = wm.ball().inertiaPoint( self_min );
    if ( self_trap_pos.x > - param.pitchHalfLength() + 0.5 )
    {
        return false;
    }

    const rcsc::Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
    const rcsc::Line2D goal_line( rcsc::Vector2D( - param.pitchHalfLength(), 10.0 ),
                                  rcsc::Vector2D( - param.pitchHalfLength(), -10.0 ) );
    const rcsc::Vector2D intersect =  ball_ray.intersection( goal_line );
    if ( ! intersect.isValid()
         || intersect.absY() > param.goalHalfWidth() + 0.5 )
    {
        return false;
    }

    if ( agent->config().version() < 12.0 )
    {
        double tackle_power = ( wm.self().body().abs() > 90.0
                                ? param.maxTacklePower()
                                : - param.maxBackTacklePower() );

        agent->doTackle( tackle_power );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    const rcsc::Line2D line_c( rcsc::Vector2D( -param.pitchHalfLength(), 0.0 ),
                               rcsc::Vector2D( 0.0, 0.0 ) );
    const rcsc::Line2D line_l( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                               rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );
    const rcsc::Line2D line_r( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                               rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );

    const rcsc::AngleDeg ball_rel_angle
        = wm.ball().angleFromSelf() - wm.self().body();
    const double tackle_rate
        = ( param.tacklePowerRate()
            * ( 1.0 - 0.5 * ( ball_rel_angle.abs() / 180.0 ) ) );

    rcsc::AngleDeg best_angle = 0.0;
    double max_speed = -1.0;

    for ( double a = -180.0; a < 180.0; a += 10.0 )
    {
        rcsc::AngleDeg target_rel_angle = a - wm.self().body().degree();

        double eff_power = param.maxBackTacklePower()
            + ( ( param.maxTacklePower() - param.maxBackTacklePower() )
                * ( 1.0 - target_rel_angle.abs() / 180.0 ) );
        eff_power *= tackle_rate;

        rcsc::Vector2D vel = wm.ball().vel()
            + rcsc::Vector2D::polar2vector( eff_power, rcsc::AngleDeg( a ) );
        rcsc::AngleDeg vel_angle = vel.th();

        if ( vel_angle.abs() > 80.0 )
        {
            continue;
        }

        double speed = vel.r();

        int n_intersects = 0;
        if ( ball_ray.intersection( line_c ).isValid() ) ++n_intersects;
        if ( ball_ray.intersection( line_l ).isValid() ) ++n_intersects;
        if ( ball_ray.intersection( line_r ).isValid() ) ++n_intersects;

        if ( n_intersects == 3 )
        {
            speed -= 2.0;
        }
        else if ( n_intersects == 2
                  && wm.ball().pos().absY() > 3.0 )
        {
            speed -= 2.0;
        }

        if ( speed > max_speed )
        {
            max_speed = speed;
            best_angle = target_rel_angle + wm.self().body();
        }
    }

    if ( max_speed < 1.0 )
    {
        return false;
    }

    agent->doTackle( ( best_angle - wm.self().body() ).degree() );
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DangerAreaTackle::executeV12( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & param = rcsc::ServerParam::i();
    const rcsc::PlayerPtrCont::const_iterator o_end = wm.opponentsFromBall().end();

    const rcsc::Vector2D goal( - param.pitchHalfLength(), 0.0 );
    const rcsc::Vector2D virtual_accel = ( wm.existKickableOpponent()
                                           ? ( goal - wm.ball().pos() ).setLengthVector( 1.0 )
                                           : rcsc::Vector2D( 0.0, 0.0 ) );
    const rcsc::Line2D goal_line( rcsc::Vector2D( - param.pitchHalfLength(), 10.0 ),
                                  rcsc::Vector2D( - param.pitchHalfLength(), -10.0 ) );
    const rcsc::Line2D line_c( rcsc::Vector2D( -param.pitchHalfLength(), 0.0 ),
                               rcsc::Vector2D( 0.0, 0.0 ) );
    const rcsc::Line2D line_l( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                               rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );
    const rcsc::Line2D line_r( rcsc::Vector2D( -param.pitchHalfLength(), -param.goalHalfWidth() ),
                               rcsc::Vector2D( 0.0, -param.goalHalfWidth() ) );

    const rcsc::AngleDeg ball_rel_angle
        = wm.ball().angleFromSelf() - wm.self().body();
    const double tackle_rate
        = ( param.tacklePowerRate()
            * ( 1.0 - 0.5 * ( ball_rel_angle.abs() / 180.0 ) ) );

    rcsc::AngleDeg best_angle = 0.0;
    double max_speed = -1.0;

    for ( double a = -180.0; a < 180.0; a += 10.0 )
    {
        rcsc::AngleDeg rel_angle = a - wm.self().body().degree();

        double eff_power = param.maxBackTacklePower()
            + ( ( param.maxTacklePower() - param.maxBackTacklePower() )
                * ( 1.0 - rel_angle.abs() / 180.0 ) );
        eff_power *= tackle_rate;

        rcsc::Vector2D vel = ( wm.ball().vel()
                               + rcsc::Vector2D::polar2vector( eff_power, rcsc::AngleDeg( a ) ) );
        vel += virtual_accel;

        const rcsc::Ray2D ball_ray( wm.ball().pos(), vel.th() );
        const rcsc::Vector2D intersect =  ball_ray.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < param.goalHalfWidth() + 5.0 )
        {
            continue;
        }

        const rcsc::Vector2D ball_next = wm.ball().pos() + vel;

        bool maybe_opponent_get_ball = false;
        for ( rcsc::PlayerPtrCont::const_iterator o = wm.opponentsFromBall().begin();
              o != o_end;
              ++o )
        {
			if( !(*o) ) continue;
            if ( (*o)->posCount() > 10 ) continue;
            if ( (*o)->isGhost() ) continue;
            if ( (*o)->isTackling() ) continue;
            if ( (*o)->distFromBall() > 6.0 ) break;;

            rcsc::Vector2D opp_pos = (*o)->pos() + (*o)->vel();
            if ( opp_pos.dist( ball_next ) < 1.0 )
            {
                maybe_opponent_get_ball = true;
                break;
            }
        }

        if ( maybe_opponent_get_ball )
        {
            continue;
        }

        double speed = vel.r();


        int n_intersects = 0;
        if ( ball_ray.intersection( line_c ).isValid() ) ++n_intersects;
        if ( ball_ray.intersection( line_l ).isValid() ) ++n_intersects;
        if ( ball_ray.intersection( line_r ).isValid() ) ++n_intersects;

        if ( n_intersects == 3 )
        {
            speed -=2.0;
        }
        else if ( n_intersects == 2
                  && wm.ball().pos().absY() > 3.0 )
        {
            speed -= 2.0;
        }

        if ( speed > max_speed )
        {
            max_speed = speed;
            best_angle = a;
    }

    if ( max_speed < 1.0 )
    {
        return false;
    }

	}
    agent->doTackle( ( best_angle - wm.self().body() ).degree() );
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    /*
    double dist = 1000.0;
	const rcsc::PlayerObject * teammate = wm.getTeammateNearestTo( target_point , 5 , &dist );
	if( (! teammate) )
		return true;
	if( teammate->goalie() || teammate->unum() > 11 || teammate->unum() < 2 )
		return true;
	int unum = teammate->unum();
    Body_Pass::say_pass( agent , unum , ball_next );
    */

    return true;

}

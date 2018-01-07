// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "body_kick_one_step.h"

#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/ray_2d.h>

#include <algorithm>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickOneStep::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    if ( ! wm.self().isKickable() )
    {
        return false;
    }

    rcsc::Vector2D ball_vel = wm.ball().vel();

    if ( ! wm.ball().velValid() )
    {
        if ( ! M_force_mode )
        {
            return rcsc::Body_StopBall().execute( agent );
        }

        ball_vel.assign( 0.0, 0.0 );
    }

    M_first_speed = std::min( M_first_speed, sp.ballSpeedMax() );

    const rcsc::AngleDeg target_angle = ( M_target_point - wm.ball().pos() ).th();

    rcsc::Vector2D first_vel = get_max_possible_vel( target_angle,
                                               wm.self().kickRate(),
                                               ball_vel );
    if ( first_vel.r() > M_first_speed )
    {
        first_vel.setLength( M_first_speed );
    }
    else
    {
    }

    // first_vel.r() may be less than M_first_speed ...


    const rcsc::Vector2D kick_accel = first_vel - ball_vel;

    double kick_power = kick_accel.r() / wm.self().kickRate();
    const rcsc::AngleDeg kick_dir = kick_accel.th() - wm.self().body();

    if ( kick_power > sp.maxPower() + 0.01 )
    {
        if ( ! M_force_mode )
        {
            return rcsc::Body_HoldBall2008( true,
                                      M_target_point,
                                      M_target_point
                                      ).execute( agent );
        }
        kick_power = sp.maxPower();
    }

    M_ball_result_pos = wm.ball().pos() + first_vel;
    M_ball_result_vel = first_vel * sp.ballDecay();
    M_kick_step = 1;

    return agent->doKick( kick_power, kick_dir );
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Vector2D
Body_KickOneStep::get_max_possible_vel( const rcsc::AngleDeg & target_angle,
                                        const double & krate,
                                        const rcsc::Vector2D & ball_vel )
{
    // ball info may be the estimated value..
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    const double max_accel
        = std::min( sp.maxPower() * krate,
                    sp.ballAccelMax() );

    // origin point is current ball pos
    rcsc::Ray2D desired_ray( rcsc::Vector2D( 0.0, 0.0 ), target_angle );
    // center point is next ball pos relative to current ball pos
    rcsc::Circle2D next_reachable_circle( ball_vel, max_accel );


    // sol is ball vel (= current ball vel + accel)
    rcsc::Vector2D sol1, sol2; // rel to current ball pos
    int num = next_reachable_circle.intersection( desired_ray, &sol1, &sol2 );

    if ( num == 0 )
    {
        //return Vector2D( 0.0, 0.0 );
        rcsc::Vector2D accel = -ball_vel;
        double accel_r = accel.r();
        if ( accel_r > max_accel )
        {
            accel *= max_accel / accel_r;
        }
        return accel; // stop the ball
    }

    if ( num == 1 )
    {
        if ( sol1.r() > sp.ballSpeedMax() )
        {
            // next inertia ball point is within reachable circle.
            if ( next_reachable_circle.contains( rcsc::Vector2D( 0.0, 0.0 ) ) )
            {
                // can adjust angle at least
                sol1.setLength( sp.ballSpeedMax() );

            }
            else
            {
                // failed
                sol1.assign( 0.0, 0.0 );
            }
        }
        return sol1;
    }


    // num == 2

    double length1 = sol1.r();
    double length2 = sol2.r();

    if ( length1 < length2 )
    {
        std::swap( sol1, sol2 );
        std::swap( length1, length2 );
    }

    if ( length1 > sp.ballSpeedMax() )
    {
        if ( length2 > sp.ballSpeedMax() )
        {
            sol1.assign( 0.0, 0.0 );
        }
        else
        {
            sol1.setLength( sp.ballSpeedMax() );
        }
    }

    return sol1;
}

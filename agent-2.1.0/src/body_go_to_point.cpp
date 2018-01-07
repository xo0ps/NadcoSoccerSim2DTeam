// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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

#include "body_go_to_point.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_stop_dash.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/segment_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

#define USE_ADJUST_DASH

namespace {
const double adjustable_dist = 1.0; // Magic Number
const double ILLEGAL_POWER = -65535.0;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_GoToPoint::execute( rcsc::PlayerAgent * agent )
{
    if ( std::fabs( M_max_dash_power ) < 0.1
         || std::fabs( M_dash_speed ) < 0.001 )
    {
        agent->doTurn( 0.0 );
        return false;
    }

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D inertia_point = wm.self().inertiaPoint( M_cycle );
    rcsc::Vector2D target_rel = M_target_point - inertia_point;

    //
    // already there
    //
    double target_dist = target_rel.r();
    if ( target_dist < M_dist_thr )
    {
        agent->doTurn( 0.0 ); // dumy action
        return false;
    }

    //
    // if necessary, change the target point to avoid goal post
    //
    checkGoalPost( agent );

    //
    // omnidir dash
    //
#ifdef USE_ADJUST_DASH
    if ( doAdjustDash( agent ) )
    {
        return true;
    }
#endif

    //
    // turn
    //
    if ( doTurn( agent ) )
    {
        return true;
    }

    //
    // dash
    //
    if ( doDash( agent ) )
    {
        return true;
    }

    agent->doTurn( 0.0 ); // dummy action
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Body_GoToPoint::checkGoalPost( const rcsc::PlayerAgent * agent )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    const double collision_dist = wm.self().playerType().playerSize() + SP.goalPostRadius() + 0.2;

    rcsc::Vector2D goal_post_l( -SP.pitchHalfLength() + SP.goalPostRadius(),
                          -SP.goalHalfWidth() - SP.goalPostRadius() );
    rcsc::Vector2D goal_post_r( -SP.pitchHalfLength() + SP.goalPostRadius(),
                          +SP.goalHalfWidth() + SP.goalPostRadius() );

    double dist_post_l = wm.self().pos().dist2( goal_post_l );
    double dist_post_r = wm.self().pos().dist2( goal_post_r );

    const rcsc::Vector2D & nearest_post = ( dist_post_l < dist_post_r
                                      ? goal_post_l
                                      : goal_post_r );
    double dist_post = std::min( dist_post_l, dist_post_r );

    if ( dist_post > collision_dist + wm.self().playerType().realSpeedMax() + 0.5 )
    {
        return;
    }

    rcsc::Circle2D post_circle( nearest_post, collision_dist );
    rcsc::Segment2D move_line( wm.self().pos(), M_target_point );

    if ( post_circle.intersection( move_line, NULL, NULL ) == 0 )
    {
        return;
    }

    rcsc::AngleDeg post_angle = ( nearest_post - wm.self().pos() ).th();
    rcsc::Vector2D new_target = nearest_post;

    if ( post_angle.isLeftOf( wm.self().body() ) )
    {
        new_target += rcsc::Vector2D::from_polar( collision_dist + 0.1, post_angle + 90.0 );
    }
    else
    {
        new_target += rcsc::Vector2D::from_polar( collision_dist + 0.1, post_angle - 90.0 );
    }

    M_target_point = new_target;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_GoToPoint::doAdjustDash( rcsc::PlayerAgent * agent )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D inertia_point = wm.self().inertiaPoint( M_cycle );
    rcsc::Vector2D target_rel = M_target_point - inertia_point;

    if ( target_rel.r2() > std::pow( M_dist_thr + adjustable_dist, 2 ) )
    {
        return false;
    }

    target_rel.rotate( - wm.self().body() );

    if ( target_rel.absY() < M_dist_thr )
    {
        return false;
    }

    const rcsc::AngleDeg target_angle = target_rel.th();

    if ( target_angle.abs() < M_dir_thr )
    {
        return false;
    }

    const rcsc::Vector2D rel_vel = wm.self().vel().rotatedVector( - wm.self().body() );

    const double dash_angle_step = std::max( 15.0, SP.dashAngleStep() );
    const double min_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.minDashAngle()
                                    : dash_angle_step * static_cast< int >( -180.0 / dash_angle_step ) );
    const double max_dash_angle = ( -180.0 < SP.minDashAngle() && SP.maxDashAngle() < 180.0
                                    ? SP.maxDashAngle() + dash_angle_step * 0.5
                                    : dash_angle_step * static_cast< int >( 180.0 / dash_angle_step ) - 1.0 );

    double best_dir = -360.0;
    double best_dist = 1000000.0;
    int best_cycle = 1000;
    double best_dash_power = 0.0;
    double best_stamina = 0.0;

    for ( double dir = min_dash_angle;
          dir < max_dash_angle;
          dir += dash_angle_step )
    {
        if ( std::fabs( dir ) < 0.5 ) continue;
        if ( std::fabs( dir ) > 100.0 ) continue; // Magic Number

        const rcsc::AngleDeg dash_angle = SP.discretizeDashAngle( SP.normalizeDashAngle( dir ) );

        if ( ( dash_angle - target_angle ).abs() > 90.0 )
        {
            continue;
        }

        const double dash_rate = wm.self().dashRate() * SP.dashDirRate( dir );
        //
        // check if player can adjust y diff with few dashes.
        //

        const int max_cycle = std::min( 3, M_cycle );

        rcsc::Vector2D my_pos( 0.0, 0.0 );
        rcsc::Vector2D my_vel = rel_vel;
        rcsc::StaminaModel stamina_model = wm.self().staminaModel();
        double first_dash_power = ILLEGAL_POWER;

        my_pos = wm.self().playerType().inertiaPoint( my_pos, rel_vel, M_cycle );

        int cycle = 0;
        for ( ; cycle < max_cycle; ++cycle )
        {
            rcsc::Vector2D required_move = target_rel - my_pos;
            required_move.rotate( - dash_angle );

            double required_x_accel
                = rcsc::calc_first_term_geom_series( required_move.x,
                                               wm.self().playerType().playerDecay(),
                                               M_cycle - cycle );

            if ( required_x_accel < 0.01 )
            {
                break;
            }

            double required_dash_power = required_x_accel / dash_rate;
            double available_stamina = ( M_save_recovery
                                         ? std::max( 0.0, stamina_model.stamina() - SP.recoverDecThrValue() - 1.0 )
                                         : stamina_model.stamina() + wm.self().playerType().extraStamina() );

            double dash_power = std::min( available_stamina, M_max_dash_power );
            dash_power = std::min( dash_power, required_dash_power );
            dash_power = std::min( dash_power, SP.maxDashPower() );

            if ( cycle == 0 )
            {
                first_dash_power = dash_power;
            }

            rcsc::Vector2D accel = rcsc::Vector2D::polar2vector( dash_power * dash_rate,
                                                     dash_angle );
            my_vel += accel;
            my_pos += my_vel;
            my_vel *= wm.self().playerType().playerDecay();

            stamina_model.simulateDash( wm.self().playerType() , dash_power );
        }

        if ( first_dash_power == ILLEGAL_POWER )
        {
            continue;
        }

        double last_dist = my_pos.dist( target_rel );
        if ( last_dist < M_dist_thr )
        {
            bool update = false;
            if ( last_dist < 0.1
                 || std::fabs( last_dist - best_dist ) < 0.001 )
            {
                if ( cycle < best_cycle
                     || stamina_model.stamina() > best_stamina )
                {
                    update = true;
                }
            }
            else if ( last_dist < best_dist )
            {
                update = true;
            }

            if ( update )
            {
                best_dir = dir;
                best_dist = last_dist;
                best_cycle = cycle;
                best_dash_power = first_dash_power;
                best_stamina = stamina_model.stamina();
            }
        }
    }

    if ( best_dir != -360.0 )
    {
        rcsc::AngleDeg dash_angle = SP.discretizeDashAngle( SP.normalizeDashAngle( best_dir ) );
        return agent->doDash( best_dash_power, dash_angle );
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_GoToPoint::doTurn( rcsc::PlayerAgent * agent )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D inertia_pos = wm.self().inertiaPoint( M_cycle );
    rcsc::Vector2D target_rel = M_target_point - inertia_pos;

    const double target_dist = target_rel.r();
    const double max_turn = wm.self().playerType().effectiveTurn( SP.maxMoment(),
                                                                  wm.self().vel().r() );

    rcsc::AngleDeg turn_moment = target_rel.th() - wm.self().body();

    // if target is very near && turn_angle is big && agent has enough stamina,
    // it is useful to reverse accel angle.
    if ( turn_moment.abs() > max_turn
         && turn_moment.abs() > 90.0
         && target_dist < 2.0
         && wm.self().stamina() > SP.recoverDecThrValue() + 500.0 )
    {
        double effective_power = SP.maxDashPower() * wm.self().dashRate();
        double effective_back_power = SP.minDashPower() * wm.self().dashRate();
        if ( std::fabs( effective_back_power ) > std::fabs( effective_power ) * 0.75 )
        {
            M_back_mode = true;
            turn_moment += 180.0;
        }
    }

    double turn_thr = 180.0;
// #ifdef USE_ADJUST_DASH
//     if ( M_dist_thr + adjustable_dist < target_dist )
//     {
//         turn_thr = AngleDeg::asin_deg( std::min( 1.0, ( M_dist_thr + adjustable_dist ) / target_dist ) );
//     }
// #else
    if ( M_dist_thr < target_dist )
    {
        turn_thr = rcsc::AngleDeg::asin_deg( M_dist_thr / target_dist );
    }
// #endif

    turn_thr = std::max( M_dir_thr, turn_thr );

    //
    // it is not necessary to perform turn action.
    //
    if ( turn_moment.abs() < turn_thr )
    {
        return false;
    }

    //
    // register turn command
    //
    return agent->doTurn( turn_moment );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_GoToPoint::doDash( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D inertia_pos = wm.self().inertiaPoint( M_cycle );
    rcsc::Vector2D target_rel = M_target_point - inertia_pos;

    rcsc::AngleDeg accel_angle = wm.self().body();
    if ( M_back_mode )
    {
        accel_angle += 180.0;
    }

    target_rel.rotate( -accel_angle ); // required_dash_dist == target_rel.x

    // consider inertia travel
    double first_speed
        = rcsc::calc_first_term_geom_series( target_rel.x,
                                       wm.self().playerType().playerDecay(),
                                       M_cycle );
    first_speed = rcsc::bound( - wm.self().playerType().playerSpeedMax(),
                         first_speed,
                         wm.self().playerType().playerSpeedMax() );
    if ( M_dash_speed > 0.0 )
    {
        if ( first_speed > 0.0 )
        {
            first_speed = std::min( first_speed, +M_dash_speed );
        }
        else
        {
            first_speed = std::max( first_speed, -M_dash_speed );
        }
    }

    rcsc::Vector2D rel_vel = wm.self().vel();
    rel_vel.rotate( -accel_angle );

    double required_accel = first_speed - rel_vel.x;


    if ( std::fabs( required_accel ) < 0.05 )
    {
        // ------- no action -------
        return false;
    }

    double dash_power = required_accel / wm.self().dashRate();
    dash_power = std::min( dash_power, M_max_dash_power );
    if ( M_back_mode )
    {
        dash_power = -dash_power;
    }
    dash_power = rcsc::ServerParam::i().normalizeDashPower( dash_power );


    if ( M_save_recovery )
    {
        dash_power = wm.self().getSafetyDashPower( dash_power );
    }

    return agent->doDash( dash_power );
}
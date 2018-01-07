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

#include "bhv_predictor.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

#include <limits>

/*-------------------------------------------------------------------*/
/*!

 */
int
Bhv_Predictor::predictOpponentsReachStep( const rcsc::WorldModel & wm,
                                            const rcsc::Vector2D & first_ball_pos,
                                            const rcsc::Vector2D & first_ball_vel,
                                            const rcsc::AngleDeg & ball_move_angle )
{
    int first_min_step = 50;

#if 1
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const rcsc::Vector2D ball_end_point = inertia_final_point( first_ball_pos,
                                                         first_ball_vel,
                                                         SP.ballDecay() );
    if ( ball_end_point.absX() > SP.pitchHalfLength()
         || ball_end_point.absY() > SP.pitchHalfWidth() )
    {
        rcsc::Rect2D pitch = rcsc::Rect2D::from_center( 0.0, 0.0, SP.pitchLength(), SP.pitchWidth() );
        rcsc::Ray2D ball_ray( first_ball_pos, ball_move_angle );
        rcsc::Vector2D sol1, sol2;
        int n_sol = pitch.intersection( ball_ray, &sol1, &sol2 );
        if ( n_sol == 1 )
        {
            first_min_step = SP.ballMoveStep( first_ball_vel.r(), first_ball_pos.dist( sol1 ) );
        }
    }
#endif

    int min_step = first_min_step;
    const rcsc::AbstractPlayerCont::const_iterator end = wm.allOpponents().end();
    for ( rcsc::AbstractPlayerCont::const_iterator o = wm.allOpponents().begin();
          o != end;
          ++o )
    {
        int step = predictOpponentReachStep( *o,
                                             first_ball_pos,
                                             first_ball_vel,
                                             ball_move_angle,
                                             min_step );
        if ( step < min_step )
        {
            min_step = step;
        }
    }

    return ( min_step == first_min_step
             ? 1000
             : min_step );
}


/*-------------------------------------------------------------------*/
/*!

 */
int
Bhv_Predictor::predictOpponentReachStep( const rcsc::AbstractPlayerObject * opponent,
                                           const rcsc::Vector2D & first_ball_pos,
                                           const rcsc::Vector2D & first_ball_vel,
                                           const rcsc::AngleDeg & ball_move_angle,
                                           const int max_cycle )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    const rcsc::PlayerType * ptype = opponent->playerTypePtr();
    const double opponent_speed = opponent->vel().r();

    int min_cycle = estimate_min_reach_cycle( opponent->pos(),
                                                             ptype->realSpeedMax(),
                                                             first_ball_pos,
                                                             ball_move_angle );
    if ( min_cycle < 0 )
    {
        min_cycle = 10;
    }

    for ( int cycle = min_cycle; cycle < max_cycle; ++cycle )
    {
        rcsc::Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                                  first_ball_vel,
                                                  cycle,
                                                  SP.ballDecay() );

        if ( ball_pos.absX() > SP.pitchHalfLength()
             || ball_pos.absY() > SP.pitchHalfWidth() )
        {
            return 1000;
        }

        rcsc::Vector2D inertia_pos = opponent->inertiaPoint( cycle );
        double target_dist = inertia_pos.dist( ball_pos );

        if ( target_dist - ptype->kickableArea() < 0.001 )
        {
            return cycle;
        }

        double dash_dist = target_dist;
        if ( cycle > 1 )
        {
            dash_dist -= ptype->kickableArea();
            dash_dist -= 0.5; // special bonus
        }

        if ( dash_dist > ptype->realSpeedMax() * cycle )
        {
            continue;
        }

        //
        // dash
        //

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( n_dash > cycle )
        {
            continue;
        }

        //
        // turn
        //
        int n_turn = ( opponent->bodyCount() > 1
                       ? 0
                       : predict_player_turn_cycle( ptype,
                                                                   opponent->body(),
                                                                   opponent_speed,
                                                                   target_dist,
                                                                   ( ball_pos - inertia_pos ).th(),
                                                                   ptype->kickableArea(),
                                                                   true ) );

        int n_step = ( n_turn == 0
                       ? n_turn + n_dash
                       : n_turn + n_dash + 1 ); // 1 step penalty for observation delay
        if ( opponent->isTackling() )
        {
            n_step += 5; // Magic Number
        }

        n_step -= std::min( 3, opponent->posCount() );

        if ( n_step <= cycle )
        {
            return cycle;
        }

    }

    return 1000;
}


bool
Bhv_Predictor::is_ball_moving_to_our_goal( const rcsc::Vector2D & ball_pos,
                                           const rcsc::Vector2D & ball_vel,
                                           const double & post_buffer )
{
    const double goal_half_width = rcsc::ServerParam::i().goalHalfWidth();
    const double goal_line_x = rcsc::ServerParam::i().ourTeamGoalLineX();
    const rcsc::Vector2D goal_plus_post( goal_line_x,
                                   +goal_half_width + post_buffer );
    const rcsc::Vector2D goal_minus_post( goal_line_x,
                                    -goal_half_width - post_buffer );
    const rcsc::AngleDeg ball_angle = ball_vel.th();

    return ( ( ( goal_plus_post - ball_pos ).th() - ball_angle ).degree() < 0
             && ( ( goal_minus_post - ball_pos ).th() - ball_angle ).degree() > 0 );
}


int
Bhv_Predictor::estimate_min_reach_cycle( const rcsc::Vector2D & player_pos,
                                         const double & player_speed_max,
                                         const rcsc::Vector2D & target_first_point,
                                         const rcsc::AngleDeg & target_move_angle )
{
    rcsc::Vector2D target_to_player = ( player_pos - target_first_point ).rotatedVector( -target_move_angle );
    return ( target_to_player.x < -1.0
             ? -1
             : std::max( 1, static_cast< int >( std::floor( target_to_player.absY() / player_speed_max ) ) ) );
}

int
Bhv_Predictor::predict_player_turn_cycle( const rcsc::PlayerType * ptype,
                                          const rcsc::AngleDeg & player_body,
                                          const double & player_speed,
                                          const double & target_dist,
                                          const rcsc::AngleDeg & target_angle,
                                          const double & dist_thr,
                                          const bool use_back_dash )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    int n_turn = 0;

    double angle_diff = ( target_angle - player_body ).abs();

    if ( use_back_dash
         && target_dist < 5.0 // Magic Number
         && angle_diff > 90.0
         && SP.minDashPower() < -SP.maxDashPower() + 1.0 )
    {
        angle_diff = std::fabs( angle_diff - 180.0 );    // assume backward dash
    }

    double turn_margin = 180.0;
    if ( dist_thr < target_dist )
    {
        turn_margin = std::max( 15.0, // Magic Number
                                rcsc::AngleDeg::asin_deg( dist_thr / target_dist ) );
    }

    double speed = player_speed;
    while ( angle_diff > turn_margin )
    {
        angle_diff -= ptype->effectiveTurn( SP.maxMoment(), speed );
        speed *= ptype->playerDecay();
        ++n_turn;
    }
    return n_turn;
}

int
Bhv_Predictor::predict_player_reach_cycle( const rcsc::AbstractPlayerObject * player,
                                    const rcsc::Vector2D & target_point,
                                    const double & dist_thr,
                                    const double & penalty_distance,
                                    const int body_count_thr,
                                    const int default_n_turn,
                                    const int wait_cycle,
                                    const bool use_back_dash )
{
	const rcsc::PlayerType * ptype = player->playerTypePtr();

    const rcsc::Vector2D & first_player_pos = ( player->seenPosCount() <= player->posCount()
                                          ? player->seenPos()
                                          : player->pos() );
    const rcsc::Vector2D & first_player_vel = ( player->seenVelCount() <= player->velCount()
                                          ? player->seenVel()
                                          : player->vel() );
    const double first_player_speed = first_player_vel.r() * std::pow( ptype->playerDecay(), wait_cycle );

    int final_reach_cycle = -1;
    {
        rcsc::Vector2D inertia_pos = ptype->inertiaFinalPoint( first_player_pos, first_player_vel );
        double target_dist = inertia_pos.dist( target_point );

        int n_turn = ( player->bodyCount() > body_count_thr
                       ? default_n_turn
                       : predict_player_turn_cycle( ptype,
                                                    player->body(),
                                                    first_player_speed,
                                                    target_dist,
                                                    ( target_point - inertia_pos ).th(),
                                                    dist_thr,
                                                    use_back_dash ) );
        int n_dash = ptype->cyclesToReachDistance( target_dist + penalty_distance );

        final_reach_cycle = wait_cycle + n_turn + n_dash;
    }

    const int max_cycle = 6; // Magic Number

    if ( final_reach_cycle > max_cycle )
    {
        return final_reach_cycle;
    }

    for ( int cycle = std::max( 0, wait_cycle ); cycle <= max_cycle; ++cycle )
    {
        rcsc::Vector2D inertia_pos = ptype->inertiaPoint( first_player_pos, first_player_vel, cycle );
        double target_dist = inertia_pos.dist( target_point ) + penalty_distance;

        if ( target_dist < dist_thr )
        {
            return cycle;
        }

        double dash_dist = target_dist - dist_thr * 0.5;

        if ( dash_dist < 0.001 )
        {
            return cycle;
        }

        if ( dash_dist > ptype->realSpeedMax() * ( cycle - wait_cycle ) )
        {
            continue;
        }

        int n_dash = ptype->cyclesToReachDistance( dash_dist );

        if ( wait_cycle + n_dash > cycle )
        {
            continue;
        }

        int n_turn = ( player->bodyCount() > body_count_thr
                       ? default_n_turn
                       : predict_player_turn_cycle( ptype,
                                                    player->body(),
                                                    first_player_speed,
                                                    target_dist,
                                                    ( target_point - inertia_pos ).th(),
                                                    dist_thr,
                                                    use_back_dash ) );

        if ( wait_cycle + n_turn + n_dash <= cycle )
        {
            return cycle;
        }

    }

    return final_reach_cycle;
}

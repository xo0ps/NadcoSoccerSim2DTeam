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

#include "shoot_table.h"

#include <rcsc/action/kick_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootTable::search( const rcsc::PlayerAgent * agent )
{
    static rcsc::GameTime s_time( 0, 0 );

    /////////////////////////////////////////////////////////////////////
    const rcsc::WorldModel & wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    if ( s_time == wm.time() )
    {
        return;
    }

    s_time = wm.time();
    M_total_count = 0;
    M_shots.clear();

    static const rcsc::Vector2D goal_c( sp.pitchHalfLength(), 0.0 );

    if ( ! wm.self().isKickable() )
    {
        return;
    }

    if ( wm.self().pos().dist2( goal_c ) > std::pow( 30.0, 2 ) )
    {
        return;
    }

    rcsc::Vector2D goal_l( sp.pitchHalfLength(),
					-sp.goalHalfWidth() );
    rcsc::Vector2D goal_r( sp.pitchHalfLength(),
                     sp.goalHalfWidth() );

    goal_l.y += std::min( 1.5,
                          0.6 + goal_l.dist( wm.ball().pos() ) * 0.042 );
    goal_r.y -= std::min( 1.5,
                          0.6 + goal_r.dist( wm.ball().pos() ) * 0.042 );

    if ( wm.self().pos().x > sp.pitchHalfLength() - 1.0
         && wm.self().pos().absY() < sp.goalHalfWidth() )
    {
        goal_l.x = wm.self().pos().x + 1.5;
        goal_r.x = wm.self().pos().x + 1.5;
    }

    const int DIST_DIVS = 25;
    const double dist_step = std::fabs( goal_l.y - goal_r.y ) / ( DIST_DIVS - 1 );

    const rcsc::PlayerObject * goalie = agent->world().getOpponentGoalie();

    rcsc::Vector2D shot_point = goal_l;

    for ( int i = 0;
          i < DIST_DIVS;
          ++i, shot_point.y += dist_step )
    {
        ++M_total_count;
        calculateShotPoint( wm, shot_point, goalie );
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
ShootTable::calculateShotPoint( const rcsc::WorldModel & wm,
                                    const rcsc::Vector2D & shot_point,
                                    const rcsc::PlayerObject * goalie )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    rcsc::Vector2D shot_rel = shot_point - wm.ball().pos();
    rcsc::AngleDeg shot_angle = shot_rel.th();

    int goalie_count = 1000;
    if ( goalie )
    {
        goalie_count = goalie->posCount();
    }

    if ( 5 < goalie_count
         && goalie_count < 30
         && wm.dirCount( shot_angle ) > 3 )
    {
        return;
    }

    double shot_dist = shot_rel.r();

    rcsc::Vector2D one_step_vel
        = rcsc::KickTable::calc_max_velocity( shot_angle,
                                        wm.self().kickRate(),
                                        wm.ball().vel() );
    double max_one_step_speed = one_step_vel.r();

    double shot_first_speed
        = ( shot_dist + 5.0 ) * ( 1.0 - sp.ballDecay() );
    shot_first_speed = std::max( max_one_step_speed, shot_first_speed );
    shot_first_speed = std::max( 1.5, shot_first_speed );

    // gaussian function, distribution = goal half width
    //double y_rate = std::exp( - std::pow( shot_point.y, 2.0 )
    //                          / ( 2.0 * sp.goalHalfWidth() * 3.0 ) );
    double y_dist = std::max( 0.0, shot_point.absY() - 4.0 );
    double y_rate = std::exp( - std::pow( y_dist, 2.0 )
                              / ( 2.0 * sp.goalHalfWidth() ) );

    bool over_max = false;
    while ( ! over_max )
    {
        if ( shot_first_speed > sp.ballSpeedMax() - 0.001 )
        {
            over_max = true;
            shot_first_speed = sp.ballSpeedMax();
        }

        Shot shot( shot_point, shot_first_speed, shot_angle );
        shot.score_ = 0;

        bool one_step = ( shot_first_speed <= max_one_step_speed );
        if ( canScore( wm, one_step, &shot ) )
        {
            shot.score_ += 100;
            if ( one_step )
            {   // one step kick
                shot.score_ += 100;
            }

            double goalie_rate = -1.0;
            if ( shot.goalie_never_reach_ )
            {
                shot.score_ += 100;
            }

            if ( goalie )
            {
                rcsc::AngleDeg goalie_angle = ( goalie->pos() - wm.ball().pos() ).th();
                double angle_diff = ( shot.angle_ - goalie_angle ).abs();
                goalie_rate = 1.0 - std::exp( - std::pow( angle_diff * 0.1, 2 )
                                              // / ( 2.0 * 90.0 * 0.1 ) );
                                              // / ( 2.0 * 40.0 * 0.1 ) ); // 2009-07
                                              / ( 2.0 * 90.0 * 0.1 ) ); // 2009-12-13
                shot.score_ = static_cast< int >( shot.score_ * goalie_rate );
            }

            shot.score_ = static_cast< int >( shot.score_ * y_rate );
            M_shots.push_back( shot );
        }
        else
        {
        }

        shot_first_speed += 0.5;
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ShootTable::canScore( const rcsc::WorldModel & wm,
                          const bool one_step_kick,
                          Shot * shot )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    static const double opp_x_thr = sp.theirPenaltyAreaLineX() - 5.0;
    static const double opp_y_thr = sp.penaltyAreaHalfWidth();

    // estimate required ball travel step
    const double ball_reach_step
        = rcsc::calc_length_geom_series( shot->speed_,
                                   wm.ball().pos().dist( shot->point_ ),
                                   sp.ballDecay() );

    if ( ball_reach_step < 1.0 )
    {
        shot->score_ += 100;
        return true;
    }

    const int ball_reach_step_i = static_cast< int >( std::ceil( ball_reach_step ) );

    // estimate opponent interception

    const rcsc::PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin();
          it != end;
          ++it )
    {
        // outside of penalty
        if( ! (*it) ) continue;
        if ( (*it)->pos().x < opp_x_thr ) continue;
        if ( (*it)->pos().absY() > opp_y_thr ) continue;
        if ( (*it)->isTackling() ) continue;

        // behind of shoot course
        if ( ( shot->angle_ - (*it)->angleFromSelf() ).abs() > 90.0 )
        {
            continue;
        }

        if ( (*it)->goalie() )
        {
            if ( maybeGoalieCatch( wm, *it, shot ) )
            {
                return false;
            }
        }
        else
        {
            if ( (*it)->posCount() > 10
                 || ( (*it)->isGhost() && (*it)->posCount() > 5 ) )
            {
                continue;
            }

            int cycle = predictOpponentReachStep( wm,
                                                  shot->point_,
                                                  *it,
                                                  wm.ball().pos(),
                                                  shot->vel_,
                                                  one_step_kick,
                                                  ball_reach_step_i );
            if ( cycle == 1
                 || cycle < ball_reach_step_i - 1 )
            {
                return false;
            }
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
ShootTable::maybeGoalieCatch( const rcsc::WorldModel & wm,
                                  const rcsc::PlayerObject * goalie,
                                  Shot * shot )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    static const
        rcsc::Rect2D penalty_area( rcsc::Vector2D( sp.theirPenaltyAreaLineX(), // left
                                       -sp.penaltyAreaHalfWidth() ), // top
                             rcsc::Size2D( sp.penaltyAreaLength(), // length
                                     sp.penaltyAreaWidth() ) ); // width
    static const double catchable_area = sp.catchableArea();

    const rcsc::ServerParam & param = rcsc::ServerParam::i();

    const double dash_accel_mag = ( param.maxDashPower()
                                    * param.defaultDashPowerRate()
                                    * param.defaultEffortMax() );
    const double seen_dist_noise = goalie->distFromSelf() * 0.05;

    int min_cycle = 1;
    {
        rcsc::Line2D shot_line( wm.ball().pos(), shot->point_ );
        double goalie_line_dist = shot_line.dist( goalie->pos() );
        goalie_line_dist -= catchable_area;
        goalie_line_dist -= seen_dist_noise;
        min_cycle = static_cast< int >
            ( std::ceil( goalie_line_dist / param.defaultRealSpeedMax() ) ) ;
        min_cycle -= std::min( 5, goalie->posCount() );
        min_cycle = std::max( 1, min_cycle );
    }
    rcsc::Vector2D ball_pos = inertia_n_step_point( wm.ball().pos(),
                                              shot->vel_,
                                              min_cycle,
                                              param.ballDecay() );
    rcsc::Vector2D ball_vel = ( shot->vel_
                          * std::pow( param.ballDecay(), min_cycle ) );


    int cycle = min_cycle;
    while ( ball_pos.x < param.pitchHalfLength() + 0.085
            && cycle <= 50 )
    {
        // estimate the required turn angle
        rcsc::Vector2D goalie_pos = goalie->inertiaPoint( cycle );
        rcsc::Vector2D ball_relative = ball_pos - goalie_pos;
        double ball_dist = ball_relative.r() - seen_dist_noise;

        if ( ball_dist < catchable_area )
        {
            return true;
        }

        //if ( ball_dist < catchable_area + 1.75 ) // 2009-07-02: 1.0 -> 1.75
        if ( ball_dist < catchable_area + 1.2 ) // 2009-12-13
        {
            shot->goalie_never_reach_ = false;
        }

        rcsc::AngleDeg ball_angle = ball_relative.th();
        rcsc::AngleDeg goalie_body = ( goalie->bodyCount() <= 5
                                 ? goalie->body()
                                 : ball_angle );

        int n_turn = 0;
        double angle_diff = ( ball_angle - goalie_body ).abs();
        if ( angle_diff > 90.0 )
        {
            angle_diff = 180.0 - angle_diff; // back dash
            goalie_body -= 180.0;
        }

        double turn_margin
            = std::max( rcsc::AngleDeg::asin_deg( catchable_area / ball_dist ),
                        15.0 );

        rcsc::Vector2D goalie_vel = goalie->vel();

        while ( angle_diff > turn_margin )
        {
            double max_turn
                = rcsc::effective_turn( 180.0,
                                  goalie_vel.r(),
                                  param.defaultInertiaMoment() );
            angle_diff -= max_turn;
            goalie_vel *= param.defaultPlayerDecay();
            ++n_turn;
        }

        // simulate dash
        goalie_pos = goalie->inertiaPoint( n_turn );

        const rcsc::Vector2D dash_accel = rcsc::Vector2D::polar2vector( dash_accel_mag,
                                                            ball_angle );
        const int max_dash = ( cycle - 1 - n_turn
                               + rcsc::bound( 0, goalie->posCount() - 1, 5 ) );
        double goalie_travel = 0.0;
        for ( int i = 0; i < max_dash; ++i )
        {
            goalie_vel += dash_accel;
            goalie_pos += goalie_vel;
            goalie_travel += goalie_vel.r();
            goalie_vel *= param.defaultPlayerDecay();

            double d = goalie_pos.dist( ball_pos ) - seen_dist_noise;
            if ( d < catchable_area + 1.0 + ( goalie_travel * 0.04 ) )
            {
                shot->goalie_never_reach_ = false;
            }
        }

        // check distance
        if ( goalie->pos().dist( goalie_pos ) * 1.05
             > goalie->pos().dist( ball_pos )
             - seen_dist_noise
             - catchable_area )
        {
            return true;
        }

        // update ball position & velocity
        ++cycle;
        ball_pos += ball_vel;
        ball_vel *= param.ballDecay();
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
int
ShootTable::predictOpponentReachStep( const rcsc::WorldModel &,
                                          const rcsc::Vector2D & target_point,
                                          const rcsc::PlayerObject * opponent,
                                          const rcsc::Vector2D & first_ball_pos,
                                          const rcsc::Vector2D & first_ball_vel,
                                          const bool one_step_kick,
                                          const int max_step )
{
    const rcsc::ServerParam & param = rcsc::ServerParam::i();
    const rcsc::PlayerType * player_type = opponent->playerTypePtr();
    const double control_area = player_type->kickableArea();

    int min_cycle = 1;
    {
        rcsc::Line2D shot_line( first_ball_pos, target_point );
        double line_dist = shot_line.dist( opponent->pos() );
        line_dist -= control_area;
        min_cycle = static_cast< int >
            ( std::ceil( line_dist / player_type->realSpeedMax() ) ) ;
        min_cycle -= std::min( 5, opponent->posCount() );
        min_cycle = std::max( 1, min_cycle );
    }

    rcsc::Vector2D ball_pos = inertia_n_step_point( first_ball_pos,
                                              first_ball_vel,
                                              min_cycle,
                                              param.ballDecay() );
    rcsc::Vector2D ball_vel = first_ball_vel * std::pow( param.ballDecay(), min_cycle );

    int cycle = min_cycle;

    while ( cycle <= max_step )
    {
        rcsc::Vector2D opp_pos = opponent->inertiaPoint( cycle );
        rcsc::Vector2D opp_to_ball = ball_pos - opp_pos;
        double opp_to_ball_dist = opp_to_ball.r();

        int n_turn = 0;
        if ( opponent->bodyCount() <= 1
             || opponent->velCount() <= 1 )
        {
            double angle_diff =  ( opponent->bodyCount() <= 1
                                  ? ( opp_to_ball.th() - opponent->body() ).abs()
                                  : ( opp_to_ball.th() - opponent->vel().th() ).abs() );

            double turn_margin = 180.0;
            if ( control_area < opp_to_ball_dist )
            {
                turn_margin = rcsc::AngleDeg::asin_deg( control_area / opp_to_ball_dist );
            }
            turn_margin = std::max( turn_margin, 12.0 );

            double opp_speed = opponent->vel().r();
            while ( angle_diff > turn_margin )
            {
                angle_diff -= player_type->effectiveTurn( param.maxMoment(), opp_speed );
                opp_speed *= player_type->playerDecay();
                ++n_turn;
            }
        }

        opp_to_ball_dist -= control_area;
        opp_to_ball_dist -= opponent->distFromSelf() * 0.03;

        if ( opp_to_ball_dist < 0.0 )
        {
            return cycle;
        }

        int n_step = player_type->cyclesToReachDistance( opp_to_ball_dist );
        n_step += n_turn;
        //n_step -= bound( 0, opponent->posCount() - 1, 2 );
        n_step -= rcsc::bound( 0, opponent->posCount(), 2 );

        if ( n_step < cycle - ( one_step_kick ? 1 : 0 ) )
        {
            return cycle;
        }

        // update ball position & velocity
        ++cycle;
        ball_pos += ball_vel;
        ball_vel *= param.ballDecay();
    }

    return cycle;
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

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

#include "body_kick_two_step.h"
#include "body_kick_one_step.h"

#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_kick_to_relative.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

const double Body_KickTwoStep::DEFAULT_MIN_DIST2 = 10000.0;

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickTwoStep::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    if ( ! wm.self().isKickable() )
    {
        return false;
    }

    //---------------------------------------------------

    rcsc::Vector2D target_rpos = M_target_point - wm.self().pos();
    M_first_speed = std::min( M_first_speed, sp.ballSpeedMax() );

    rcsc::Vector2D achieved_vel;

    if ( simulate_one_kick( &achieved_vel,
                            NULL,
                            NULL,
                            target_rpos,
                            M_first_speed,
                            rcsc::Vector2D( 0.0, 0.0 ), // my current pos
                            wm.self().vel(),
                            wm.self().body(),
                            wm.ball().rpos(),
                            wm.ball().vel(),
                            agent,
                            false ) ) // not enforce
    {
        rcsc::Vector2D accel = achieved_vel - wm.ball().vel();
        double kick_power = accel.r() / wm.self().kickRate();
        rcsc::AngleDeg kick_dir = accel.th() - wm.self().body();

        M_ball_result_pos = wm.ball().pos() + achieved_vel;
        M_ball_result_vel = achieved_vel * sp.ballDecay();;
        M_kick_step = 1;

		//std::cout<<"two kick"<<std::endl;
        return agent->doKick( kick_power, kick_dir );
    }


    rcsc::Vector2D next_vel;
    if ( simulate_two_kick( &achieved_vel,
                            &next_vel,
                            target_rpos,
                            M_first_speed,
                            rcsc::Vector2D( 0.0, 0.0 ), // my current pos
                            wm.self().vel(),
                            wm.self().body(),
                            wm.ball().rpos(),
                            wm.ball().vel(),
                            agent,
                            M_enforce_kick ) )
    {
        M_kick_step = 2;

        if ( M_enforce_kick
             && next_vel.r2() < rcsc::square( M_first_speed ) )
        {
            rcsc::Vector2D one_kick_max_vel
                = Body_KickOneStep::get_max_possible_vel( ( target_rpos - wm.ball().rpos() ).th(),
                                                          wm.self().kickRate(),
                                                          wm.ball().vel() );
            if ( one_kick_max_vel.r2() > next_vel.r2() )
            {
                next_vel = one_kick_max_vel;
                M_kick_step = 1;
            }

            if ( next_vel.r() < M_first_speed * 0.8 )
            {
                M_kick_step = 0;

                return rcsc::Body_HoldBall( true, // turn to target
                                      M_target_point,
                                      ( wm.gameMode().type() == rcsc::GameMode::PlayOn
                                        ? M_target_point
                                        : rcsc::Vector2D::INVALIDATED )
                                      ).execute( agent );
            }
        }

        rcsc::Vector2D accel = next_vel - wm.ball().vel();
        double kick_power = accel.r() / wm.self().kickRate();
        rcsc::AngleDeg kick_dir = accel.th() - wm.self().body();

        M_ball_result_pos = wm.ball().pos() + next_vel;
        M_ball_result_vel = next_vel * sp.ballDecay();

		//std::cout<<"two kick"<<std::endl;
        return agent->doKick( kick_power, kick_dir );
    }

    return rcsc::Body_HoldBall( true, // turn to target
                          M_target_point,
                          ( wm.gameMode().type() == rcsc::GameMode::PlayOn
                            ? M_target_point  // keep reverse side
                            : rcsc::Vector2D::INVALIDATED )
                          ).execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickTwoStep::is_opp_kickable( const rcsc::PlayerAgent * agent,
                                   const rcsc::Vector2D & rel_pos,
                                   double * min_dist2 )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    // use default value
    static const double KICKABLE2
        = rcsc::square( sp.defaultKickableArea() + 0.17 );

    const rcsc::PlayerPtrCont::const_iterator end = agent->world().opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = agent->world().opponentsFromSelf().begin();
          it != end;
          ++it )
    {
		if( !(*it) ) continue;
        if ( (*it)->distFromSelf() > 6.0 )
        {
            break;
        }

        if ( (*it)->posCount() >= 2 )
        {
            continue;
        }

        double d2 = rel_pos.dist2( (*it)->rpos() + (*it)->vel() );
        if ( min_dist2 && d2 < *min_dist2 )
        {
            *min_dist2 = d2;
        }

        if ( d2 < KICKABLE2 )
        {
            return true;
        }
        // check opponent goalie
        else if ( (*it)->goalie()
                  && (*it)->pos().x > sp.theirPenaltyAreaLineX() + 1.0
                  && ( (*it)->pos().absY()
                       < sp.penaltyAreaHalfWidth() - 1.0 )
                  && ( rel_pos.dist( (*it)->rpos() + (*it)->vel() )
                       < sp.catchAreaLength() )
                  )
        {
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickTwoStep::simulate_one_kick( rcsc::Vector2D * achieved_vel,
                                     double * kick_power,
                                     double * opp_dist2,
                                     const rcsc::Vector2D & target_rpos,
                                     const double & first_speed,
                                     const rcsc::Vector2D & my_rpos,
                                     const rcsc::Vector2D & my_vel,
                                     const rcsc::AngleDeg & my_body,
                                     const rcsc::Vector2D & ball_rpos,
                                     const rcsc::Vector2D & ball_vel,
                                     const rcsc::PlayerAgent * agent,
                                     const bool enforce )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    rcsc::Vector2D ball_rel_at_here = ball_rpos - my_rpos;
    const double krate
        = rcsc::kick_rate( ball_rel_at_here.r(),
                     ( ball_rel_at_here.th() - my_body ).degree(),
                     sp.kickPowerRate(),
                     sp.ballSize(),
                     agent->world().self().playerType().playerSize(),
                     agent->world().self().playerType().kickableMargin() );
    const double max_accel = std::min( sp.maxPower() * krate,
                                       sp.ballAccelMax() );

    rcsc::Vector2D required_vel
        = rcsc::Vector2D::polar2vector( first_speed, ( target_rpos - ball_rpos ).th() );
    double required_accel = ( required_vel - ball_vel ).r();
    double required_power = required_accel / krate;

    // can NOT reach the target vel
    if ( required_accel > max_accel )
    {
        if ( ! enforce )
        {
            return false;
        }
        required_vel
            = Body_KickOneStep::get_max_possible_vel( (target_rpos - ball_rpos).th(),
                                                      krate,
                                                      ball_vel );
    }

    // check collision & opponents
    double tmp_opp_dist2 = DEFAULT_MIN_DIST2;
    rcsc::Vector2D next_ball_rpos = ball_rpos + required_vel;
    if ( next_ball_rpos.dist2( my_rpos + my_vel )
         < rcsc::square( agent->world().self().playerType().playerSize()
                   + sp.ballSize()
                   + 0.1 )
         || is_opp_kickable( agent, next_ball_rpos, &tmp_opp_dist2 ) )
    {
        return false;
    }


    if ( achieved_vel )
    {
        *achieved_vel = required_vel;
    }

    if ( kick_power )
    {
        *kick_power = required_power;
    }

    if ( opp_dist2 )
    {
        *opp_dist2 = tmp_opp_dist2;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickTwoStep::simulate_two_kick( rcsc::Vector2D * achieved_vel,
                                     rcsc::Vector2D * next_vel,
                                     const rcsc::Vector2D & target_rpos,
                                     const double & first_speed,
                                     const rcsc::Vector2D & my_rpos,
                                     const rcsc::Vector2D & my_vel,
                                     const rcsc::AngleDeg & my_body,
                                     const rcsc::Vector2D & ball_rpos,
                                     const rcsc::Vector2D & ball_vel,
                                     const rcsc::PlayerAgent * agent,
                                     const bool enforce )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    rcsc::Vector2D ball_rel_at_here = ball_rpos - my_rpos;
    const double krate
        = rcsc::kick_rate( ball_rel_at_here.r(),
                     ( ball_rel_at_here.th() - my_body ).degree(),
                     sp.kickPowerRate(),
                     sp.ballSize(),
                     agent->world().self().playerType().playerSize(),
                     agent->world().self().playerType().kickableMargin() );
    const double max_accel
        = std::min( sp.maxPower() * krate,
                    sp.ballAccelMax() );

    const rcsc::Vector2D my_next = my_rpos + my_vel;
    const double my_kickable = sp.defaultKickableArea();

    // next ball position & vel
    //std::vector< std::pair< Vector2D, Vector2D > > subtargets;
    std::vector< SubTarget > subtargets;

    //////////////////////////////////////////////////////////////////
    // first element is next kickable edge
    {
        const rcsc::Ray2D desired_ray( ball_rpos, ( target_rpos - ball_rpos ).th() );
        const rcsc::Circle2D next_kickable_circle( my_next, my_kickable - 0.1 );

        // get intersection next kickable circle & desired ray
        // solutions are next ball pos, relative to current my pos
        rcsc::Vector2D sol1, sol2;
        int num = next_kickable_circle.intersection( desired_ray, &sol1, &sol2 );
        rcsc::Vector2D required_vel( rcsc::Vector2D::INVALIDATED );

        if ( num == 1 )
        {
            // if ball point is not within next kicable area.
            // it is very dangerous to kick to the kickable edge.
            if ( next_kickable_circle.contains( ball_rpos ) )
            {
                required_vel = sol1 - ball_rpos;
            }
        }
        else if ( num == 2 )
        {
            // current ball point is NOT within next kicable area.
            rcsc::Vector2D v1 = sol1 - ball_rpos;
            rcsc::Vector2D v2 = sol2 - ball_rpos;
            // set bigger one
            required_vel = ( v1.r2() > v2.r2() ? v1 : v2 );
        }

        if ( required_vel.isValid() )
        {
            double d = required_vel.r();
            double ball_noise = d * sp.ballRand() * 1.412;
            double self_noise = my_vel.r() * sp.playerRand();
            d = std::max( d - ball_noise - self_noise - 0.15, 0.0 );
            required_vel.setLength( d );
        }

        double opp_dist2 = DEFAULT_MIN_DIST2;
        if ( required_vel.isValid()
             && ! is_opp_kickable( agent, ball_rpos + required_vel, &opp_dist2 )
             && ( required_vel - ball_vel ).r() < max_accel )
        {
            double d = required_vel.r();
            if ( d > sp.ballSpeedMax() )
            {
                required_vel *= sp.ballSpeedMax() / d;
            }

            // add element
            subtargets.push_back( SubTarget( ball_rpos + required_vel,
                                             required_vel * sp.ballDecay(),
                                             opp_dist2 ) );
        }
    }

    //////////////////////////////////////////////////////////////////
    // generate other subtargets
    {
        const double subtarget_dist = std::max( my_kickable * 0.7,
                                                my_kickable - 0.35 );
        const double default_dir_inc = 30.0;

        const rcsc::AngleDeg angle_self_to_target = ( target_rpos - my_next ).th();
        const double ball_target_dir_diff
            = ( angle_self_to_target - ( ball_rpos - my_next ).th() ).abs();

        // sub-targets should be more forward than ball
        double inc = ball_target_dir_diff / 5.0;
        inc = std::max( inc, default_dir_inc );
        inc = std::min( inc, ball_target_dir_diff );
        for ( double d = -ball_target_dir_diff;
              d <= ball_target_dir_diff + 1.0;
              d += inc )
        {

            rcsc::Vector2D sub // rel to current my pos
                = my_next
                + rcsc::Vector2D::polar2vector( subtarget_dist,
                                          angle_self_to_target + d );

            double opp_dist2 = DEFAULT_MIN_DIST2;
            rcsc::Vector2D require_vel = sub - ball_rpos;
            if ( ! is_opp_kickable( agent, sub, &opp_dist2 )
                 && (require_vel - ball_vel).r() < max_accel )
            {
                subtargets.push_back( SubTarget( sub,
                                                 require_vel * sp.ballDecay(),
                                                 opp_dist2 ) );
            }
        }
    }

    //////////////////////////////////////////////////////////////////
    // subtargets are next ball position
    rcsc::Vector2D max_achieved_vel( 0.0, 0.0 );
    rcsc::Vector2D sol_next_ball_vel( 100.0, 100.0 );
    bool found = false;
    {
        double min_kick_power = sp.maxPower() + 0.1;
        double min_opp_dist2 = 0.0;

        const rcsc::Vector2D my_next_vel
            = my_vel * agent->world().self().playerType().playerDecay();

        const std::vector< SubTarget >::const_iterator end = subtargets.end();
        for ( std::vector< SubTarget >::const_iterator it = subtargets.begin();
              it != end;
              ++it )
        {
            rcsc::Vector2D sol_vel;
            double kick_power = 1000000.0;
            double opp_dist2 = DEFAULT_MIN_DIST2;
            if ( simulate_one_kick( &sol_vel,
                                    &kick_power,
                                    &opp_dist2,
                                    target_rpos,
                                    first_speed,
                                    my_next,
                                    my_next_vel,
                                    my_body,
                                    it->ball_pos_, // next ball pos
                                    it->ball_vel_, // next ball vel
                                    agent,
                                    enforce )
                 )
            {
                bool update = true;
                if ( enforce
                     && sol_vel.r2() < max_achieved_vel.r2() )
                {
                    update = false;
                }

                if ( update )
                {
                    if ( it->opp_dist2_ != DEFAULT_MIN_DIST2
                         || min_opp_dist2 != 0.0 )
                    {
                        if ( it->opp_dist2_ == min_opp_dist2 )
                        {
                            if ( kick_power > min_kick_power )
                            {
                                update = false;
                            }
                        }
                        else if ( it->opp_dist2_ < min_opp_dist2 )
                        {
                            update = false;
                        }
                    }
                    else
                    {
                        if ( kick_power > min_kick_power )
                        {
                            update = false;
                        }
                    }
                }

                if ( update )
                {
                    found = true;
                    max_achieved_vel = sol_vel;
                    min_kick_power = kick_power;
                    min_opp_dist2 = it->opp_dist2_;
                    sol_next_ball_vel = it->ball_vel_ / sp.ballDecay();
                  
                }
            }
        }

    }


    if ( ! found )
    {
        return false;
    }

    if ( achieved_vel )
    {
        *achieved_vel = max_achieved_vel;
    }

    if ( next_vel )
    {
        *next_vel = sol_next_ball_vel;
    }
    return true;
}

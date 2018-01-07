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

#include "body_kick_multi_step.h"
#include "body_kick_one_step.h"
#include "body_kick_two_step.h"

#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_hold_ball.h>

#include <rcsc/player/player_agent.h>

#include <rcsc/common/server_param.h>
#include <rcsc/soccer_math.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_KickMultiStep::execute( rcsc::PlayerAgent * agent )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    if ( ! agent->world().self().isKickable() )
    {
        return false;
    }

    //-------------------------------------------
    rcsc::Vector2D target_rpos = M_target_point - agent->world().self().pos();
    M_first_speed = std::min( M_first_speed, sp.ballSpeedMax() );

    rcsc::Vector2D achieved_vel;

    if ( Body_KickTwoStep::simulate_one_kick( &achieved_vel,
                                              NULL,
                                              NULL,
                                              target_rpos,
                                              M_first_speed,
                                              rcsc::Vector2D( 0.0, 0.0 ), // my current pos
                                              agent->world().self().vel(),
                                              agent->world().self().body(),
                                              agent->world().ball().rpos(),
                                              agent->world().ball().vel(),
                                              agent,
                                              false ) ) // not enforce
    {
        rcsc::Vector2D accel = achieved_vel - agent->world().ball().vel();
        double kick_power = accel.r() / agent->world().self().kickRate();
        rcsc::AngleDeg kick_dir = accel.th() - agent->world().self().body();

        M_ball_result_pos = agent->world().ball().pos() + achieved_vel;
        M_ball_result_vel = achieved_vel * sp.ballDecay();;
        M_kick_step = 1;
        
		//std::cout<<"multi kick"<<std::endl;
        return agent->doKick( kick_power, kick_dir );
    }

    rcsc::Vector2D next_vel;

    if ( Body_KickTwoStep::simulate_two_kick( &achieved_vel,
                                              &next_vel,
                                              target_rpos,
                                              M_first_speed,
                                              rcsc::Vector2D( 0.0, 0.0 ), // my current pos
                                              agent->world().self().vel(),
                                              agent->world().self().body(),
                                              agent->world().ball().rpos(),
                                              agent->world().ball().vel(),
                                              agent,
                                              false ) ) // not enforced
    {
        rcsc::Vector2D accel = next_vel - agent->world().ball().vel();
        double kick_power = accel.r() / agent->world().self().kickRate();
        rcsc::AngleDeg kick_dir = accel.th() - agent->world().self().body();

        M_ball_result_pos = agent->world().ball().pos() + next_vel;
        M_ball_result_vel = next_vel * sp.ballDecay();;
        M_kick_step = 2;
        
		//std::cout<<"multi kick"<<std::endl;
        return agent->doKick( kick_power, kick_dir );
    }

    if ( simulate_three_kick( &achieved_vel,
                              &next_vel,
                              target_rpos,
                              M_first_speed,
                              rcsc::Vector2D( 0.0, 0.0 ), // my current pos
                              agent->world().self().vel(),
                              agent->world().self().body(),
                              agent->world().ball().rpos(),
                              agent->world().ball().vel(),
                              agent,
                              false ) )
    {
        rcsc::Vector2D accel = next_vel - agent->world().ball().vel();
        double kick_power = accel.r() / agent->world().self().kickRate();
        rcsc::AngleDeg kick_dir = accel.th() - agent->world().self().body();

        M_ball_result_pos = agent->world().ball().pos() + next_vel;
        M_ball_result_vel = next_vel * sp.ballDecay();;
        M_kick_step = 3;

		//std::cout<<"multi kick"<<std::endl;
        return agent->doKick( kick_power, kick_dir );
    }


    if ( M_enforce_kick )
    {
        Body_KickTwoStep kick( M_target_point,
                               M_first_speed,
                               true // enforce
                               );
        bool result = kick.execute( agent );
        M_ball_result_pos = kick.ballResultPos();
        M_ball_result_vel = kick.ballResultVel();
        M_kick_step = kick.kickStep();
        
		//std::cout<<"multi kick"<<std::endl;
        return result;
    }

    return rcsc::Body_HoldBall( true, // turn to target
                          M_target_point,
                          ( agent->world().gameMode().type() == rcsc::GameMode::PlayOn
                            ? M_target_point  // keep reverse side
                            : rcsc::Vector2D::INVALIDATED )
                          ).execute( agent );
}

/*-------------------------------------------------------------------*/
/*!
  static method
  internal simulation like recursive
*/
bool
Body_KickMultiStep::simulate_three_kick( rcsc::Vector2D * achieved_vel,
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

    const double my_kickable = rcsc::ServerParam::i().defaultKickableArea();

    std::vector< std::pair< rcsc::Vector2D, rcsc::Vector2D > > subtargets; // next ball position & vel

    const rcsc::Vector2D my_next = my_rpos + my_vel;

    //////////////////////////////////////////////////////////////////
    // generate subtargets
    {
        // subtargets is generated in opposite side of target dir.
        const double max_accel2 = std::pow( std::min( sp.maxPower() * krate,
                                                      sp.ballAccelMax() ),
                                            2.0 );

        const rcsc::Vector2D my_next_next
            = my_next + my_vel * agent->world().self().playerType().playerDecay();

        const double subtarget_dist = my_kickable * 0.65;
        const double default_dir_inc =  30.0;
        const double default_add_max = 181.0 - default_dir_inc * 0.5;

        const rcsc::AngleDeg angle_self_to_target = (target_rpos - my_next_next).th();
        const rcsc::AngleDeg first_sub_target_angle
            = angle_self_to_target + 90.0 + default_dir_inc * 0.5;

        double add_dir = 0.0;
        while ( add_dir < default_add_max )
        {
            rcsc::Vector2D sub // rel to current my pos
                = my_next
                + rcsc::Vector2D::polar2vector(subtarget_dist,
                                         first_sub_target_angle + add_dir);
            rcsc::Vector2D require_vel = sub - ball_rpos;
            if ( ( require_vel - ball_vel ).r2() < max_accel2
                 && ! Body_KickTwoStep::is_opp_kickable( agent, sub, NULL ) )
            {
                subtargets.push_back
                    ( std::make_pair( sub,
                                      require_vel * sp.ballDecay() ) );
            }
            add_dir += default_dir_inc;
        }
    }


    //////////////////////////////////////////////////////////////////
    // subtargets are next ball position

    rcsc::Vector2D max_achieved_vel( 0.0, 0.0 );
    rcsc::Vector2D sol_next_ball_vel( 100.0, 100.0 ); // next ball vel to archive the final vel
    bool found = false;
    {
        const rcsc::Vector2D my_next_vel
            = my_vel * agent->world().self().playerType().playerDecay();

        rcsc::Vector2D sol_vel; // last reachable vel
        const std::vector< std::pair< rcsc::Vector2D, rcsc::Vector2D > >::const_iterator
            target_end = subtargets.end();

        for ( std::vector< std::pair< rcsc::Vector2D, rcsc::Vector2D > >::const_iterator
                  it = subtargets.begin();
              it != target_end;
              ++it )
        {
            if ( Body_KickTwoStep::simulate_two_kick( &sol_vel,
                                                      NULL,
                                                      target_rpos,
                                                      first_speed,
                                                      my_next,
                                                      my_next_vel,
                                                      my_body,
                                                      it->first, // next ball
                                                      it->second, // next ball vel
                                                      agent,
                                                      enforce )
                 )
            {
                rcsc::Vector2D bvel = it->second / sp.ballDecay();
                if ( ( enforce && sol_vel.r2() > max_achieved_vel.r2() )
                     || ( ! enforce // lower speed makes lower noise
                          && sol_vel.r2() >= max_achieved_vel.r2() - 0.0001
                          && sol_next_ball_vel.r2() > bvel.r2() ) )
                {
                    found = true;
                    max_achieved_vel = sol_vel;
                    sol_next_ball_vel = bvel;
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

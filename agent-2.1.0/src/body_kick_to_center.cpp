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

#include "body_kick_to_center.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "body_pass.h"
#include "body_kick_two_step.h"
#include "body_kick_multi_step.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

std::vector< Body_KickToCenter::Target >Body_KickToCenter::S_cached_target;

/*-------------------------------------------------------------------*/
bool
Body_KickToCenter::execute( rcsc::PlayerAgent * agent )
{
    int receiver_number = 0;
    rcsc::Vector2D target_point( 40.0, 0.0 );
    double first_speed = 2.0;

    int receiver_conf = 1000;

    if ( get_best_point( agent, &target_point ) )
    {
        std::vector< Target >::iterator max_it = std::max_element( S_cached_target.begin(), S_cached_target.end(),TargetCmp() );
        receiver_number = max_it->receiver->unum();
        receiver_conf = max_it->receiver->posCount();
        target_point = max_it->target_point;
        first_speed = max_it->first_speed;
    }
    else
    {
		return false;
    }


    if(! Body_SmartKick( target_point, first_speed, first_speed * 0.96, 3 ).execute( agent ) )
    {
		if( agent->world().existKickableOpponent() )
	    {
	        Body_KickOneStep( target_point , first_speed ).execute( agent );
	    }
	}
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	Body_Pass::say_pass( agent , receiver_number , target_point );
	std::cout<<agent->world().self().unum()<<" kick to center"<<std::endl;
	return true;
}

/*-------------------------------------------------------------------*/
bool
Body_KickToCenter::get_best_point( const rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point )
{
    search( agent );

    if( S_cached_target.empty() )
        return false;

    if( target_point )
    {
        std::vector< Target >::iterator max_it = std::max_element( S_cached_target.begin(), S_cached_target.end(), TargetCmp() );
        *target_point = max_it->target_point;
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
void
Body_KickToCenter::search( const rcsc::PlayerAgent * agent )
{
    static rcsc::GameTime s_last_calc_time( -1, 0 );

    if ( s_last_calc_time == agent->world().time() )
        return;

    s_last_calc_time = agent->world().time();

    const rcsc::Rect2D cross_area
        ( rcsc::Vector2D( rcsc::ServerParam::i().theirPenaltyAreaLineX() - 3.0,
                          - rcsc::ServerParam::i().penaltyAreaHalfWidth() + 5.0 ),
          rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() + 3.0,
                        rcsc::ServerParam::i().penaltyAreaWidth() - 10.0 ) );
    const double offside_x = agent->world().offsideLineX();
    S_cached_target.clear();
    const rcsc::PlayerPtrCont::const_iterator mates_end = agent->world().teammatesFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = agent->world().teammatesFromSelf().begin(); it != mates_end; ++it )
    {
		if ( !(*it) ) continue;
        if ( (*it)->posCount() > 5 ) continue;
        if ( (*it)->isTackling() ) continue;
        if ( (*it)->pos().x > offside_x + 1.0 ) continue;
        if ( (*it)->distFromSelf() > 30.0 ) break;
        if ( ! cross_area.contains( (*it)->pos() ) ) continue;
        if ( create_close( agent, *it ) ) break;
        create_target( agent, *it );
    }

}

/*-------------------------------------------------------------------*/
bool
Body_KickToCenter::create_close( const rcsc::PlayerAgent * agent, const rcsc::PlayerObject * receiver )
{
    if ( receiver->distFromSelf() < 2.0 || receiver->distFromSelf() > 14.0 )
        return false;

    if ( receiver->pos().x < 40.0 || receiver->pos().absY() > 15.0 )
        return false;

    if( receiver->pos().absY() > 7.0
	 && receiver->pos().y * agent->world().self().pos().y > 0.0
     && receiver->pos().absY() > agent->world().self().pos().absY() )
        return false;

    if ( agent->world().self().pos().x < 30.0 || agent->world().self().pos().absY() > 25.0 )
        return false;

    const rcsc::PlayerPtrCont & opps = agent->world().opponentsFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();

    rcsc::Vector2D target_point = receiver->pos();
    target_point.y += receiver->vel().y * 2.0;
    target_point.x += 1.8;

    for ( int i = 0; i < 3; i++ )
    {
        target_point.x -= 0.6;
        const rcsc::Line2D cross_line( agent->world().ball().pos(), target_point );
        const double target_dist = agent->world().ball().pos().dist( target_point );
        
        bool success = true;
        for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end; ++it )
        {
            if ( (*it)->posCount() > 5 ) continue;
            if ( (*it)->goalie() )
            {
                if ( (*it)->distFromBall() > target_dist + 3.0 )
                    continue;

                if ( cross_line.dist( (*it)->pos() + (*it)->vel() ) < rcsc::ServerParam::i().catchAreaLength() + 1.5 )
                //if( Body_Pass::exist_opponent3( agent , (*it)->pos() + (*it)->vel() ) )
                {
                    success = false;
                    break;
                }
            }
            else
            {
                if ( (*it)->distFromSelf() > target_dist + 2.0 )
                    continue;

                if ( cross_line.dist( (*it)->pos() + (*it)->vel() ) < rcsc::ServerParam::i().defaultKickableArea() + 0.5 )
                //if( Body_Pass::exist_opponent3( agent , (*it)->pos() + (*it)->vel() ) )
                {
                    success = false;
                    break;
                }
            }
        }

        if ( success )
        {
            const double dash_x = std::max( std::fabs( target_point.x - receiver->pos().x )
                                            - receiver->vel().x
                                            - receiver->playerTypePtr()->kickableArea() * 0.5,
                                            0.0 );
            const double dash_step = receiver->playerTypePtr()->cyclesToReachDistance( dash_x );
            const double target_dist = agent->world().ball().pos().dist( target_point );

            double end_speed = 1.8;
            double first_speed = 1000.0;
            double ball_steps_to_target = 1.0;
            do
            {
                first_speed = rcsc::calc_first_term_geom_series_last( end_speed, target_dist, rcsc::ServerParam::i().ballDecay() );
                if ( first_speed > rcsc::ServerParam::i().ballSpeedMax() )
                {
                    end_speed -= 0.1;
                    continue;
                }

                ball_steps_to_target = rcsc::calc_length_geom_series( first_speed, target_dist, rcsc::ServerParam::i().ballDecay() );
                if ( dash_step < ball_steps_to_target )
                    break;
                end_speed -= 0.05;
            }
            while ( end_speed > 1.5 );

            if ( first_speed > rcsc::ServerParam::i().ballSpeedMax() || dash_step > ball_steps_to_target )
                return false;
                
            S_cached_target.push_back( Target( receiver, target_point,first_speed, 1000.0 ) );
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
void
Body_KickToCenter::create_target( const rcsc::PlayerAgent * agent, const rcsc::PlayerObject * receiver )
{
    static const double blocker_dash_speed = 1.0;
    const rcsc::PlayerPtrCont & opps = agent->world().opponentsFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();

    const double y_diff2 = rcsc::square( receiver->pos().y - agent->world().ball().pos().y );
    const double max_x = std::min( receiver->pos().x + 4.0, rcsc::ServerParam::i().pitchHalfLength() - 2.0 );
    
    const rcsc::PlayerType * player_type = agent->world().teammatePlayerType( receiver->unum() );
    double dash_step_buf = 0.0;
	if ( receiver->velCount() <= 1 && receiver->vel().x > 0.3 )
        dash_step_buf = -1.0;
    else if ( receiver->bodyCount() <= 1 && receiver->body().abs() < 15.0 )
        dash_step_buf = 0.0;
    else
        dash_step_buf = 2.0;

    for ( double receive_x = std::max( receiver->pos().x - 2.0, rcsc::ServerParam::i().theirPenaltyAreaLineX() ); receive_x < max_x; receive_x += 0.5 )
    {
        double dash_x = std::fabs( receive_x - receiver->pos().x );
        const double dash_step = player_type->cyclesToReachDistance( dash_x - 0.5 );
        double target_dist = std::sqrt( std::pow( receive_x - agent->world().ball().pos().x , 2.0 ) + y_diff2 );

        double end_speed = 1.8;
        double first_speed = 100.0;
        double ball_steps_to_target = 100.0;

        while ( end_speed > 1.5 )
        {
            first_speed = rcsc::calc_first_term_geom_series_last( end_speed, target_dist, rcsc::ServerParam::i().ballDecay() );
            if ( first_speed > rcsc::ServerParam::i().ballSpeedMax() )
            {
                end_speed -= 0.1;
                continue;
            }

            ball_steps_to_target = rcsc::calc_length_geom_series( first_speed, target_dist , rcsc::ServerParam::i().ballDecay() );

            if ( ball_steps_to_target < dash_step + dash_step_buf )
            {
                end_speed -= 0.075;
                continue;
            }

            break;
        }

        if ( first_speed > rcsc::ServerParam::i().ballSpeedMax() || ball_steps_to_target < dash_step )
            continue;

        const rcsc::Vector2D target_point( receive_x, receiver->pos().y );
        const rcsc::AngleDeg target_angle = ( target_point - agent->world().ball().pos() ).th();
        const rcsc::AngleDeg minus_target_angle = -target_angle;
        const rcsc::Vector2D first_vel = rcsc::Vector2D::polar2vector( first_speed, target_angle );
        const double next_speed = first_speed * rcsc::ServerParam::i().ballDecay();

        bool success = true;
        double opp_min_step = 100.0;

        for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end; ++it )
        {
            double opp_to_target_dist = (*it)->pos().dist( target_point );
            if ( (*it)->goalie() )
            {
                if ( opp_to_target_dist - 3.0 < dash_x )
                {
                    success = false;
                    break;
                }
            }
            else if ( (*it)->pos().dist( target_point ) < dash_x - 3.0 )
            {
                success = false;
                break;
            }

            rcsc::Vector2D ball_to_opp = (*it)->pos();
            ball_to_opp -= agent->world().ball().pos();
            ball_to_opp -= first_vel;
            ball_to_opp.rotate( minus_target_angle );

            if ( 0.0 < ball_to_opp.x && ball_to_opp.x < target_dist )
            {
                const double virtual_dash = blocker_dash_speed * std::min( 3, (*it)->posCount() );
                double opp2line_dist = ball_to_opp.absY();
                opp2line_dist -= virtual_dash;
                if ( (*it)->goalie() )
                    opp2line_dist -= rcsc::ServerParam::i().catchAreaLength();
                else
                    opp2line_dist -= (*it)->playerTypePtr()->kickableArea();

                if ( opp2line_dist < 0.0 )
                {
                    success = false;
                    break;
                }

                const double ball_steps_to_project = rcsc::calc_length_geom_series( next_speed, ball_to_opp.x, rcsc::ServerParam::i().ballDecay() );

                double opp_step = (*it)->playerTypePtr()->cyclesToReachDistance( opp2line_dist );
                if ( ball_steps_to_project < 0.0 || opp_step < ball_steps_to_project )
                {
                    success = false;
                    break;
                }

                if ( opp_step < opp_min_step )
                {
                    opp_min_step = opp_step;
                }
            }
        }

        if ( success )
        {
            double tmp = 0.0;
            tmp -= dash_x;
            double recv_x_diff = std::fabs( receiver->pos().x - agent->world().ball().pos().x );
            tmp -= rcsc::min_max( -10.0, recv_x_diff * 2.0, 10.0 );

            if ( target_point.x > 40.0 && target_point.absY() < 7.0 )
                tmp += 500.0;
            S_cached_target.push_back( Target( receiver, target_point, first_speed, tmp ) );
        }
    }

}

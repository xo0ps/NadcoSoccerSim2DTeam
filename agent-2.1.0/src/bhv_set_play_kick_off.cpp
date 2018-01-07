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

#include "body_go_to_point.h"
#include "body_kick_one_step.h"
#include "bhv_go_to_static_ball.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_set_play_kick_off.h"

#include <rcsc/common/server_param.h>

#include <rcsc/player/player_agent.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlayKickOff::execute( rcsc::PlayerAgent * agent )
{
    // check kicker
    //if ( isKicker( agent ) )
    if( Bhv_SetPlay::is_kicker( agent ) )
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
Bhv_SetPlayKickOff::isKicker( const rcsc::PlayerAgent * agent )
{
    if ( ! agent->world().teammatesFromBall().empty()
         && ( agent->world().teammatesFromBall().front()->distFromBall()
              < agent->world().ball().distFromSelf() ) )
    {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayKickOff::doKick( rcsc::PlayerAgent * agent )
{
    static int S_scan_count = -5;

    // go to ball
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }

    const rcsc::WorldModel & wm = agent->world();

    // already kick point
    if ( wm.self().body().abs() < 175.0 )
    {
        rcsc::Vector2D face_point( -rcsc::ServerParam::i().pitchHalfLength(), 0.0 );
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( S_scan_count < 0 )
    {
        S_scan_count++;
        rcsc::Body_TurnToAngle( 180.0 ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    if ( S_scan_count < 10 && wm.teammatesFromSelf().empty() )
    {
        S_scan_count++;
        rcsc::Body_TurnToAngle( 180.0 ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }

    S_scan_count = -5;

    const double max_kick_speed = wm.self().kickRate() * rcsc::ServerParam::i().maxPower();

    rcsc::Vector2D target_point;
    double ball_speed = 0.0;

    // teammate not found
    if ( wm.teammatesFromSelf().empty() )
    {
        target_point.assign( rcsc::ServerParam::i().pitchHalfLength(),
                             static_cast< double >( -1 + 2 * wm.time().cycle() % 2 ) * 0.8 * rcsc::ServerParam::i().goalHalfWidth() );
        ball_speed = max_kick_speed;
    }
    else
    {
        const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
		const rcsc::PlayerPtrCont::const_iterator end = team.end();
		for ( rcsc::PlayerPtrCont::const_iterator it = team.begin(); it != end; ++it )
		{
			if(! (*it) )
				continue;
			if( (*it)->pos().x > -5.0 )
				continue;
			target_point = (*it)->pos();
			break;
		}
		double dist = target_point.dist( wm.ball().pos() );
        
        // too far
        if ( dist > 35.0 )
        {
            target_point.assign( rcsc::ServerParam::i().pitchHalfLength(),
                                 static_cast< double >( -1 + 2 * wm.time().cycle() % 2 ) * 0.8 * rcsc::ServerParam::i().goalHalfWidth() );
            ball_speed = max_kick_speed;
        }
        else
        {
            //target_point = wm.teammatesFromSelf().front()->pos();
            ball_speed = rcsc::calc_first_term_geom_series_last( 1.6, dist, rcsc::ServerParam::i().ballDecay() );
            ball_speed = std::max( ball_speed, wm.self().playerType().playerSize() + wm.self().playerType().kickableArea() );
            ball_speed = std::min( ball_speed, max_kick_speed );
            ball_speed = std::min( ball_speed , rcsc::ServerParam::i().maxPower() );
        }
    }

    Body_KickOneStep( target_point, ball_speed ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayKickOff::doMove( rcsc::PlayerAgent * agent )
{
	rcsc::Vector2D target_pos = M_home_pos;
	if( target_pos.x > -1.0 )
		target_pos.x = -1.0;
	
    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = agent->world().ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    if ( ! Body_GoToPoint( target_pos, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Body_TurnToBall().execute( agent );
    }
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

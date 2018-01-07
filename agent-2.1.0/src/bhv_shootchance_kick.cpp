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

#include "strategy.h"
#include "sample_player.h"
#include "body_pass.h"
#include "body_through_pass.h"
#include "body_through_pass2.h"
#include "body_leading_pass.h"
#include "body_direct_pass.h"
#include "body_selection_pass.h"
#include "bhv_shootchance_kick.h"
#include "body_dash.h"
#include "body_clear_ball.h"
#include "body_dribble.h"
#include "body_shoot.h"
#include "body_kick_to_corner.h"
#include "body_kick_to_center.h"

#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/intention_dribble2008.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_ShootChanceKick::execute( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();

	const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerObject * nearest_opp = ( opps.empty() ? static_cast< rcsc::PlayerObject * >( 0 ) : opps.front() );
    const double nearest_opp_dist = ( nearest_opp ? nearest_opp->distFromBall() : 1000.0 );
    const rcsc::Vector2D nearest_opp_pos = ( nearest_opp ? nearest_opp->pos() : rcsc::Vector2D( -1000.0, 0.0 ) );
	

	rcsc::Vector2D target( 45.0 , 0.0 );
	if( wm.ball().pos().x > target.x )
		target.x = 49.0;
	rcsc::AngleDeg deg = ( wm.ball().pos() - target ).th();
	rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , deg );
	const rcsc::Sector2D sector( wm.self().pos(), 0.5 , 10.0 , deg - 30.0, deg + 30.0 );
	
	static int counter = 0;
	if( Body_Dash::test( agent ) )
	{
		if( counter == 1 )
		{
			if( Body_Dash().execute( agent ) )
			{
				return true;
			}
		}
		counter++;
	}
	else
	{
		counter = 0;
	}

	rcsc::Vector2D point( -100 , -100 );
	if( Body_DirectPass::test_new( agent , &point ) )
	{
		if( point.x > 45.0 && point.absY() < 15.0 )
		{
			if( Body_DirectPass("playOn").execute( agent ) )
			{
				return true;
			}	
		}
	}
	
	if ( nearest_opp_dist < 7.0 )
	{
		if( Body_DirectPass("playOn").execute( agent ) )
		{
			return true;
		}
	}
	
	if ( nearest_opp_dist < 5.0
	  && nearest_opp_dist > ( rcsc::ServerParam::i().tackleDist() + rcsc::ServerParam::i().defaultPlayerSpeedMax() * 1.5 )
      //&& wm.self().body().abs() < 90.0 )
      )
    {
        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( max_dir_count < 3 )
        {
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
                Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 4 ).execute( agent );
                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
        else
        {
			Body_HoldBall().execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
    }
	
	if ( nearest_opp_pos.x < wm.self().pos().x + 1.0 )
    {
		const int max_dash_step = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
		Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), std::min( 5 , max_dash_step ) ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
    }

    if ( nearest_opp_dist > 5.0 )
    {
        Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower() * 0.4 , 1 , false ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }
    
	if( Body_DirectPass("playOn").execute( agent ) )
	{
		return true;
	}
	
	if ( nearest_opp_dist > 3.0 )
    {
        Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower() * 0.2 , 1 , false ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }
	
	if ( nearest_opp_dist > 2.5 )
    {
        Body_HoldBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }
	
	if( Body_ClearBall().execute( agent ) )
	{
		return true;
	}

	std::cout<<"shootchance false"<<std::endl;
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    return false;
}

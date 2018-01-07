// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

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
#include "body_leading_pass.h"
#include "body_direct_pass.h"
#include "body_through_pass.h"
#include "bhv_danger_kick.h"
#include "body_clear_ball.h"
#include "bhv_block.h"
#include "bhv_tactics.h"
#include "body_dribble.h"

#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DangerKick::execute( rcsc::PlayerAgent * agent )
{  
	const rcsc::WorldModel & wm = agent->world();
	if( Strategy::i().danger_fast_pass() )
	{
		const rcsc::PlayerObject * nearest_opp = wm.getOpponentNearestToSelf( 7 );
		const double nearest_opp_dist = ( nearest_opp ? nearest_opp->distFromSelf() : 1000.0 );
		
		if( nearest_opp_dist < 5.0 )
		{
			if( Body_ClearBall().execute( agent ) )
			{
				return true;
			}
		}

		if( Bhv_Tactics( "substitueKick" ).execute( agent ) )
		{
			return true;
		}
		
		/*
		rcsc::Vector2D point1( -100 , -100 );
		if( Body_LeadingPass::test( agent , &point1 ) && point1.x > wm.ball().pos().x + 3.0 && point1.absY() > 20.0 )
		{
			if( Body_LeadingPass("playOn").execute( agent ) )
			{
				return true;
			}
		}
		*/
		
		if( Body_ClearBall::furthestTM( agent ) )
		{
			return true;
		}
		
		rcsc::Vector2D point( -100 , -100 );
		if( Body_DirectPass::test_new( agent , &point ) )
		{
			if( point.x > wm.ball().pos().x + 10.0 )
			{
				if( Body_DirectPass("playOn").execute( agent ) )
				{
					return true;
				}	
			}
		}
		
		rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
		const rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0, -30.0, 30.0 );
		if ( ! wm.existOpponentIn( sector, 10, true ) )
		{
			Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 1 ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
		else
		{
			Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower() * 0.8 , 2 , false ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
		
		if( Body_ClearBall().execute( agent ) )
		{
			return true;
		}
				
		std::cout<<"danger false"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	    return false;
	}
	
	std::cout<<"danger false"<<std::endl;
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return false;
}

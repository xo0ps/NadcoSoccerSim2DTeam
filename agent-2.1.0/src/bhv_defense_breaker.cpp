// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

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

#include "bhv_defense_breaker.h"
#include "sample_player.h"
#include "strategy.h"
#include "bhv_predictor.h"
#include "body_pass.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DefenseBreaker::execute( rcsc::PlayerAgent * agent )
{

	//if ( ! SamplePlayer::instance().def_break() )
	if ( ! Strategy::i().def_break() )
    {
		return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();
        
	if( ! wm.self().isKickable() )
		return false;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	
	std::vector< rcsc::PlayerObject * >candidates;
	
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 10 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if ( (*tm)->isTackling() )
			continue;
		rcsc::Vector2D tmpos = (*tm)->pos();
		rcsc::Vector2D ball_pos = wm.ball().pos();
		if( tmpos.x < -29 && tmpos.absY() < 18 )
			continue;
		if( tmpos.x < ball_pos.x + 1.0 )
			continue;
		if( tmpos.x > 50.0 || tmpos.absY() > 31.0 )
			continue;
		if( tmpos.x < offside - 10.0 )
			continue;
		if( tmpos.x > offside - 1.0 )
			continue;
		
		rcsc::Vector2D point( offside - 1.0 , tmpos.y );
				
		if( point.dist( ball_pos ) > max_dp_dist )
			continue;
		if( point.dist( ball_pos ) < 20.0 )
			continue;
		if( point.x < ball_pos.x + 5.0 )
			continue;
		if( point.x > 48.0 )
			continue;
		if( point.absY() > 30.0 )
			continue;
					
		if( Body_Pass::exist_opponent3( agent , point , true ) )
			continue;
						
		const rcsc::PlayerObject * nearest = wm.getOpponentNearestTo( point , 5 , NULL );
		if( nearest &&
			Bhv_Predictor::predict_player_reach_cycle( nearest, point, 0.5, 0.0, 1, 1, 0, false )
			>= Bhv_Predictor::predict_player_reach_cycle( (*tm) , point, 0.5, 0.0, 1, 1, 0, false ) )
		{
			candidates.push_back( *tm );
		}
	}
    
    
    
    
	return false;
}

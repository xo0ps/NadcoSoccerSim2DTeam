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

#include "bhv_goal_patterns.h"
#include "sample_player.h"
#include "strategy.h"
#include "bhv_block.h"
#include "body_go_to_point.h"
#include "bhv_cross_move.h"
#include "bhv_basic_tackle.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_neck_body_to_ball.h>
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalPatterns::execute( rcsc::PlayerAgent * agent )
{	
	
	//if ( ! SamplePlayer::instance().goal_patterns() )
	if ( ! Strategy::i().goal_patterns() )
    {
		return false;
    }
    
	return false;
}



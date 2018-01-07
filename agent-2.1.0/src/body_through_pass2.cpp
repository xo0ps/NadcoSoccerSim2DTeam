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

#include "body_through_pass2.h"
#include "body_through_pass.h"
#include "body_pass.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "bhv_predictor.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/audio_memory.h>

/*-------------------------------------------------------------------*/

bool Body_ThroughPass2::execute( rcsc::PlayerAgent * agent )
{
		
	/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return Body_ThroughPass( "toBehindDefenders" ).execute( agent );

}
/*-----------------------------------------------------------------------*/

bool Body_ThroughPass2::calculate( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, int * receiver )
{
		/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return false;

}



/*-----------------------------------------------------------------------*/

bool Body_ThroughPass2::test( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, int * receiver )
{

	
	return false;
}

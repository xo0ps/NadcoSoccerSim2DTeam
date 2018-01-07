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


#include "strategy.h"
#include "bhv_basic_tackle.h"
#include "bhv_danger_area_tackle.h"
#include "body_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/player/intercept_table.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Tackle::execute(rcsc::PlayerAgent* agent)
{

	if ( ! Strategy::i().tackle() )
    {
	    return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

	if( self_min < mate_min && self_min < opp_min )
		return false;
	
	if( mate_min < opp_min && mate_min < self_min )
		return false;

	rcsc::Vector2D selfReachPoint = wm.ball().inertiaPoint( self_min );
	rcsc::Vector2D teammateReachPoint = wm.ball().inertiaPoint( mate_min );
	rcsc::Vector2D opponentReachPoint = wm.ball().inertiaPoint( opp_min );
	
	const rcsc::PlayerObject * fastest_tmm = wm.interceptTable()->fastestTeammate();
	if( fastest_tmm )
	{
		if( fastest_tmm->isSelf() && selfReachPoint.absX() < 52.0 && selfReachPoint.absY() < 34.0 )
			return false;
	}

	/*
	if( Bhv_DangerAreaTackle().execute( agent ) )
	{
		return true;
	}
	else
	*/
	if( Bhv_BasicTackle( 0.85 , 80 ).execute( agent ) )
	{
		return true;
	}

	rcsc::Line2D line( wm.ball().pos(), wm.ball().vel() * 100.0 );
	rcsc::Vector2D bottom_left( -52.5 , -8.0 ), top_left( -52.5 , 8.0 );
	rcsc::Vector2D inters = line.intersection( rcsc::Line2D( bottom_left, top_left ) );
	
	bool goal_danger = false;
	
	//if( inters.isValid() )
	//	goal_danger = true;
	
	if( selfReachPoint.absX() < -52.0 && selfReachPoint.absY() < 8.0 )
		goal_danger = true;
	
	if( teammateReachPoint.absX() < -52.0 && teammateReachPoint.absY() < 8.0 )
		goal_danger = true;
		
	if( opponentReachPoint.absX() < -52.0 && opponentReachPoint.absY() < 8.0 )
		goal_danger = true;
	
	if( goal_danger )
	{
		if( Bhv_DangerAreaTackle( 0.1 ).execute( agent ) )
		{
			return true;
		}
		else
		if( Bhv_BasicTackle( 0.1 , 50 ).execute( agent ) )
		{
			return true;
		}
	}
	
	return false;
}

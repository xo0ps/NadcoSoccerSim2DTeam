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

#include "bhv_after_goal.h"
#include "strategy.h"
#include "bhv_before_kick_off.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_AfterGoal::execute( rcsc::PlayerAgent * agent )
{
	rcsc::Vector2D move_point;
	
	if ( agent->world().setplayCount() % 10 == 0 && agent->world().setplayCount() < 40 )
	{
		int Unum = agent->world().self().unum();
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0) , -10 * sin(0.0) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0) , -10 * sin(36.0) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0) , -10 * sin(72.0) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0) , -10 * sin(108.0) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0) , -10 * sin(144.0) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0) , -10 * sin(180.0) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0) , -10 * sin(216.0) );
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0) , -10 * sin(252.0) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0) , -10 * sin(288.0) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0) , -10 * sin(324.0) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0) , -10 * sin(360.0) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );

	if ( agent->world().setplayCount() % 10 == 1 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 10) , -10 * sin(0.0 + 10) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 10) , -10 * sin(36.0 + 10) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 10) , -10 * sin(72.0 + 10) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 10) , -10 * sin(108.0 + 10) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 10) , -10 * sin(144.0 + 10) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 10) , -10 * sin(180.0 + 10) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 10) , -10 * sin(216.0 + 10) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 10) , -10 * sin(252.0 + 10) );
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 10) , -10 * sin(288.0 + 10) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 10) , -10 * sin(324.0 + 10) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 10) , -10 * sin(360.0 + 10) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );

	if ( agent->world().setplayCount() % 10 == 2 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 20) , -10 * sin(0.0 + 20) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 20) , -10 * sin(36.0 + 20) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 20) , -10 * sin(72.0 + 20) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 20) , -10 * sin(108.0 + 20) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 20) , -10 * sin(144.0 + 20) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 20) , -10 * sin(180.0 + 20) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 20) , -10 * sin(216.0 + 20) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 20) , -10 * sin(252.0 + 20) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 20) , -10 * sin(288.0 + 20) );
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 20) , -10 * sin(324.0 + 20) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 20) , -10 * sin(360.0 + 20) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );

	if ( agent->world().setplayCount() % 10 == 3 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 30) , -10 * sin(0.0 + 30) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 30) , -10 * sin(36.0 + 30) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 30) , -10 * sin(72.0 + 30) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 30) , -10 * sin(108.0 + 30) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 30) , -10 * sin(144.0 + 30) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 30) , -10 * sin(180.0 + 30) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 30) , -10 * sin(216.0 + 30) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 30) , -10 * sin(252.0 + 30) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 30) , -10 * sin(288.0 + 30) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 30) , -10 * sin(324.0 + 30) );
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 30) , -10 * sin(360.0 + 30) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );

	if ( agent->world().setplayCount() % 10 == 4 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 40) , -10 * sin(0.0 + 40) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 40) , -10 * sin(36.0 + 40) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 40) , -10 * sin(72.0 + 40) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 40) , -10 * sin(108.0 + 40) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 40) , -10 * sin(144.0 + 40) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 40) , -10 * sin(180.0 + 40) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 40) , -10 * sin(216.0 + 40) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 40) , -10 * sin(252.0 + 40) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 40) , -10 * sin(288.0 + 40) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 40) , -10 * sin(324.0 + 40) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 40) , -10 * sin(360.0 + 40) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
    
	if ( agent->world().setplayCount() % 10 == 5 && agent->world().setplayCount() < 40 )
	{
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 50) , -10 * sin(0.0 + 50) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 50) , -10 * sin(36.0 + 50) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 50) , -10 * sin(72.0 + 50) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 50) , -10 * sin(108.0 + 50) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 50) , -10 * sin(144.0 + 50) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 50) , -10 * sin(180.0 + 50) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 50) , -10 * sin(216.0 + 50) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 50) , -10 * sin(252.0 + 50) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 50) , -10 * sin(288.0 + 50) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 50) , -10 * sin(324.0 + 50) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 50) , -10 * sin(360.0 + 50) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
    
    if ( agent->world().setplayCount() % 10 == 6 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 60) , -10 * sin(0.0 + 60) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 60) , -10 * sin(36.0 + 60) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 60) , -10 * sin(72.0 + 60) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 60) , -10 * sin(108.0 + 60) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 60) , -10 * sin(144.0 + 60) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 60) , -10 * sin(180.0 + 60) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 60) , -10 * sin(216.0 + 60) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 60) , -10 * sin(252.0 + 60) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 60) , -10 * sin(288.0 + 60) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 60) , -10 * sin(324.0 + 60) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 60) , -10 * sin(360.0 + 60) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
    
    if ( agent->world().setplayCount() % 10 == 7 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 70) , -10 * sin(0.0 + 70) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 70) , -10 * sin(36.0 + 70) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 70) , -10 * sin(72.0 + 70) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 70) , -10 * sin(108.0 + 70) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 70) , -10 * sin(144.0 + 70) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 70) , -10 * sin(180.0 + 70) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 70) , -10 * sin(216.0 + 70) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 70) , -10 * sin(252.0 + 70) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 70) , -10 * sin(288.0 + 70) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 70) , -10 * sin(324.0 + 70) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 70) , -10 * sin(360.0 + 70) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
    
    if ( agent->world().setplayCount() % 10 == 8 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 80) , -10 * sin(0.0 + 80) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 80) , -10 * sin(36.0 + 80) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 80) , -10 * sin(72.0 + 80) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 80) , -10 * sin(108.0 + 80) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 80) , -10 * sin(144.0 + 80) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 80) , -10 * sin(180.0 + 80) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 80) , -10 * sin(216.0 + 80) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 80) , -10 * sin(252.0 + 80) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 80) , -10 * sin(288.0 + 80) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 80) , -10 * sin(324.0 + 80) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 80) , -10 * sin(360.0 + 80) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
    
    if ( agent->world().setplayCount() % 10 == 9 && agent->world().setplayCount() < 40 )
	{
		
		int Unum = agent->world().self().unum();
		if ( Unum == 8 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(0.0 + 90) , -10 * sin(0.0 + 90) );
		if ( Unum == 9 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(36.0 + 90) , -10 * sin(36.0 + 90) );
		if ( Unum == 10 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(72.0 + 90) , -10 * sin(72.0 + 90) );
		if ( Unum == 11 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(108.0 + 90) , -10 * sin(108.0 + 90) );
		if ( Unum == 1 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(144.0 + 90) , -10 * sin(144.0 + 90) );
		if ( Unum == 2 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(180.0 + 90) , -10 * sin(180.0 + 90) );
		if ( Unum == 3 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(216.0 + 90) , -10 * sin(216.0 + 90) );
		if ( Unum == 4 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(252.0 + 90) , -10 * sin(252.0 + 90) );
		if ( Unum == 5 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(288.0 + 90) , -10 * sin(288.0 + 90) );
		if ( Unum == 6 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(324.0 + 90) , -10 * sin(324.0 + 90) );
		if ( Unum == 7 )
			move_point = rcsc::Vector2D ( -26 - 10 * cos(360.0 + 90) , -10 * sin(360.0 + 90) );
	}
    agent->setViewAction( new rcsc::View_Wide() );
    Bhv_BeforeKickOff( move_point ).execute( agent );
	
	if ( agent->world().setplayCount() >= 40 )
	{
		move_point = Strategy::i().getPosition( agent->world().self().unum() );
		agent->setViewAction( new rcsc::View_Wide() );
		Bhv_BeforeKickOff( move_point ).execute( agent );
	}
	return true;
}

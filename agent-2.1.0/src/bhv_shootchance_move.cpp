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

#include "strategy.h"
#include "sample_player.h"
#include "bhv_shootchance_move.h"
#include "bhv_block.h"
#include "body_tackle.h"
#include "body_go_to_point.h"
#include "body_pass.h"
#include "body_shoot.h"
#include "bhv_mark.h"
#include "bhv_global_positioning.h"
#include "body_intercept.h"
#include "bhv_through_pass_cut.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_point.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/geom/sector_2d.h>

#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_ShootChanceMove::execute( rcsc::PlayerAgent * agent )
{
		
	if( Body_Tackle().execute( agent ) )
		return true;
    
    const rcsc::WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }
    
	//goToReceivablePoint( agent );

	if( Bhv_Block().execute( agent ) )
	{
		return true;
	}
	
	if( Bhv_ThroughPassCut().execute( agent ) )
		return true;

	if( Bhv_GlobalPositioning( M_home_pos ).execute( agent ) )
		return true;

	std::cout<<"shootchance move false"<<std::endl;
	return false;
}

/*---------------------------------------------------------*/

void
Bhv_ShootChanceMove::goToReceivablePoint( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D shoot_pos( 52.5 , 0 );
    rcsc::Vector2D pos = M_home_pos;
    if ( wm.existKickableTeammate() && ( wm.self().unum() == 9 || wm.self().unum() == 10 ) )
    {
		pos = rcsc::Vector2D( std::max( wm.self().pos().x , wm.offsideLineX() - 2.0 ) ,10 );
		
	    if ( wm.self().unum() == 9 )
			pos.y = -10;
	}
	
    if ( wm.existKickableTeammate() && ( wm.self().unum() == 7 || wm.self().unum() == 8 ) )
    {
		pos = rcsc::Vector2D( std::max( wm.self().pos().x , wm.offsideLineX() - 2.0 ) , 15 );
		
	    if ( wm.self().unum() == 7 )
			pos.y = -15;
	}
	
    if ( wm.existKickableTeammate() && wm.self().unum() == 6 )
    {
		pos = rcsc::Vector2D( std::max( wm.self().pos().x , wm.offsideLineX() - 5.0 ) , 1 );
	}

	if ( wm.existKickableTeammate() && wm.self().unum() == 11 )
    {
		pos = rcsc::Vector2D( std::max( wm.self().pos().x , wm.offsideLineX() - 2.0 ) , -1 );
	}

    bool pass = false;
	rcsc::AngleDeg deg = ( wm.ball().pos() - pos ).th();
	/*
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( wm.ball().pos() , a , b );
	*/
	rcsc::Sector2D route( wm.ball().pos() , 0.0 , wm.ball().pos().dist( pos ) , deg - 10 , deg + 10 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( wm.self().pos().dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
		{
			rcsc::Vector2D pass_pos = rcsc::Vector2D( ( wm.self().pos().x + pos.x ) / 3 , wm.self().pos().y );
			Body_Pass::passRequest( agent , pass_pos );
		}
	}
			
	//double dash_power = getDashPower( agent, pos );
	double dash_power = rcsc::ServerParam::i().maxDashPower();
	dash_power *= 2.0;
	dash_power = std::min( dash_power , rcsc::ServerParam::i().maxDashPower() );
	double dist_thr = wm.self().pos().dist( pos ) * 0.1;
	if( dist_thr < 0.5 )
		dist_thr = 0.5;

	if( ! Body_GoToPoint( pos, dist_thr, dash_power ).execute( agent ) )
		if( std::fabs( wm.self().body().degree() - ( wm.self().pos() - wm.ball().pos() ).th().degree() ) > 90.0 )
			rcsc::Body_TurnToBall().execute( agent );
		
	if ( wm.ball().distFromSelf() < 18.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	else
    	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
}

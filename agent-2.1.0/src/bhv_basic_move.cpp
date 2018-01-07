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

#include "strategy.h"
#include "sample_player.h"
#include "body_pass.h"
#include "bhv_basic_move.h"
#include "bhv_block.h"
#include "bhv_basic_tackle.h"
#include "body_go_to_point.h"
#include "body_intercept.h"
#include "bhv_global_positioning.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/geom/sector_2d.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_BasicMove::execute( rcsc::PlayerAgent * agent )
{

	if ( Strategy::i().tackle() )
    {
	    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
	    {
	        return true;
	    }
    }
    
    const rcsc::WorldModel & wm = agent->world();
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min < mate_min + 3
                   && self_min < opp_min + 4 )
              )
         )
    {
        Body_Intercept( ).execute( agent );

        if ( M_turn_neck )
        {
            if ( wm.ball().distFromSelf()
                 < rcsc::ServerParam::i().visibleDistance() )
            {
                agent->setNeckAction( new rcsc::Neck_TurnToBall() );
                return true;
            }
            else
            {
                agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
                return true;
            }
        }

    }
    
	if( Bhv_Block().execute( agent ) )
	{
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}
	
	/*
	if( Bhv_Mark( "markPass" , M_home_pos ).execute( agent ) )
		return true;    
    */
    
    //goToReceivablePoint( agent );	
    
    if( Bhv_GlobalPositioning( M_home_pos ).execute( agent ) )
		return true;

	return true;

}

/*---------------------------------------------------------*/

void
Bhv_BasicMove::goToReceivablePoint( rcsc::PlayerAgent * agent )
{

    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D pos = M_home_pos;

	//Bhv_Mark( "markEscape" , M_home_pos ).execute( agent );
    
    if ( wm.existKickableTeammate() && wm.self().pos().x > 0 && ( wm.self().unum() == 9 || wm.self().unum() == 10 ) )
    {
		pos.x = std::max( wm.self().pos().x , wm.offsideLineX() - 2.0 );
	}
	
    if ( wm.existKickableTeammate() && wm.self().pos().x > 0 && ( wm.self().unum() == 7 || wm.self().unum() == 8 ) )
    {
		pos.x = std::max( wm.self().pos().x , wm.offsideLineX() - 5.0 );
	}
	
    if ( wm.existKickableTeammate() && wm.self().pos().x > 0 && wm.self().unum() == 6 )
    {
		pos.x = std::max( wm.self().pos().x , wm.offsideLineX() - 20.0 );
	}

	if ( wm.existKickableTeammate() && wm.self().pos().x > 0 && wm.self().unum() == 11 )
    {
		pos.x = std::max( wm.self().pos().x , wm.offsideLineX() - 2.0 );
	}
    
    /*
    if ( wm.existKickableTeammate() && wm.self().pos().x < 0 && ( wm.self().unum() == 9 || wm.self().unum() == 10 ) )
    {
		pos.x = std::min( wm.ball().pos().x + 15.0 , wm.offsideLineX() - 2.0 );
	}
		
    if ( wm.existKickableTeammate() && wm.ball().pos().x < 0 && ( wm.self().unum() == 7 || wm.self().unum() == 8 ) )
    {
		pos.x = std::min( wm.ball().pos().x + 10.0 , wm.offsideLineX() - 2.0 );
	}
	
    if ( wm.existKickableTeammate() && wm.ball().pos().x < 0 && wm.self().unum() == 6 )
    {
		pos.x = std::min( wm.ball().pos().x + 7.0 , wm.offsideLineX() - 2.0 );
	}

	if ( wm.existKickableTeammate() && wm.ball().pos().x < 0 && wm.self().unum() == 11 )
    {
		pos.x = std::min( wm.ball().pos().x + 16.0 , wm.offsideLineX() - 2.0 );
	}
	*/

    bool pass = false;
	rcsc::AngleDeg deg = ( wm.ball().pos() - pos ).th();
	/*
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( wm.ball().pos() , a , b );
	*/
	rcsc::Sector2D route( wm.ball().pos() , 0.0 , wm.ball().pos().dist( pos ) , deg - 15 , deg + 15 );
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
	double dist_thr = wm.self().pos().dist( pos ) * 0.1;
	if( dist_thr < 0.5 )
		dist_thr = 0.5;

	rcsc::Vector2D target( 52 , 0 );
	rcsc::AngleDeg deg1 = ( wm.self().pos() - target ).th();
	if( ! Body_GoToPoint( pos, dist_thr, dash_power ).execute( agent ) )
		//if( std::fabs( wm.self().body().degree() - ( wm.self().pos() - wm.ball().pos() ).th().degree() ) > 90.0 )
			//rcsc::Body_TurnToBall().execute( agent );
			rcsc::Body_TurnToAngle( deg1 ).execute( agent );
		
	if ( wm.ball().distFromSelf() < 18.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	else
    	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
}

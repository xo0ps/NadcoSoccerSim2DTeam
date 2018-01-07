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
#include "bhv_cross_move.h"
#include "bhv_danger_move.h"
#include "bhv_shootchance_move.h"
#include "bhv_block.h"
#include "body_tackle.h"
#include "body_go_to_point.h"
#include "bhv_mark.h"
#include "bhv_tactics.h"
#include "bhv_hassle.h"
#include "body_pass.h"
#include "bhv_global_positioning.h"
#include "bhv_spread_positioning.h"
#include "body_intercept.h"
#include "bhv_through_pass_cut.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_CrossMove::execute( rcsc::PlayerAgent * agent )
{
	
	if( Body_Tackle().execute( agent ) )
		return true;

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3)
              )
         )
    {
		//std::cout<<wm.self().unum()<<" intercept"<<std::endl;
        Body_Intercept().execute( agent );
		//agent->setNeckAction( new rcsc::Neck_OffensiveInterceptNeck() );
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }
    
    if( Bhv_Block().execute( agent ) )
	{
		return true;
	}
    
		
	/*
	if( Bhv_Mark( "markPass" , M_home_pos ).execute( agent ) )
		return true;
    */
    
    //goToReceivablePoint( agent );
    
    if( wm.ball().pos().x < 0.0 )
    {
		//Bhv_Tactics( "substitueRequest" ).execute( agent );
		if( Bhv_Hassle( M_home_pos ).execute( agent ) )
			return true;
	}
	
	if( M_sideback && Bhv_ThroughPassCut().execute( agent ) )
	//if( Bhv_ThroughPassCut().execute( agent ) )
		return true;
	
	if( wm.existKickableOpponent() )
	if( fabs( wm.self().pos().y - wm.ball().pos().y ) < 5.0 && wm.self().pos().x < wm.ball().pos().x - 2.0 )
	{
		//Body_Intercept().execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}
	
	if( wm.self().pos().absY() > wm.ball().pos().absY() && wm.ball().pos().x < -34.0 )
	{
		//Body_Intercept().execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}

	if( wm.ball().pos().x < -46.0 )
	{
		//Body_Intercept().execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}

	if( Bhv_SpreadPositioning( M_home_pos ).execute( agent ) )
	{
		return true;
	}

	if( Bhv_GlobalPositioning( M_home_pos ).execute( agent ) )
	{
		return true;
	}
	
    
    std::cout<<"cross move false"<<std::endl;
	return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
double
Bhv_CrossMove::getDashPower( const rcsc::PlayerAgent * agent,
                             const rcsc::Vector2D & /*target_point*/ )
{
    static bool s_recover_mode = false;

    const rcsc::WorldModel & wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    // check recover
    if ( wm.self().stamina() < sp.staminaMax() * 0.5 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > sp.staminaMax() * 0.7 )
    {
        s_recover_mode = false;
    }

    /*--------------------------------------------------------*/
    double dash_power = 100.0;
    const double my_inc
        = wm.self().playerType().staminaIncMax()
        * wm.self().recovery();

    if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 )
			dash_power = 0.0;
    }
    // exist kickable teammate
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               sp.maxDashPower() );
    }
    // in offside area
    else if ( wm.self().pos().x > wm.offsideLineX() )
    {
        dash_power = sp.maxDashPower();
    }
    else if ( wm.ball().pos().x > 25.0
              && wm.ball().pos().x > wm.self().pos().x
              && wm.self().pos().x > 10.0
              && self_min < opp_min - 6
              && mate_min < opp_min - 6 )
    {
        dash_power = rcsc::bound( sp.maxDashPower() * 0.1,
                                  my_inc * 0.5,
                                  sp.maxDashPower() );
    }

    else
    {
        dash_power = std::min( my_inc * 1.7,
                               sp.maxDashPower() );
    }

    return dash_power;
}

/*---------------------------------------------------------*/

void
Bhv_CrossMove::goToReceivablePoint( rcsc::PlayerAgent * agent )
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
			pass  = false;
		if( pass )
		{
			rcsc::Vector2D pass_pos = rcsc::Vector2D( ( wm.self().pos().x + pos.x ) / 3 , wm.self().pos().y );
			Body_Pass::passRequest( agent , pass_pos );
		}
	}
	
	double dash_power = getDashPower( agent, pos );
	dash_power *= 1.2;
	dash_power = std::min( dash_power , rcsc::ServerParam::i().maxDashPower() );
	double dist_thr = wm.self().pos().dist( pos ) * 0.07;
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


/*----------------------------------------------*/
double
Bhv_CrossMove::get_dash_power( const rcsc::PlayerAgent * agent )
{
	
	const rcsc::WorldModel & wm = agent->world();
    static bool s_recover_mode = false;

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        return std::min( rcsc::ServerParam::i().maxDashPower(),
                         wm.self().stamina() + wm.self().playerType().extraStamina() );
    }

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    // check recover
    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        s_recover_mode = false;
    }
    else if ( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.5 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.7 )
    {
        s_recover_mode = false;
    }

    /*--------------------------------------------------------*/
    double dash_power = rcsc::ServerParam::i().maxDashPower();
    const double my_inc
        = wm.self().playerType().staminaIncMax()
        * wm.self().recovery();

    if ( wm.ourDefenseLineX() > wm.self().pos().x
         && wm.ball().pos().x < wm.ourDefenseLineX() + 20.0 )
    {
        dash_power = rcsc::ServerParam::i().maxDashPower();
    }
    else if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 ) dash_power = 0.0;

    }
    // exist kickable teammate
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               rcsc::ServerParam::i().maxDashPower() );
    }
    // in offside area
    else if ( wm.self().pos().x > wm.offsideLineX() )
    {
        dash_power = rcsc::ServerParam::i().maxDashPower();
    }
    else if ( wm.ball().pos().x > 25.0
              && wm.ball().pos().x > wm.self().pos().x + 10.0
              && self_min < opp_min - 6
              && mate_min < opp_min - 6 )
    {
        dash_power = rcsc::bound( rcsc::ServerParam::i().maxDashPower() * 0.1,
                            my_inc * 0.5,
                            rcsc::ServerParam::i().maxDashPower() );
    }
    // normal
    else
    {
        dash_power = std::min( my_inc * 1.7,
                               rcsc::ServerParam::i().maxDashPower() );
    }

    return dash_power;
}

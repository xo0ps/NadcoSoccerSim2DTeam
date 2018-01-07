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

#include "bhv_danger_move.h"
#include "bhv_cross_move.h"
#include "bhv_block.h"
#include "body_tackle.h"
#include "body_go_to_point.h"
#include "bhv_mark.h"
#include "strategy.h"
#include "bhv_hassle.h"
#include "bhv_tactics.h"
#include "body_intercept.h"
#include "bhv_global_positioning.h"
#include "bhv_through_pass_cut.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_ball.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DangerMove::execute( rcsc::PlayerAgent * agent )
{

	if( agent->world().self().unum() < 7 && doDefensiveMove( agent ) )
	//if( agent->world().self().unum() < 7 && Bhv_CrossMove::doCrossBlockMove( agent ) )
		return true;
	
	if( Body_Tackle().execute( agent ) )
		return true;

    const rcsc::WorldModel & wm = agent->world();

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
        Body_Intercept( false ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
	}

	if( Bhv_Block().execute( agent ) )
	{
		return true;
	}

	//Bhv_Tactics( "substitueRequest" ).execute( agent );
    if( Bhv_Hassle( M_home_pos ).execute( agent ) )
		return true;
	
	if( wm.existKickableOpponent() )
	if( fabs( wm.self().pos().y - wm.ball().pos().y ) < 8.0 && wm.self().pos().x < wm.ball().pos().x - 2.0 )
	{
		//Body_Intercept().execute( agent );
        //agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        //return true;
	}

	//if( Bhv_ThroughPassCut().execute( agent ) )
	//	return true;
	
	
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
		
	if( Bhv_GlobalPositioning( M_home_pos ).execute( agent ) )
		return true;
		
	std::cout<<"danger move false"<<std::endl;
	return false;
}

/*---------------------------------------------------------*/
bool 
Bhv_DangerMove::doDefensiveMove( rcsc::PlayerAgent * agent )
{
	return false;
    
    if( Body_Tackle().execute( agent ) )
		return true;
    
	const rcsc::WorldModel & wm = agent->world();

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( self_min <= opp_min && self_min <= mate_min && ! wm.existKickableTeammate() )
    {
        Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
	}

    const rcsc::PlayerObject * fastest_opp = wm.interceptTable()->fastestOpponent();
    rcsc::Vector2D opp_trap_pos = wm.ball().inertiaPoint( opp_min );
    const rcsc::Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
    const PositionType position_type = Strategy::i().getPositionType( wm.self().unum() );

    if ( wm.existKickableTeammate()
         || ! fastest_opp
         || opp_trap_pos.x > -36.0
         || opp_trap_pos.dist( home_pos ) > 7.0
         || opp_trap_pos.absY() > 9.0
         )
    {
        return false;
    }

    /*
    if ( ( position_type == Position_Left
           && home_pos.y + 3.0 < opp_trap_pos.y )
         || ( position_type == Position_Right
              && opp_trap_pos.y < home_pos.y - 3.0 )
         )
    {
        bool exist_blocker = false;
        const double my_dist = wm.self().pos().dist( opp_trap_pos );
        const rcsc::PlayerPtrCont::const_iterator end = wm.teammatesFromBall().end();
        for( rcsc::PlayerPtrCont::const_iterator p = wm.teammatesFromBall().begin(); p != end; ++p )
        {
            if ( (*p)->goalie() ) continue;
            if ( (*p)->isGhost() ) continue;
            if ( (*p)->posCount() >= 10 ) continue;
            if ( (*p)->pos().x > fastest_opp->pos().x + 2.0 ) continue;
            if ( (*p)->pos().dist( opp_trap_pos ) > my_dist + 1.0 ) continue;
            exist_blocker = true;
            break;
        }

        if ( exist_blocker )
            return false;
    }
    */

	if( Bhv_Block().execute( agent ) )
    {
        return true;
    }

	if( Bhv_ThroughPassCut().execute( agent ) )
    {
        return true;
    }

    const rcsc::PlayerObject * mark_target = wm.getOpponentNearestTo( home_pos, 1, NULL );

    if ( ! mark_target
         || mark_target->pos().x > -39.0
         || mark_target->pos().dist( home_pos ) > 3.0
         || mark_target->distFromBall() < mark_target->playerTypePtr()->kickableArea() + 0.5
         )
    {
        return false;
    }

    double marker_dist = 100.0;
    const rcsc::PlayerObject * marker = wm.getTeammateNearestTo( mark_target->pos(), 30, &marker_dist );
    if ( marker && marker->pos().x < mark_target->pos().x + 1.0 && marker_dist < mark_target->distFromSelf() )
    {
        return false;
    }

    rcsc::Vector2D mark_point = mark_target->pos();
    mark_point += mark_target->vel();
    mark_point.x -= 0.9;
    mark_point.y += ( mark_target->pos().y > wm.ball().pos().y ? -0.6 : 0.6 );

    if ( mark_point.x > wm.ball().pos().x + 5.0 )
    {
        return false;
    }

    double dash_power = rcsc::ServerParam::i().maxDashPower();
    double x_diff = mark_point.x - wm.self().pos().x;

    if ( x_diff > 20.0 )
    {
        dash_power = wm.self().playerType().staminaIncMax() * wm.self().recovery();
    }
    else if ( x_diff > 10.0 )
    {
        dash_power *= 0.7;
    }
    else if ( wm.ball().pos().dist( mark_point ) > 20.0 )
    {
        dash_power *= 0.6;
    }

    double dist_thr = wm.ball().distFromSelf() * 0.05;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    if ( wm.self().pos().x < mark_point.x && wm.self().pos().dist( mark_point ) < dist_thr )
    {
        if( mark_target && mark_target->unum() == rcsc::Unum_Unknown )
        {
            rcsc::Vector2D target_pos = mark_target->pos() + mark_target->vel();
            rcsc::Bhv_NeckBodyToPoint( target_pos, 10.0 ).execute( agent );
            return true;
        }

        rcsc::AngleDeg body_angle = ( wm.ball().pos().x < wm.self().pos().x - 5.0 ? 0.0 : 180.0 );
        rcsc::Body_TurnToAngle( body_angle ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    if ( wm.self().pos().dist( mark_point ) > 3.0 )
    {
        Body_GoToPoint( mark_point, dist_thr, dash_power ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    return false;
}

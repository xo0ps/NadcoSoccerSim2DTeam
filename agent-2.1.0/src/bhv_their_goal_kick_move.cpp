// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

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

#include "bhv_their_goal_kick_move.h"
#include "body_go_to_point.h"
#include "bhv_set_play.h"
#include "body_kick_one_step.h"
#include "bhv_mark.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_TheirGoalKickMove::execute( rcsc::PlayerAgent * agent )
{
    static const rcsc::Rect2D
        expand_their_penalty( rcsc::Vector2D( rcsc::ServerParam::i().theirPenaltyAreaLineX() - 0.75,
                                              -rcsc::ServerParam::i().penaltyAreaHalfWidth() - 0.75 ),
                              rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() + 0.75,
                                            rcsc::ServerParam::i().penaltyAreaWidth() + 1.5 ) );

    const rcsc::WorldModel & wm = agent->world();

    if ( doChaseBall( agent ) )
    {
        return true;
    }

    rcsc::Vector2D intersection;

    if ( wm.ball().vel().r() > 0.2 )
    {
        if ( ! expand_their_penalty.contains( wm.ball().pos() )
             || expand_their_penalty.intersection( rcsc::Ray2D
                                                   ( wm.ball().pos(),
                                                     wm.ball().vel().th() ),
                                                   &intersection, NULL ) != 1
             )
        {
            doNormal( agent );
            return true;
        }
    }
    else
    {
        if ( wm.ball().pos().x > rcsc::ServerParam::i().theirPenaltyAreaLineX() + 7.0
             && wm.ball().pos().absY() < rcsc::ServerParam::i().goalAreaHalfWidth() + 2.0 )
        {
            doNormal( agent );
            return true;
        }

        intersection.x = rcsc::ServerParam::i().theirPenaltyAreaLineX() - 0.76;
        intersection.y = wm.ball().pos().y;
    }


    double min_dist = 100.0;
    wm.getTeammateNearestTo( intersection, 10, &min_dist );
    if ( min_dist < wm.self().pos().dist( intersection ) )
    {
        doNormal( agent );
        return true;
    }

    double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );

    if ( intersection.x < rcsc::ServerParam::i().theirPenaltyAreaLineX()
         && wm.self().pos().x > rcsc::ServerParam::i().theirPenaltyAreaLineX() - 0.5 )
    {
        intersection.y = rcsc::ServerParam::i().penaltyAreaHalfWidth() - 0.5;
        if ( wm.self().pos().y < 0.0 ) intersection.y *= -1.0;
    }
    else if ( intersection.y > rcsc::ServerParam::i().penaltyAreaHalfWidth()
              && wm.self().pos().absY() < rcsc::ServerParam::i().penaltyAreaHalfWidth() + 0.5 )
    {
        intersection.y = rcsc::ServerParam::i().penaltyAreaHalfWidth() + 0.5;
        if ( wm.self().pos().y < 0.0 ) intersection.y *= -1.0;
    }


    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

	//std::cout<<wm.self().unum()<<" going1 => "<<intersection.x<<" , "<<intersection.y<<std::endl;
    if ( ! Body_GoToPoint( intersection, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Body_TurnToBall().execute( agent );
    }
    agent->setNeckAction( new rcsc::Neck_ScanField() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_TheirGoalKickMove::doNormal( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    
    if( Bhv_Mark( "mark" , M_home_pos ).execute( agent ) )
		return;
    
    //double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() * 0.8 );

    rcsc::Vector2D move_pos = M_home_pos;

    // attract to ball
    if ( move_pos.x > 25.0
    && ( move_pos.y * wm.ball().pos().y < 0.0 || move_pos.absY() < 10.0 ) )
    {
        double ydiff = wm.ball().pos().y - move_pos.y;
        move_pos.y += ydiff * 0.4;
    }

    // check penalty area
    if ( wm.self().pos().x > rcsc::ServerParam::i().theirPenaltyAreaLineX()
         && M_home_pos.absY() < rcsc::ServerParam::i().penaltyAreaHalfWidth() )
    {
        move_pos.y = rcsc::ServerParam::i().penaltyAreaHalfWidth() + 2.0;
        if ( wm.self().pos().y < 0.0 ) move_pos.y *= -1.0;
    }
    
	if( move_pos.absY() < 20.0 )
		move_pos.x = std::min( move_pos.x , 35.0 );

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;
    //std::cout<<wm.self().unum()<<" going2 => "<<move_pos.x<<" , "<<move_pos.y<<std::endl;
    if ( ! Body_GoToPoint( move_pos, dist_thr, dash_power ).execute( agent ) )
    {
        // already there
        rcsc::Body_TurnToBall().execute( agent );
    }
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_TheirGoalKickMove::doChaseBall( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.ball().vel().r() < 0.2 )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();

    if ( self_min > 10 )
    {
        return false;
    }

    rcsc::Vector2D get_pos = wm.ball().inertiaPoint( self_min );

    const double pen_x = rcsc::ServerParam::i().theirPenaltyAreaLineX() - 1.0;
    const double pen_y = rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0;
    const rcsc::Rect2D their_penalty( rcsc::Vector2D( pen_x, -pen_y ),
                                      rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() + 1.0,
                                                    rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );
    if ( their_penalty.contains( get_pos ) )
    {
        return false;
    }

    if ( get_pos.x > pen_x
         && wm.self().pos().x < pen_x
         && wm.self().pos().absY() < pen_y - 0.5 )
    {
        return false;
    }

    Body_Intercept().execute( agent );
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    return true;
}

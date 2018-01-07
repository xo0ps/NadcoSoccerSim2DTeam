// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hiroki SHIMORA
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_goalie.h"
#include "bhv_goalie_free_kick.h"
#include "bhv_set_play.h"
#include "strategy.h"
#include "bhv_basic_tackle.h"
#include "bhv_danger_area_tackle.h"
#include "body_clear_ball.h"
#include "body_go_to_point.h"
#include "body_smart_kick.h"
#include "neck_goalie_turn_neck.h"
#include "body_intercept.h"
#include "body_pass.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>

#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_hold_ball.h>

#include <rcsc/geom.h>

int Bhv_Goalie::M_passMate = -1;
int Bhv_Goalie::M_neck_cnt = -1;

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_Goalie::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    static const rcsc::Rect2D our_penalty( rcsc::Vector2D( -sp.pitchHalfLength(),
                                                           -sp.penaltyAreaHalfWidth() + 1.0 ),
                                           rcsc::Size2D( sp.penaltyAreaLength() - 1.0,
                                                         sp.penaltyAreaWidth() - 2.0 ) );

    if( ( wm.gameMode().type() == rcsc::GameMode::FreeKick_ || wm.gameMode().type() == rcsc::GameMode::GoalieCatch_ )
        && wm.gameMode().side() == wm.ourSide() )
    {
        Bhv_GoalieFreeKick().execute( agent );
        return true;
    }
    else if( ( wm.gameMode().type() == rcsc::GameMode::BackPass_ || wm.gameMode().type() == rcsc::GameMode::IndFreeKick_ )
             && wm.gameMode().side() != wm.ourSide() )
    {
        rcsc::Vector2D home_pos = Strategy::i().getPosition( wm.self().unum() );
        Bhv_SetPlay( home_pos ).execute( agent );
        return true;
    }
    else if( wm.gameMode().type() == rcsc::GameMode::GoalKick_ )
    {
        doMove( agent );
        return true;
    }
    //////////////////////////////////////////////////////////////
    // play_on play
    static int ball_holder_unum = -2;
    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    for( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps.end(); it++ )
    {
		if( !(*it) ) continue;
        if( (*it)->pos().dist( wm.ball().pos() ) < (*it)->playerTypePtr()->kickableArea() )
            ball_holder_unum = (*it)->unum() + 11;
    }
    
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates.end(); it++ )
    {
		if( !(*it) ) continue;
        if( (*it)->pos().dist( wm.ball().pos() ) < (*it)->playerTypePtr()->kickableArea() )
            ball_holder_unum = (*it)->unum();
    }
    
    if( rcsc::Segment2D( wm.ball().pos(), wm.ball().inertiaFinalPoint() ).existIntersection( rcsc::Segment2D( rcsc::Vector2D( -52.5, -9.0 ), rcsc::Vector2D( -52.5, 9.0 ) ) ) )
        ball_holder_unum = 30;

    // catchable
    if ( wm.time().cycle() > wm.self().catchTime().cycle() + sp.catchBanCycle()
         && wm.ball().distFromSelf() < rcsc::ServerParam::i().catchableArea() - 0.05
         && our_penalty.contains( wm.ball().pos() )
         && ball_holder_unum > 11 
         && wm.lastKickerSide() != wm.self().side() )
    {
        agent->doCatch();
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    if ( wm.self().isKickable() )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}
/*-------------------------------------------------------------------*/
/*!
  kick action
*/
bool
Bhv_Goalie::doKick( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    rcsc::Vector2D target_point;
    //double pass_speed = 0.0;
    //int receiver = 0;
    
    std::vector< rcsc::PlayerObject* > def_line;
    std::vector< int > opp_num;

    //direct pass
    const rcsc::Rect2D penalty_area( rcsc::Vector2D( -sp.pitchHalfLength(), -sp.penaltyAreaHalfWidth() ),
                                     rcsc::Vector2D( -sp.pitchHalfLength() + sp.penaltyAreaLength(),
                                                     sp.penaltyAreaHalfWidth() ) );
    const double ball_decay = sp.ballDecay();
    const double ball_speed_max = sp.ballSpeedMax();
    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
    const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
    { 
		if( !(*it) ) continue;
        bool target_is_free = true;
        const rcsc::Sector2D sector( wm.ball().pos(),
                                     1.0, (*it)->pos().dist( wm.ball().pos() ) + 3.0,
                                     ( (*it)->pos() - wm.ball().pos() ).th() - rcsc::AngleDeg( 30.0 ),
                                     ( (*it)->pos() - wm.ball().pos() ).th() + rcsc::AngleDeg( 30.0 ) );
        for( rcsc::PlayerPtrCont::const_iterator itr = opps.begin(); itr != opps.end(); ++itr )
        {
			if( !(*itr) ) continue;
            if( sector.contains( (*itr)->pos() ) )
                target_is_free = false;
        }

        if( target_is_free
            && (*it)->posCount() < 3
            && (*it)->pos().x > -25.0
            && (*it)->pos().x > wm.self().pos().x - 5.0
            && (*it)->pos().dist( wm.self().pos() ) > 10.0
            && rcsc::AngleDeg( ( (*it)->pos() - wm.self().pos() ).th() ).abs() < 70.0 )
        {
            int reach_cycle = (int)rcsc::calc_length_geom_series( ball_speed_max, wm.self().pos().dist( (*it)->pos() ), ball_decay );
            if( reach_cycle < 0 || reach_cycle > 30 )
                reach_cycle = 30;
            double ball_speed = (*it)->playerTypePtr()->kickableArea() * 1.5 * pow( 1.0 / ball_decay, reach_cycle - 1 );
            double first_speed = std::min( ball_speed_max, ball_speed );
            if( penalty_area.contains( wm.ball().pos() ) )
            {
                if( (*it)->seenPosCount() == 0 )
                {
                    if( Body_SmartKick( (*it)->pos(), first_speed, first_speed * 0.92, 1 ).execute( agent ) )
                    {
						Body_Pass::say_pass( agent , (*it)->unum() , (*it)->pos() );
                    }
                    if( M_passMate != -1 )
                    {
                        M_neck_cnt--;
                        if( M_neck_cnt <= 0 )
                            M_passMate = -1;
                    }
                    else
                        agent->setNeckAction( new Neck_GoalieTurnNeck() );
                    
                }
                else if( M_passMate != -1 )
                {
                    rcsc::Body_HoldBall().execute( agent );
                    M_neck_cnt--;
                    if( M_neck_cnt <= 0 )
                        M_passMate = -1;
                }
                else
                {
                    rcsc::Body_HoldBall().execute( agent );
                    M_passMate = (*it)->unum();
                    M_neck_cnt = 3;
                }
                return true;
            }
            else
            {
                if( (*it)->seenPosCount() == 0 )
                {
                    Body_SmartKick( (*it)->pos(), first_speed, first_speed * 0.92, 3 ).execute( agent );
                    Body_Pass::say_pass( agent , (*it)->unum() , (*it)->pos() );

                    if( M_passMate != -1 )
                    {
                        M_neck_cnt--;
                        if( M_neck_cnt <= 0 )
                            M_passMate = -1;
                    }
                    else
                        agent->setNeckAction( new Neck_GoalieTurnNeck() );
                }
                else if( M_passMate != -1 )
                {
                    rcsc::Body_HoldBall().execute( agent );
                    M_neck_cnt--;
                    if( M_neck_cnt <= 0 )
                        M_passMate = -1;
                }
                else
                {
                    rcsc::Body_HoldBall().execute( agent );
                    M_passMate = (*it)->unum();
                    M_neck_cnt = 3;
                }
                return true;
            }
        }
    }
    
    double best_angle = 90.0 * wm.ball().pos().y / wm.ball().pos().absY();
    for( double angle = -90.0; angle <= 90.0; angle += 10.0 )
    {
        bool angle_safety = true;
        const rcsc::Sector2D sector( wm.ball().pos(),
                                     1.0, 100.0,
                                     rcsc::AngleDeg( angle ) - rcsc::AngleDeg( 20.0 ),
                                     rcsc::AngleDeg( angle ) + rcsc::AngleDeg( 20.0 ) );
        for( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end; ++it )
        {
			if( !(*it) ) continue;
            if( sector.contains( (*it)->pos() ) )
                angle_safety = false;
        }
        if( angle_safety && abs( angle ) < abs( best_angle ) )
            best_angle = angle;
    }
    
    target_point.x = wm.ball().pos().x + rcsc::AngleDeg( best_angle ).cos();
    target_point.y = wm.ball().pos().y + rcsc::AngleDeg( best_angle ).sin();
    if( Body_SmartKick( target_point, ball_speed_max, ball_speed_max * 0.92, 1 ).execute( agent ) )
    {
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
    Body_ClearBall().execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
    return true;
}
/*-------------------------------------------------------------------*/
/*!
  move action
*/
bool
Bhv_Goalie::doMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    //ball lost
    if( wm.ball().state().ghost_count_ > 0 )
    {
        agent->doTurn( 180.0 );
        rcsc::Neck_ScanField().execute( agent );
        return true;
    }

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( ! wm.self().isKickable()
     && !wm.existKickableTeammate()
     && !wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( mate_min, opp_min );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );

    //corner kick situation
    if( wm.gameMode().side() != wm.ourSide() && wm.gameMode().type() == rcsc::GameMode::CornerKick_ )
    {
        const rcsc::Vector2D move_point( -51.0, 6.0 * ( wm.ball().pos().y / wm.ball().pos().absY() ) );
        const double dash_power = dashPower( agent, move_point );
        if( ! Body_GoToPoint( move_point, dash_power , 0.5 ).execute( agent ) )
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }
    //kick in situation
    //offside situation
    else
    if( wm.gameMode().type() == rcsc::GameMode::KickIn_ || wm.gameMode().type() == rcsc::GameMode::OffSide_ )
    {
        basicMove( agent );
        return true;
    }
    //goal kick situation
    else
    if( wm.gameMode().side() == wm.ourSide() && wm.gameMode().type() == rcsc::GameMode::GoalKick_ )
    {
        const rcsc::Vector2D move_point( -50.0, 0.0 );
        const double dash_power = dashPower( agent, move_point );
        if( ! Body_GoToPoint( move_point, dash_power , 0.5 ).execute( agent ) )
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }
    //kick off situation
    else
    if( wm.gameMode().type() == rcsc::GameMode::KickOff_ )
    {
        const rcsc::Vector2D move_point( -40.0, 0.0 );
        const double dash_power = dashPower( agent, move_point );
        if( ! Body_GoToPoint( move_point, dash_power , 0.5 ).execute( agent ) )
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    const rcsc::Rect2D our_penalty( rcsc::Vector2D( -sp.pitchHalfLength(),
                                                    -sp.penaltyAreaHalfWidth() + 1.0 ),
                                    rcsc::Size2D( sp.penaltyAreaLength() - 1.0,
                                                  sp.penaltyAreaWidth() - 2.0 ) );
    //tackle
    if( our_penalty.contains( wm.ball().pos() )
        && !wm.existKickableTeammate()
        && Bhv_DangerAreaTackle( 0.5 ).execute( agent ) )
    {
        return true;
    }
    else
    if( Bhv_BasicTackle( 0.9, 80.0 ).execute( agent ) )
    {
        return true;
    }
    else
    if( wm.ball().pos().x < -35.0 && wm.ball().distFromSelf() < 3.0 )
    {
        if( Bhv_BasicTackle( 0.5, 80.0 ).execute( agent ) )
        {
            return true;
        }
    }
    else if( wm.ball().pos().x < -32.0 
             && ! wm.existKickableTeammate() 
             && std::abs( wm.ball().pos().y - wm.self().pos().y ) < 2.0 
             && wm.ball().distFromSelf() < 6.0 )
    {
        if( Bhv_BasicTackle( 0.5, 80.0 ).execute( agent ) )
        {
            return true;
        }
    }

    //shoot situation
    if( rcsc::Segment2D( wm.ball().pos(), wm.ball().inertiaFinalPoint() ).existIntersection( rcsc::Segment2D( rcsc::Vector2D( -52.5, -9.0 ), rcsc::Vector2D( -52.5, 9.0 ) ) ) )
    {
        if( wm.ball().inertiaPoint( self_min ).x > -52.0 )
        {
            chaseMove( agent );
        }
        else
        {
            rcsc::Vector2D move_point = rcsc::Line2D( wm.ball().pos(), wm.ball().inertiaFinalPoint() ).intersection( rcsc::Line2D( rcsc::Vector2D( -52.5, -9.0 ), rcsc::Vector2D( -52.5, 9.0 ) ) );
            int cycle = 1;
            for( int i = 1; i < 15; i++ )
            {
                if( wm.ball().inertiaPoint( i ).dist( wm.self().pos() ) < move_point.dist( wm.self().pos() ) )
                {
                    move_point = wm.ball().inertiaPoint( i );
                    cycle = i;
                }
                else
                {
                    break;
                }
            }
            Body_GoToPoint( move_point, sp.maxDashPower() , 0.5 ).execute( agent );
            agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        }
    }
    else if( dangerSituation( agent ) )
    {
        chaseMove( agent );
    }
    else if( self_min <= opp_min
             && self_min < mate_min
             && base_pos.x > -sp.pitchHalfLength()
             && base_pos.absY() < sp.pitchHalfWidth() )
    {
        chaseMove( agent );
    }
    else
    {
        basicMove( agent );
    }
    return true;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_Goalie::chaseMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( ! wm.self().isKickable()
     && !wm.existKickableTeammate()
     && !wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( self_min, std::min( mate_min, opp_min ) );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );

    if( self_min < mate_min && self_min <= opp_min )
    {
        Body_Intercept( false ).execute( agent );
    }
    else
    if( wm.existKickableOpponent() && self_min < 3 )
    {
        rcsc::Vector2D opp_pos = wm.interceptTable()->fastestOpponent()->pos();
        Body_GoToPoint( opp_pos, sp.maxDashPower(), 0.5 ).execute( agent );
    }
    else
    {
        rcsc::Vector2D goal_center = centroid( agent, NULL, NULL, NULL, NULL );
        rcsc::Line2D base_line( base_pos, goal_center );
        rcsc::Vector2D move_point = base_line.projection( wm.self().pos() );
        if( base_line.dist( wm.self().pos() ) < 1.0 )
        {
            Body_Intercept( false ).execute( agent );
        }
        else
        {
            if( move_point.x < -51.5 )
            {
                move_point.x = -51.5;
                move_point.y = base_line.getY( -51.5 );
            }
            else if( move_point.x > base_pos.x )
            {
                move_point = base_pos;
            }
            const double dash_power = dashPower( agent, move_point );
            double dist_thr = move_point.dist( wm.self().pos() ) * 0.3;
            if ( dist_thr < 0.5 ) dist_thr = 0.5;
            Body_GoToPoint( move_point, dash_power, dist_thr ).execute( agent );
        }
    }
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );

    return true;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_Goalie::basicMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( ! wm.self().isKickable()
     && ! wm.existKickableTeammate()
     && ! wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( self_min, std::min( mate_min, opp_min ) );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );

    rcsc::Vector2D left_side;//( -sp.pitchHalfLength(), -7.0 );
    rcsc::Vector2D right_side;//( -sp.pitchHalfLength(), 7.0 );
    const rcsc::Vector2D center = centroid( agent, &left_side, &right_side, NULL, NULL );
    const rcsc::Vector2D static_point( -51.5, center.y );
    const rcsc::Line2D line_base_to_center( base_pos, center );
    const rcsc::AngleDeg angle_base_to_center = ( base_pos - center ).th();
    rcsc::Vector2D move_point = static_point;

    const double min_opp_x = wm.offsideLineX();
    double ball_factor = ( min_opp_x + sp.pitchHalfLength() ) * 0.5;
    if( base_pos.x < wm.ourDefenseLineX() + 1.0 || base_pos.x < -30.0 )
    {
        ball_factor = ( base_pos.x + sp.pitchHalfLength() ) * 0.3;
    }
    move_point.x = static_point.x + angle_base_to_center.cos() * ball_factor;
    move_point.y = static_point.y + angle_base_to_center.sin() * ball_factor;

    if( move_point.dist( static_point ) > 25.0 )
    {
        move_point.x = static_point.x + angle_base_to_center.cos() * 25.0;
        move_point.y = static_point.y + angle_base_to_center.sin() * 25.0;
    }

    const rcsc::Rect2D our_goal( rcsc::Vector2D( -sp.pitchHalfLength(),
                                                 -sp.goalAreaHalfWidth() ),
                                 rcsc::Size2D( sp.goalAreaLength(),
                                               sp.goalAreaWidth() ) );
    if( our_goal.contains( move_point ) )
    {
        const rcsc::AngleDeg left_angle = ( rcsc::Vector2D( -47.0, -9.0 ) - center ).th();
        const rcsc::AngleDeg right_angle = ( rcsc::Vector2D( -47.0, 9.0 ) - center ).th();
        if( angle_base_to_center.degree() > left_angle.degree() && angle_base_to_center.degree() < right_angle.degree() )
        {
            move_point.x = -47.0;
            move_point.y = line_base_to_center.getY( -47.0 );
        }
        else
        {
            move_point.x = std::max( line_base_to_center.getX( 9.0 * ( base_pos.y / base_pos.absY() ) ), -52.0 );
            move_point.y = 9.0 * ( base_pos.y / base_pos.absY() );
        }
    }
    if( wm.self().inertiaFinalPoint().dist( move_point ) > 0.5
        && move_point.dist( static_point ) < wm.self().pos().dist( static_point )
        && wm.self().pos().dist( move_point ) < 5.0 )
    {
        const rcsc::AngleDeg angle_to_point = ( wm.self().pos() - move_point ).th();
        const rcsc::AngleDeg angle = 180.0 - ( wm.self().body() - angle_to_point ).degree();
        agent->doDash( sp.maxDashPower(), angle );
    }
    else
    {
        const double dash_power = dashPower( agent, move_point );
        double dist_thr = move_point.dist( wm.self().pos() ) * 0.3;
        if ( dist_thr < 0.5 ) dist_thr = 0.5;
        if( ! Body_GoToPoint( move_point, dash_power, dist_thr ).execute( agent ) )
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
    }
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    return true;
}
/*-------------------------------------------------------------------*/
/*!

 */
rcsc::Vector2D
Bhv_Goalie::centroid( rcsc::PlayerAgent * agent, rcsc::Vector2D * left_goal_side, rcsc::Vector2D * right_goal_side, int * left_cycle, int * right_cycle )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( !wm.self().isKickable()
        && !wm.existKickableTeammate()
        && !wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( self_min, std::min( mate_min, opp_min ) );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );
    rcsc::Vector2D left_side( -sp.pitchHalfLength(), -7.0 );
    rcsc::Vector2D right_side( -sp.pitchHalfLength(), 7.0 );

    rcsc::Triangle2D left_triangle( base_pos, left_side, sp.ourTeamGoalPos() );
    rcsc::Triangle2D right_triangle( base_pos, right_side, sp.ourTeamGoalPos() );
    const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates.end(); ++it )
    {
		if( !(*it) ) continue;
        if( left_triangle.contains( (*it)->pos() ) )
        {
            rcsc::Line2D left_line( base_pos, (*it)->pos() );
            left_side = rcsc::Vector2D( -sp.pitchHalfLength(), left_line.getY( -sp.pitchHalfLength() ) + 1.0 );
            left_triangle = rcsc::Triangle2D( base_pos, left_side, sp.ourTeamGoalPos() );
        }
        else if( right_triangle.contains( (*it)->pos() ) )
        {
            rcsc::Line2D right_line( base_pos, (*it)->pos() );
            right_side = rcsc::Vector2D( -sp.pitchHalfLength(), right_line.getY( -sp.pitchHalfLength() ) - 1.0 );
            right_triangle = rcsc::Triangle2D( base_pos, right_side, sp.ourTeamGoalPos() );
        }
    }
    if( left_goal_side )
        *left_goal_side = left_side;
    if( right_goal_side )
        *right_goal_side = right_side;

    rcsc::Vector2D centroid = sp.ourTeamGoalPos();

    double cycle_ball_to_left_side = (int)rcsc::calc_length_geom_series( sp.ballSpeedMax(),
                                                                         base_pos.dist( left_side ),
                                                                         sp.ballDecay() );
    if( cycle_ball_to_left_side < 1 || cycle_ball_to_left_side > 30 )
        cycle_ball_to_left_side = 30;

    if( left_cycle )
        *left_cycle = cycle_ball_to_left_side;

    double cycle_ball_to_right_side = (int)rcsc::calc_length_geom_series( sp.ballSpeedMax(),
                                                                          base_pos.dist( right_side ),
                                                                          sp.ballDecay() );
    if( cycle_ball_to_right_side < 1 || cycle_ball_to_right_side > 30 )
        cycle_ball_to_right_side = 30;

    if( right_cycle )
        *right_cycle = cycle_ball_to_right_side;

    centroid.y = sp.goalWidth() * 0.5 - ( sp.goalWidth() * ( cycle_ball_to_right_side ) 
                                          / ( cycle_ball_to_left_side + cycle_ball_to_right_side ) );

    return centroid;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_Goalie::dangerSituation( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( !wm.existKickableTeammate()
        && !wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( mate_min, opp_min );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );

    if( self_min < opp_min || mate_min < opp_min )
    {
        //opps don't control ball
        return false;
    }

    if( base_pos.absY() > 15.0 )
    {
        //safety angle
        return false;
    }
    const rcsc::Triangle2D goal_triangle( base_pos, rcsc::Vector2D( -52.0, -6.0 ), rcsc::Vector2D( -52.5, 6.0 ) );
    const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates.end(); it++ )
    {
		if( !(*it) ) continue;
        if( goal_triangle.contains( (*it)->pos() ) )
        {
            //opps far from our goal
            return false;
        }
    }
    if( mate_min < self_min )
    {
        //exit defense mate
        return false;
    }

    const rcsc::AngleDeg left_angle = ( rcsc::Vector2D( -52.0, -7.0 ) - base_pos ).th();
    const rcsc::AngleDeg right_angle = ( rcsc::Vector2D( -52.0, 7.0 ) - base_pos ).th();
    bool is_left_contained = false;
    bool is_right_contained = false;
    for( int i = 0; i < 30; i++ )
    {
        const double dash_r = wm.self().playerTypePtr()->dashDistanceTable().at( i );
        const double ball_r = rcsc::calc_sum_geom_series( sp.ballSpeedMax(), sp.ballDecay(), i + 1 );
        const rcsc::Vector2D left_ball_pos( base_pos.x + left_angle.cos() * ball_r,
                                            base_pos.y + left_angle.sin() * ball_r );
        const rcsc::Vector2D right_ball_pos( base_pos.x + right_angle.cos() * ball_r,
                                             base_pos.y + right_angle.cos() * ball_r );
        const rcsc::Circle2D my_circle( wm.self().pos(), dash_r );
        if( my_circle.contains( left_ball_pos ) )
        {
            is_left_contained = true;
        }
        if( my_circle.contains( right_ball_pos ) )
        {
            is_right_contained = true;
        }

        if( left_ball_pos.x < -52.5 && !is_left_contained )
        {
            return true;
        }
        if( right_ball_pos.x < -52.5 && !is_right_contained )
        {
            return true;
        }
    }
    return false;
}
/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_Goalie::dashPower( rcsc::PlayerAgent * agent, const rcsc::Vector2D & target_point )
{    
	static bool s_recover_mode = false;
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int ball_reach_step = 0;
    if( !wm.existKickableTeammate()
        && !wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( wm.interceptTable()->teammateReachCycle(),
                                    wm.interceptTable()->opponentReachCycle() );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );
    
    if ( wm.self().stamina() < SP.staminaMax() * 0.4 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > SP.staminaMax() * 0.6 )
    {
        s_recover_mode = false;
    }

    /*--------------------------------------------------------*/
    double dash_power = SP.maxDashPower();
    const double my_inc = wm.self().playerType().staminaIncMax() * wm.self().recovery();

    if( wm.gameMode().type() != rcsc::GameMode::PlayOn )
    {
        dash_power = my_inc * 0.8;
    }
    else if ( ( ( base_pos.x < wm.ourDefenseLineX() )
                || ( base_pos.x < wm.self().pos().x ) )
              && ! wm.existKickableTeammate() )
    {
        // keep max power
        dash_power = SP.maxDashPower();
    }
    else if( self_min <= mate_min
             && self_min <= opp_min )
    {
        dash_power = SP.maxDashPower();
    }
    else if ( base_pos.x < -30.0
              && opp_min <= mate_min
              && opp_min <= self_min )
    {
        dash_power = rcsc::ServerParam::i().maxDashPower();
    }
    else if( ( target_point.x < wm.self().pos().x
               || target_point.dist( SP.ourTeamGoalPos() ) < wm.self().pos().dist( SP.ourTeamGoalPos() ) )
             && opp_min <= mate_min
             && opp_min <= self_min )
    {
        dash_power = rcsc::ServerParam::i().maxDashPower();
    }
    else if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 ) dash_power = 0.0;

    }
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() > 15.0 )
    {
        dash_power = std::min( my_inc - 20.0,
                               rcsc::ServerParam::i().maxPower() * 0.5 );
    }
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               rcsc::ServerParam::i().maxDashPower() );
    }
    else if( wm.self().pos().dist( target_point ) > 15.0
             && wm.self().stamina() >  SP.staminaMax() * 0.7 )
    {
        dash_power = SP.maxDashPower();
    }
    //nomal
    else
    {
        dash_power = std::min( my_inc * 1.3,
                               rcsc::ServerParam::i().maxPower() * 0.75 );
    }
    return dash_power;
}

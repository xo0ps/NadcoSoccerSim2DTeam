// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Opuci2010 Team
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

#include "bhv_goalie_basic_move.h"

#include "neck_goalie_turn_neck.h"
#include "bhv_danger_area_tackle.h"
#include "body_go_to_point.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::Vector2D move_point = getTargetPoint( agent );

    //////////////////////////////////////////////////////////////////
    // tackle
    if ( Bhv_DangerAreaTackle().execute( agent ) )
    {
        return true;
    }

    /////////////////////////////////////////////////////////////
    //----------------------------------------------------------
    if ( doEmergentAdvance( agent, move_point ) )
    {
        // execute intercept
        return true;
    }
    //----------------------------------------------------------
    if ( doPrepareDeepCross( agent, move_point ) )
    {
        // face to opponent side to wait the opponent last cross pass
        return true;
    }
    //----------------------------------------------------------
    // check distance to the move target point
    // if already there, try to stop
    if ( doStopAtMovePoint( agent, move_point ) )
    {
        // execute stop action
        return true;
    }
    //----------------------------------------------------------
    // check whether ball is in very dangerous state
    if ( doMoveForDangerousState( agent, move_point ) )
    {
        // execute emergency action
        return true;
    }
    //----------------------------------------------------------
    // check & correct X difference
    if ( doCorrectX( agent, move_point ) )
    {
        // execute x-pos adjustment action
        return true;
    }

    //----------------------------------------------------------
    if ( doCorrectBodyDir( agent, move_point, true ) ) // consider opp
    {
        // exeucte turn
        return true;
    }

    //----------------------------------------------------------
    if ( doGoToMovePoint( agent, move_point ) )
    {
        // mainly execute Y-adjustment if body direction is OK. -> only dash
        // if body direction is not good, nomal go to action is done.
        return true;
    }

    //----------------------------------------------------------
    // change my body angle to desired angle
    if ( doCorrectBodyDir( agent, move_point, false ) ) // not consider opp
    {
        return true;
    }

    agent->doTurn( 0.0 );
    agent->setNeckAction( new Neck_GoalieTurnNeck() );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Vector2D
Bhv_GoalieBasicMove::getTargetPoint( rcsc::PlayerAgent * agent )
{
    const double base_move_x = -49.8;
    //const double danger_move_x = -51.5;
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    int ball_reach_step = 0;
    if ( ! wm.existKickableTeammate()
         && ! wm.existKickableOpponent() )
    {
        ball_reach_step
            = std::min( wm.interceptTable()->teammateReachCycle(),
                        wm.interceptTable()->opponentReachCycle() );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );

    //---------------------------------------------------------//
    // kickable opponent is in penalty area
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    rcsc::Rect2D penalty_area( rcsc::Vector2D( -sp.pitchHalfLength(), 
					       -sp.penaltyAreaHalfWidth() ),
                               rcsc::Vector2D( -sp.pitchHalfLength() 
					       + sp.penaltyAreaLength(),
                                               sp.penaltyAreaHalfWidth() ) );
    rcsc::Rect2D goal_area( rcsc::Vector2D( -sp.pitchHalfLength(), 
					    -sp.goalAreaHalfWidth() ),
                            rcsc::Vector2D( -sp.pitchHalfLength() 
					    + sp.goalAreaHalfWidth() + 2.0,
                                            sp.goalAreaHalfWidth() ) );

    if( ( penalty_area.contains( base_pos )
          || base_pos.x < -sp.pitchHalfLength()
          || sp.ourTeamGoalPos().dist( base_pos ) < 20.0 )
        && opp_min <= self_min
        && opp_min <= mate_min )
    {
        rcsc::Vector2D right_pole( -sp.pitchHalfLength(), sp.goalHalfWidth() );
        rcsc::Vector2D left_pole( -sp.pitchHalfLength(), -sp.goalHalfWidth() );
        rcsc::Line2D right_shoot_line( base_pos, right_pole );
        rcsc::Line2D left_shoot_line( base_pos, left_pole );
        rcsc::Line2D center_shoot_line( base_pos, sp.ourTeamGoalPos() );
        const double ball_decay = sp.ballDecay();
        double ball_speed = sp.ballSpeedMax();
        double player_speed = wm.self().playerType().playerSpeedMax();

        const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
        const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();
        for( int y = -7; y <= 7; y++ )
        {
            for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
            {
				if( !(*it) ) continue;
                rcsc::Line2D shoot_line( base_pos, rcsc::Vector2D( -52.5, (double)y ) );
                if( (*it)->pos().x < base_pos.x - ball_speed
                    && shoot_line.dist( (*it)->pos() ) < (*it)->playerTypePtr()->kickableArea() * 0.8 )
                {
                    if( base_pos.y > 0.0 && (double)y > left_pole.y )
                    {
                        left_pole.y = (double)y;
                        left_shoot_line.assign( base_pos, left_pole );
                    }
                    else if( base_pos.y < 0.0 && (double)y < right_pole.y )
                    {
                        right_pole.y = (double)y;
                        right_shoot_line.assign( base_pos, right_pole );
                    }
                }
            }
        }
        rcsc::Line2D defend_line( rcsc::Vector2D( base_move_x, 0.0 ), rcsc::AngleDeg( 90.0 ) );
        rcsc::Vector2D right_danger_point( base_move_x, 7.0 );
        if( defend_line.intersection( right_shoot_line ).isValid() )
        {
            right_danger_point = defend_line.intersection( right_shoot_line );
        }
        rcsc::Vector2D left_danger_point( base_move_x, -7.0 );
        if( defend_line.intersection( left_shoot_line ).isValid() )
        {
            left_danger_point = defend_line.intersection( left_shoot_line );
        }
        int right_shoot_cycle = (int)rcsc::calc_length_geom_series( ball_speed,  ( base_pos - right_danger_point ).r(),  ball_decay );
        if( right_shoot_cycle < 0 )
        {
            right_shoot_cycle = 100;
        }
        int left_shoot_cycle = (int)rcsc::calc_length_geom_series( ball_speed, ( base_pos - left_danger_point ).r(),  ball_decay );
        if( left_shoot_cycle < 0 )
        {
            left_shoot_cycle = 100;
        }
        center_shoot_line.assign( base_pos, 
                                  rcsc::Vector2D( sp.ourTeamGoalPos().x, 
                                                  sp.goalHalfWidth() 
                                                  - ( sp.goalWidth() 
						      * base_pos.dist( right_danger_point ) 
                                                      / ( base_pos.dist( right_danger_point ) 
							  + base_pos.dist( left_danger_point ) ) ) ) );
        agent->debugClient().addLine( base_pos, 
				      center_shoot_line.intersection( rcsc::Line2D( rcsc::Vector2D( -52.5, 0.0 ), 
										    rcsc::AngleDeg( 90.0 ) ) ) );

        if( base_pos.x < base_move_x )
        {
            if( base_pos.y > 0.0 )
            {
                return rcsc::Vector2D( -51.5, 7.0 );
            }
            else
            {
                return rcsc::Vector2D( -51.5, -7.0 );
            }
        }
        if( base_pos.y > 0.0 )
        {
            left_danger_point = left_shoot_line.projection( wm.self().pos() );
            left_shoot_cycle = (int)rcsc::calc_length_geom_series( ball_speed, 
                                                                   ( base_pos - left_danger_point ).r(), 
                                                                   ball_decay );
            if( left_shoot_cycle < 0 )
            {
                left_shoot_cycle = 100;
            }
            double player_vel = wm.self().vel().y;
            const double player_accel = sp.maxDashPower() * wm.self().playerType().dashRate( wm.self().playerType().effortMax() );
            double right_reach_distance = 0.0;
            double left_reach_distance = 0.0;
            for( int i = 1; i < right_shoot_cycle; ++i )
            {
                player_vel += player_accel;
                player_vel = std::min( player_vel, player_speed );
                right_reach_distance += player_vel;
                player_vel *= wm.self().playerType().playerDecay();
            }
            player_vel = 0.0;
            for( int i = 3; i < left_shoot_cycle; ++i )
            {
                player_vel += player_accel;
                player_vel = std::min( player_vel, player_speed );
                left_reach_distance += player_vel;
                player_vel *= wm.self().playerType().playerDecay();
            }

            rcsc::Vector2D mate_nearest_ball( 0.0, 0.0 );
            for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
            {
				if( !(*it) ) continue;
                if( (*it)->pos().dist( wm.ball().pos() ) < mate_nearest_ball.dist( wm.ball().pos() ) )
                {
                    mate_nearest_ball = (*it)->pos();
                }
            }
            if( right_reach_distance + left_reach_distance + sp.catchableArea() * 1.6
                > wm.self().pos().dist( right_danger_point ) + wm.self().pos().dist( left_danger_point )
                || ( mate_nearest_ball.dist( wm.ball().pos() ) < 3.0
                     && mate_nearest_ball.x < wm.ball().pos().x
                     && left_shoot_line.dist( mate_nearest_ball ) < 1.0 ) )
            {
                rcsc::Vector2D move_pos( right_danger_point.x, right_danger_point.y - right_reach_distance );
                if( move_pos.x < -sp.pitchHalfLength() )
                {
                    move_pos.x = -sp.pitchHalfLength()  + sp.catchableArea() * 0.8;
                }
                if( move_pos.y > right_pole.y )
                {
                    move_pos.y = right_pole.y;
                }
                else if( move_pos.y < 0.0 )
                {
                    move_pos.y = 0.0;
                }
                return move_pos;
            }
            else
            {
                rcsc::Vector2D projection = center_shoot_line.projection( wm.self().pos() );
                int self_reach_cycle 
                    = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( projection ) );
                if( self_reach_cycle <= ball_reach_step
                    || wm.self().pos().dist( projection ) < sp.catchableArea() * 0.5 )
                {
                    if ( Bhv_DangerAreaTackle( 0.5 ).execute( agent ) )
                    {
                        return wm.self().pos();
                    }
                    rcsc::Vector2D move_pos( base_pos );
                    return move_pos;
                }
                else
                {
                    if( projection.x < -sp.pitchHalfLength() )
                    {
                        rcsc::Line2D dead_line( rcsc::Vector2D( -sp.pitchHalfLength() 
								+ sp.catchableArea(), 
								0.0 ),
                                                rcsc::AngleDeg( 90.0 ) );
                        return center_shoot_line.intersection( dead_line );
                    }
                    else
                    {
                        return projection;
                    }
                }
            }
            
        }
        else
        {
            right_danger_point = right_shoot_line.projection( wm.self().pos() );
            int right_shoot_cycle = (int)rcsc::calc_length_geom_series( ball_speed, ( base_pos - right_danger_point ).r(), ball_decay );
            if( right_shoot_cycle < 0 )
            {
                right_shoot_cycle = 100;
            }
            double player_vel = -wm.self().vel().y;
            double player_accel = 100.0 * wm.self().playerType().dashRate( wm.self().playerType().effortMax() );
            double right_reach_distance = 0.0;
            double left_reach_distance = 0.0;
            for( int i = 1; i < left_shoot_cycle; ++i )
            {
                player_vel += player_accel;
                player_vel = std::min( player_vel, player_speed );
                left_reach_distance += player_vel;
                player_vel *= wm.self().playerType().playerDecay();
            }
            player_vel = 0.0;
            for( int i = 3; i < right_shoot_cycle; ++i )
            {
                player_vel += player_accel;
                player_vel = std::min( player_vel, player_speed );
                right_reach_distance += player_vel;
                player_vel *= wm.self().playerType().playerDecay();
            }

            rcsc::Vector2D mate_nearest_ball( 0.0, 0.0 );
            for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
            {
				if( !(*it) ) continue;
                if( (*it)->pos().dist( wm.ball().pos() ) < mate_nearest_ball.dist( wm.ball().pos() ) )
                {
                    mate_nearest_ball = (*it)->pos();
                }
            }
            if( right_reach_distance + left_reach_distance + sp.catchableArea() * 1.6
                > wm.self().pos().dist( right_danger_point ) + wm.self().pos().dist( left_danger_point )
                || ( mate_nearest_ball.dist( wm.ball().pos() ) < 3.0
                     && mate_nearest_ball.x < wm.ball().pos().x
                     && left_shoot_line.dist( mate_nearest_ball ) < 1.0 ) )
            {
                rcsc::Vector2D move_pos( left_danger_point.x, left_danger_point.y + left_reach_distance );
                if( move_pos.x < -sp.pitchHalfLength() )
                {
                    move_pos.x = -sp.pitchHalfLength() + sp.catchableArea() * 0.8;
                }
                if( move_pos.y < left_pole.y )
                {
                    move_pos.y = left_pole.y;
                }
                else if( move_pos.y > 0.0 )
                {
                    move_pos.y = 0.0;
                }
                return move_pos;
            }
            else
            {
                rcsc::Vector2D projection = center_shoot_line.projection( wm.self().pos() );
                int self_reach_cycle = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( projection ) );
                if( self_reach_cycle <= ball_reach_step || wm.self().pos().dist( projection ) < sp.catchableArea() * 0.5 )
                {
                    if ( Bhv_DangerAreaTackle( 0.5 ).execute( agent ) )
                    {
                        return wm.self().pos();
                    }
                    rcsc::Vector2D move_pos( base_pos );
                    return move_pos;
                }
                else
                {
                    if( projection.x < -sp.pitchHalfLength() )
                    {
                        rcsc::Line2D dead_line( rcsc::Vector2D( -sp.pitchHalfLength() + sp.catchableArea(),  0.0 ), rcsc::AngleDeg( 90.0 ) );
                        return center_shoot_line.intersection( dead_line );
                    }
                    else
                    {
                        return projection;
                    }
                }
            }

        }
    }

    //---------------------------------------------------------//
    // angle is very dangerous
    if ( base_pos.y > sp.goalHalfWidth() + 3.0 )
    {
        rcsc::Vector2D right_pole( - sp.pitchHalfLength(),
                                   sp.goalHalfWidth() );
        rcsc::AngleDeg angle_to_pole = ( right_pole - base_pos ).th();

        if ( -140.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < -90.0 )
        {
            //return rcsc::Vector2D( danger_move_x, sp.goalHalfWidth() + 0.001 );
            return rcsc::Vector2D( base_move_x, sp.goalHalfWidth() + 0.001 );
        }
    }
    else if ( base_pos.y < -sp.goalHalfWidth() - 3.0 )
    {
        rcsc::Vector2D left_pole( - sp.pitchHalfLength(),
                                  - sp.goalHalfWidth() );
        rcsc::AngleDeg angle_to_pole = ( left_pole - base_pos ).th();

        if ( 90.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < 140.0 )
        {
            //return rcsc::Vector2D( danger_move_x, - sp.goalHalfWidth() - 0.001 );
            return rcsc::Vector2D( base_move_x, - sp.goalHalfWidth() - 0.001 );
        }
    }

    //---------------------------------------------------------//
    // ball is close to goal line
    if ( base_pos.x < -sp.pitchHalfLength() + 8.0
         && base_pos.absY() > sp.goalHalfWidth() + 2.0 )
    {
        rcsc::Vector2D target_point( base_move_x, sp.goalHalfWidth() - 0.1 );
        if ( base_pos.y < 0.0 )
        {
            target_point.y *= -1.0;
        }

        return target_point;
    }

    //---------------------------------------------------------//
    {
        const double x_back = 7.0; // tune this!!
        int ball_pred_cycle = 5; // tune this!!
        const double y_buf = 0.5; // tune this!!
        const rcsc::Vector2D base_point( - sp.pitchHalfLength() - x_back, 0.0 );
        rcsc::Vector2D ball_point;
        if ( wm.existKickableOpponent() )
        {
            ball_point = base_pos;
        }
        else
        {
            int opp_min = wm.interceptTable()->opponentReachCycle();
            if ( opp_min < ball_pred_cycle )
            {
                ball_pred_cycle = opp_min;
            }

            ball_point = rcsc::inertia_n_step_point( base_pos, wm.ball().vel(), ball_pred_cycle, sp.ballDecay() );
        }

        if ( ball_point.x < base_point.x + 0.1 )
        {
            ball_point.x = base_point.x + 0.1;
        }

        rcsc::Line2D ball_line( ball_point, base_point );
        double move_y = ball_line.getY( base_move_x );

        if ( move_y > sp.goalHalfWidth() - y_buf )
        {
            move_y = sp.goalHalfWidth() - y_buf;
        }
        if ( move_y < - sp.goalHalfWidth() + y_buf )
        {
            move_y = - sp.goalHalfWidth() + y_buf;
        }

        return rcsc::Vector2D( base_move_x, move_y );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
double
Bhv_GoalieBasicMove::getBasicDashPower( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::PlayerType & mytype = wm.self().playerType();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    
    const double my_inc = mytype.staminaIncMax() * wm.self().recovery();

    if ( std::fabs( wm.self().pos().x - move_point.x ) > 3.0 )
    {
        return sp.maxDashPower();
    }

    if ( wm.ball().pos().x > -30.0 )
    {
        if ( wm.self().stamina() < sp.staminaMax() * 0.9 )
        {
            return my_inc * 0.5;
        }
        return my_inc;
    }
    else if ( wm.ball().pos().x > sp.ourPenaltyAreaLineX() )
    {
        if ( wm.ball().pos().absY() > 20.0 )
        {
            // penalty area
            return my_inc;
        }
        if ( wm.ball().vel().x > 1.0 )
        {
            // ball is moving to opponent side
            return my_inc * 0.5;
        }

        int opp_min = wm.interceptTable()->opponentReachCycle();
        if ( opp_min <= 3 )
        {
            return sp.maxDashPower();
        }

        if ( wm.self().stamina() < sp.staminaMax() * 0.7 )
        {
            return my_inc * 0.7;
        }
        return sp.maxDashPower() * 0.6;
    }
    else
    {
        if ( wm.ball().pos().absY() < 15.0 || wm.ball().pos().y * wm.self().pos().y < 0.0 ) // opposite side
        {
            return sp.maxDashPower();
        }
        else
        {
            return my_inc;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doEmergentAdvance( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    // not emergent
    if( move_point.x < -rcsc::ServerParam::i().pitchHalfLength() )
    {
        return false;
    }
    Body_GoToPoint( move_point, 0.5, rcsc::ServerParam::i().maxDashPower(), 1 ).execute( agent );
    if( agent->world().existKickableOpponent() )
    {
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    else if( agent->world().ball().posCount() < 1 )
    {
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    else
    {
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doPrepareDeepCross( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    if ( move_point.absY() < sp.goalHalfWidth() - 0.8 )
    {
        // consider only very deep cross
        return false;
    }

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D goal_c( - sp.pitchHalfLength(), 0.0 );

    rcsc::Vector2D goal_to_ball = wm.ball().pos() - goal_c;

    if ( goal_to_ball.th().abs() < 60.0 )
    {
        // ball is not in side cross area
        return false;
    }

    rcsc::Vector2D my_inertia = wm.self().inertiaFinalPoint();
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;
    //double dist_thr = 0.5;

    if ( my_inertia.dist( move_point ) > dist_thr )
    {
        // needed to go to move target point
        double dash_power = getBasicDashPower( agent, move_point );
        doGoToPointLookBall( agent, move_point, wm.ball().angleFromSelf(), dist_thr, dash_power );
        return true;
    }

    rcsc::AngleDeg body_angle = ( wm.ball().pos().y < 0.0 ? 10.0 : -10.0 );
    rcsc::Body_TurnToAngle( body_angle ).execute( agent );
    agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doStopAtMovePoint( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    //----------------------------------------------------------
    // already exist at target point
    // but inertia movement is big
    // stop dash

    const rcsc::WorldModel & wm = agent->world();
    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    // now, in the target area
    if ( wm.self().pos().dist( move_point ) < dist_thr )
    {
        const rcsc::Vector2D my_final = rcsc::inertia_final_point( wm.self().pos(), wm.self().vel(), wm.self().playerType().playerDecay() );
        // after inertia move, can stay in the target area
        if ( my_final.dist( move_point ) < dist_thr )
        {
            return false;
        }

        // try to stop at the current point
        rcsc::Body_StopDash( true ).execute( agent ); // save recovery
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doMoveForDangerousState( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    const rcsc::WorldModel& wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    const double x_buf = 0.5;

    const rcsc::Vector2D ball_next = wm.ball().pos() + wm.ball().vel();

    if ( std::fabs( move_point.x - wm.self().pos().x ) > x_buf
         && ball_next.x < -sp.pitchHalfLength() + 11.0
         && ball_next.absY() < sp.goalHalfWidth() + 1.0 )
    {
        // x difference to the move point is over threshold
        // but ball is in very dangerous area (just front of our goal)

        // and, exist opponent close to ball
        if ( ! wm.opponentsFromBall().empty() && wm.opponentsFromBall().front()->distFromBall() < 2.0 )
        {
            rcsc::Vector2D block_point = wm.opponentsFromBall().front()->pos();
            block_point.x -= 2.5;
            block_point.y = move_point.y;

            if ( wm.self().pos().x < block_point.x )
            {
                block_point.x = wm.self().pos().x;
            }

            if ( doGoToMovePoint( agent, block_point ) )
            {
                return true;
            }

            double dist_thr = wm.ball().distFromSelf() * 0.1;
            if ( dist_thr < 0.5 ) dist_thr = 0.5;

            doGoToPointLookBall( agent, move_point, wm.ball().angleFromSelf(), dist_thr, sp.maxDashPower() );
            agent->setNeckAction( new Neck_GoalieTurnNeck() );
            return true;
        }
    }
	agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doCorrectX( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    const rcsc::WorldModel & wm = agent->world();

    const double x_buf = 0.5;

    if ( std::fabs( move_point.x - wm.self().pos().x ) < x_buf )
    {
        // x difference is already small.
        return false;
    }

    int opp_min_cyc = wm.interceptTable()->opponentReachCycle();
    if ( ( ! wm.existKickableOpponent() && opp_min_cyc >= 4 )
         || wm.ball().distFromSelf() > 18.0 )
    {
        double dash_power = getBasicDashPower( agent, move_point );

        if ( ! wm.existKickableOpponent() && wm.ball().distFromSelf() > 30.0 )
        {
            if ( ! Body_GoToPoint( move_point, x_buf, dash_power ).execute( agent ) )
            {
                rcsc::AngleDeg body_angle = ( wm.self().body().degree() > 0.0 ? 90.0 : -90.0 );
                rcsc::Body_TurnToAngle( body_angle ).execute( agent );

            }
            agent->setNeckAction( new Neck_GoalieTurnNeck() );
            return true;
        }

//         rcsc::Bhv_GoToPointLookBall( move_point, x_buf, dash_power
//                                      ).execute( agent );
        doGoToPointLookBall( agent, move_point, wm.ball().angleFromSelf(), x_buf, dash_power );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
	agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doCorrectBodyDir( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point, const bool consider_opp )
{
    // adjust only body direction

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    
    int ball_reach_step = 0;
    if ( ! wm.existKickableTeammate()
         && ! wm.existKickableOpponent() )
    {
        ball_reach_step = std::min( wm.interceptTable()->teammateReachCycle(), wm.interceptTable()->opponentReachCycle() );
    }
    const rcsc::Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );
    const rcsc::Vector2D ball_next = base_pos; //wm.ball().pos() + wm.ball().vel();

    const rcsc::AngleDeg target_angle = ( ball_next.y < 0.0 ? -90.0 : 90.0 );
    const double angle_diff = ( wm.self().body() - target_angle ).abs();

    if ( angle_diff < 5.0 )
    {
        return false;
    }

#if 1
    {
        const rcsc::Vector2D goal_c( - sp.pitchHalfLength(), 0.0 );
        rcsc::Vector2D goal_to_ball = wm.ball().pos() - goal_c;
        if ( goal_to_ball.th().abs() >= 60.0 )
        {
            return false;
        }
    }
#else
    if ( wm.ball().pos().x < -36.0
         && wm.ball().pos().absY() < 15.0
         && wm.self().pos().dist( move_point ) > 1.5 )
    {
        return false;
    }
#endif

    double opp_ball_dist = ( wm.opponentsFromBall().empty() ? 100.0 : wm.opponentsFromBall().front()->distFromBall() );
    if ( ! consider_opp
         || opp_ball_dist > 7.0
         || wm.ball().distFromSelf() > 20.0
         || ( std::fabs( move_point.y - wm.self().pos().y ) < 1.0 // y diff
              && ! wm.existKickableOpponent() ) )
    {
        rcsc::Body_TurnToAngle( target_angle ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
	agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieBasicMove::doGoToMovePoint( rcsc::PlayerAgent * agent, const rcsc::Vector2D & move_point )
{
    // move to target point
    // check Y coordinate difference

    const rcsc::WorldModel & wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    double dist_thr = wm.ball().distFromSelf() * 0.08;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    const double y_diff = std::fabs( move_point.y - wm.self().pos().y ) - wm.self().vel().y;
    if ( y_diff < dist_thr )
    {
        // already there
        return false;
    }

    //----------------------------------------------------------//
    // dash to body direction

    double dash_power = getBasicDashPower( agent, move_point );

    // body direction is OK
    if ( std::fabs( wm.self().body().abs() - 90.0 ) < 7.0 )
    {
        // calc dash power only to reach the target point
        double required_power = y_diff / wm.self().dashRate();
        if ( dash_power > required_power )
        {
            dash_power = required_power;
        }

        if ( move_point.y > wm.self().pos().y )
        {
            if ( wm.self().body().degree() < 0.0 )
            {
                dash_power *= -1.0;
            }
        }
        else
        {
            if ( wm.self().body().degree() > 0.0 )
            {
                dash_power *= -1.0;
            }
        }

        dash_power = sp.normalizeDashPower( dash_power );
        agent->doDash( dash_power );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    else
    {
        doGoToPointLookBall( agent, move_point, wm.ball().angleFromSelf(), dist_thr, dash_power );
    }
    agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_GoalieBasicMove::doGoToPointLookBall( rcsc::PlayerAgent * agent, const rcsc::Vector2D & target_point, const rcsc::AngleDeg & body_angle, const double & dist_thr, const double & dash_power, const double & back_power_rate )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.gameMode().type() == rcsc::GameMode::PlayOn
         || wm.gameMode().type() == rcsc::GameMode::PenaltyTaken_ )
    {
        rcsc::Bhv_GoToPointLookBall( target_point, dist_thr, dash_power, back_power_rate ).execute( agent );
		agent->setNeckAction( new Neck_GoalieTurnNeck() );
    }
    else
    {
        if ( Body_GoToPoint( target_point, dist_thr, dash_power ).execute( agent ) )
        {
			agent->setNeckAction( new Neck_GoalieTurnNeck() );
        }
        else
        {
            rcsc::Body_TurnToAngle( body_angle ).execute( agent );
            agent->setNeckAction( new Neck_GoalieTurnNeck() );
        }

        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
}

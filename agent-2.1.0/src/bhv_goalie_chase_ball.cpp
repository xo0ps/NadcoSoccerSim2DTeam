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

#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_danger_area_tackle.h"
#include "body_go_to_point.h"
#include "neck_goalie_turn_neck.h"
#include "body_intercept.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>


/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_GoalieChaseBall::execute( rcsc::PlayerAgent * agent )
{

    //////////////////////////////////////////////////////////////////
    // tackle
    if ( Bhv_DangerAreaTackle().execute( agent ) )
    {
        return true;
    }

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point

    rcsc::Vector2D my_int_pos = wm.ball().inertiaPoint( wm.interceptTable()->selfReachCycle() );
    
    double pen_thr = wm.ball().distFromSelf() * 0.1 + 1.0;
    if ( pen_thr < 1.0 ) pen_thr = 1.0;

    //----------------------------------------------------------
    const rcsc::Line2D ball_line( wm.ball().pos(), wm.ball().vel().th() );
    const rcsc::Line2D defend_line( rcsc::Vector2D( wm.self().pos().x, -10.0 ),
                                    rcsc::Vector2D( wm.self().pos().x, 10.0 ) );
    const rcsc::Line2D goal_line( rcsc::Vector2D( -sp.pitchHalfLength(), 0.0 ),
                                  rcsc::AngleDeg( 90.0 ) );
    rcsc::Vector2D intersection( wm.self().pos().x, 0.0 );
    if( ball_line.intersection( defend_line ).isValid() )
    {
        intersection = ball_line.intersection( defend_line );
    }

    rcsc::Vector2D next_dash_self_pos( wm.self().pos().x + wm.self().playerType().realSpeedMax() 
                                       * wm.self().body().cos(),
                                       wm.self().pos().y + wm.self().playerType().realSpeedMax() 
                                       * wm.self().body().sin() );
    rcsc::Vector2D next_inertia_self_pos = wm.self().pos() + wm.self().vel();
    rcsc::Vector2D next_ball_pos = wm.ball().pos() + wm.ball().vel();
    rcsc::Vector2D next_dash_ball_rpos = next_ball_pos - next_dash_self_pos;
    next_dash_ball_rpos.setPolar( next_dash_ball_rpos.r(), next_dash_ball_rpos.th() - wm.self().body() );
    rcsc::Vector2D next_turn_ball_rpos = next_ball_pos - next_inertia_self_pos;
    if( next_turn_ball_rpos.th().degree() - wm.self().body().degree() > 0 )
    {
        next_turn_ball_rpos.setPolar( next_turn_ball_rpos.r(), next_turn_ball_rpos.th() 
                                      - ( wm.self().body() + rcsc::AngleDeg( 90.0 ) ) );
    }
    else
    {
        next_turn_ball_rpos.setPolar( next_turn_ball_rpos.r(), next_turn_ball_rpos.th() 
                                      - ( wm.self().body() + rcsc::AngleDeg( -90.0 ) ) );
    }
    rcsc::Rect2D tackle_area( rcsc::Vector2D( -sp.tackleBackDist(),
                                              -sp.tackleWidth() ),
                             rcsc::Vector2D( sp.tackleDist(),
                                             sp.tackleWidth() ) );

    //next cycle catchable or tackle
    if( ( next_dash_self_pos.dist( next_ball_pos ) < sp.catchableArea() ) && ! wm.existKickableOpponent() )
    {
        Body_GoToPoint( next_dash_self_pos, 0.1, sp.maxDashPower(), 1, false, false ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
    else
    if( next_inertia_self_pos.dist( next_ball_pos ) < sp.catchableArea() && ! wm.existKickableOpponent() )
    {
        rcsc::Body_TurnToAngle( ( next_ball_pos - next_inertia_self_pos ).th() - wm.self().body() ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;

    }
    else
    if( tackle_area.contains( next_dash_ball_rpos ) && ! wm.existKickableOpponent() )
    {
        Body_GoToPoint( next_dash_self_pos, 0.1, sp.maxDashPower(), 1, false, false ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
    else
    if( tackle_area.contains( next_turn_ball_rpos ) && ! wm.existKickableOpponent() )
    {
        if( ( next_ball_pos - next_inertia_self_pos ).th().degree() - wm.self().body().degree() > 0 )
        {
            rcsc::Body_TurnToAngle( wm.self().body() + rcsc::AngleDeg( 90.0 ) ).execute( agent );
        }
        else
        {
            rcsc::Body_TurnToAngle( wm.self().body() + rcsc::AngleDeg( -90.0 ) ).execute( agent );
        }
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    // tackle
    if ( Bhv_DangerAreaTackle( 0.01 ).execute( agent ) && ! wm.existKickableOpponent() )
    {
        return true;
    }
    else
    if( wm.self().tackleProbability() > 0.01 && ! wm.existKickableOpponent() )
    {
        agent->doTackle( rcsc::Vector2D( wm.ball().pos() - wm.self().pos() ).th().degree() );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    if( my_int_pos.x < -sp.pitchHalfLength()
        && wm.self().pos().absY() < sp.goalAreaHalfWidth()
        && wm.self().pos().x < -51.5 )
    {
        if( ( wm.self().body().degree() < -60.0
              && wm.self().body().degree() >= -95.0
              && intersection.y < wm.self().pos().y )
            || ( wm.self().body().degree() > 60.0
                 && wm.self().body().degree() <= 95.0
                 && intersection.y > wm.self().pos().y ) )
        {
            Body_GoToPoint( ball_line.intersection( rcsc::Line2D( wm.self().pos(), wm.self().body() ) ), 0.1, sp.maxDashPower(), 1, false, false ).execute( agent );
        }
        else if( intersection.y > wm.self().pos().y )
        {
            rcsc::Body_TurnToAngle( rcsc::AngleDeg( 90.0 ) ).execute( agent );
        }
        else
        {
            rcsc::Body_TurnToAngle( rcsc::AngleDeg( -90.0 ) ).execute( agent );
        }
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    if( my_int_pos.x > -sp.pitchHalfLength() - 1.0
        && my_int_pos.x < sp.ourPenaltyAreaLineX() - pen_thr
        && my_int_pos.absY() < sp.penaltyAreaHalfWidth() - pen_thr )
    {
        Body_GoToPoint( my_int_pos, 0.3, sp.maxDashPower(), 1, false, false ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }
    int self_goalie_min = wm.interceptTable()->selfReachCycle();
    int opp_min_cyc = wm.interceptTable()->opponentReachCycle();

    if ( ! intersection.isValid()
         || ball_line.dist( wm.self().pos() ) < sp.catchableArea() * 0.8
         || ball_line.intersection( goal_line ).absY() > sp.goalHalfWidth() + 3.0
         )
    {
        if ( ! wm.existKickableOpponent() )
        {
            if ( self_goalie_min <= opp_min_cyc + 2
                 && my_int_pos.x > -sp.pitchHalfLength() - 2.0
                 && my_int_pos.x < sp.ourPenaltyAreaLineX() - pen_thr
                 && my_int_pos.absY() < sp.penaltyAreaHalfWidth() - pen_thr
                 )
            {
                if ( Body_Intercept( false ).execute( agent ) )
                {
                    agent->setNeckAction( new Neck_GoalieTurnNeck() );
                    return true;
                }
            }

            Body_GoToPoint( wm.ball().pos(), 0.5, sp.maxDashPower(), 1, false, false ).execute( agent );
            agent->setNeckAction( new Neck_GoalieTurnNeck() );
            return true;
        }
    }

    //----------------------------------------------------------
    // check already there
    const rcsc::Vector2D my_inertia_final_pos = wm.self().pos() + wm.self().vel() / (1.0 - wm.self().playerType().playerDecay());
    double dist_thr = 0.2 + wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    // if already intersection point stop dash
    if ( my_inertia_final_pos.dist( intersection ) < dist_thr )
    {
        rcsc::Body_StopDash( false ).execute( agent );
        agent->setNeckAction( new Neck_GoalieTurnNeck() );
        return true;
    }

    //----------------------------------------------------------
    // forward or backward

    if ( wm.ball().pos().x > -35.0 )
    {
        if ( wm.ball().pos().y * intersection.y < 0.0 ) // opposite side
        {
            intersection.y = 0.0;
        }
        else
        {
            intersection.y *= 0.5;
        }
    }
    rcsc::Vector2D projection = ball_line.projection( wm.self().pos() );
    Body_GoToPoint( projection, 0.5, sp.maxDashPower(), 1, false, false ).execute( agent );
    agent->setNeckAction( new Neck_GoalieTurnNeck() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_GoalieChaseBall::doGoToCatchPoint( rcsc::PlayerAgent * agent, const rcsc::Vector2D & target_point )
{
    const rcsc::WorldModel & wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    double dash_power = 0.0;

    rcsc::Vector2D rel = target_point - wm.self().pos();
    rel.rotate( - wm.self().body() );
    rcsc::AngleDeg rel_angle = rel.th();
    const double angle_buf = std::fabs( rcsc::AngleDeg::atan2_deg( sp.catchableArea() * 0.9, rel.r() ) );

    // forward dash
    if ( rel_angle.abs() < angle_buf )
    {
        dash_power = std::min( wm.self().stamina() + wm.self().playerType().extraStamina(), sp.maxDashPower() );
        agent->doDash( dash_power );
    }
    // back dash
    else
    if ( rel_angle.abs() > 180.0 - angle_buf )
    {
        dash_power = sp.minDashPower();

        double required_stamina = ( sp.minDashPower() < 0.0
                                    ? sp.minDashPower() * -2.0
                                    : sp.minDashPower() );
        if ( wm.self().stamina() + wm.self().playerType().extraStamina() < required_stamina )
        {
            dash_power = wm.self().stamina() + wm.self().playerType().extraStamina();
            if ( sp.minDashPower() < 0.0 )
            {
                dash_power *= -0.5;
                if ( dash_power < sp.minDashPower() )
                {
                    dash_power = sp.minDashPower();
                }
            }
        }
        agent->doDash( dash_power );
    }
    // forward dash turn
    else
    {
        agent->doTurn( rel_angle );
    }
    agent->setNeckAction( new Neck_GoalieTurnNeck() );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieChaseBall::is_ball_chase_situation( const rcsc::PlayerAgent* agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    if ( wm.gameMode().type() != rcsc::GameMode::PlayOn )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    ////////////////////////////////////////////////////////////////////////
    // ball is in very dangerous area
    const rcsc::Vector2D ball_next_pos = wm.ball().pos() + wm.ball().vel();
    if ( ball_next_pos.x < -sp.pitchHalfLength() + 8.0
         && ball_next_pos.absY() < sp.goalHalfWidth() + 3.0 )
    {
        // exist kickable teammate
        // avoid back pass
        if ( wm.existKickableTeammate() )
        {
            return false;
        }
        else
        if ( wm.ball().distFromSelf() < 3.0 && self_min <= 3 )
        {
            return true;
        }
        else
        if ( self_min > opp_min + 3 && opp_min < 7 )
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // check shoot moving
    if ( is_ball_shoot_moving( agent ) )
        //&& self_min < opp_min )
    {
        return true;
    }

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point

    const rcsc::Vector2D my_int_pos = wm.ball().inertiaPoint( wm.interceptTable()->selfReachCycle() );

    double pen_thr = wm.ball().distFromSelf() * 0.1 + 1.0;
    if ( pen_thr < 1.0 ) pen_thr = 1.0;
    if ( ( my_int_pos.absY() > sp.penaltyAreaHalfWidth() - pen_thr
           && my_int_pos.x > -sp.pitchHalfLength() )
         || my_int_pos.x > sp.ourPenaltyAreaLineX() - pen_thr )
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////
    // Now, I can chase the ball
    // check the ball possessor

    if ( wm.existKickableTeammate()
         && ! wm.existKickableOpponent() )
    {
        return false;
    }

    if ( opp_min <= self_min - 2 )
    {
        return false;
    }

    const double my_dist_to_catch = wm.self().pos().dist( my_int_pos );

    double opp_min_dist = 10000.0;
    wm.getOpponentNearestTo( my_int_pos, 30, &opp_min_dist );

    if ( opp_min_dist < my_dist_to_catch - 2.0 )
    {
        return false;
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GoalieChaseBall::is_ball_shoot_moving( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel& wm = agent->world();
	const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    if ( wm.ball().distFromSelf() > 30.0 )
    {
        return false;
    }
#if 1
    //if ( wm.ball().pos().x > -34.5 )
    if ( wm.ball().pos().x > -30.0 )
    {
        return false;
    }
#endif

    // check opponent kicker
    if ( wm.existKickableOpponent() )
    {
        return false;
    }
    else if ( wm.existKickableTeammate() )
    {
        return false;
    }

    if ( wm.ball().vel().absX() < 0.1 )
    {
        if ( wm.ball().pos().x < -46.0
             && wm.ball().pos().absY() < sp.goalHalfWidth() + 2.0 )
        {
            return true;
        }
        return false;
    }


    const rcsc::Line2D ball_line( wm.ball().pos(), wm.ball().vel().th() );
    const double intersection_y = ball_line.getY( -sp.pitchHalfLength() );

    if ( std::fabs( ball_line.getB() ) > 0.1
         && std::fabs( intersection_y ) < sp.goalHalfWidth() + 2.0 )
    {
        if ( wm.ball().pos().x < -35.0//-40.0
             && wm.ball().pos().absY() < 15.0 )
        {
            const rcsc::Vector2D end_point
                = wm.ball().pos()
                + wm.ball().vel() / ( 1.0 - sp.ballDecay());
            if ( wm.ball().vel().r() > 0.5 // 1.0
                 && end_point.x < -sp.pitchHalfLength() + 2.0 )
            {
                return true;
            }
        }
    }
    return false;
}

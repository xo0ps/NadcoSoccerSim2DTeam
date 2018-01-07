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

#include "bhv_penalty_kick.h"
#include "shoot_table.h"
#include "body_shoot.h"
#include "bhv_goalie_chase_ball.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_go_to_static_ball.h"
#include "bhv_basic_tackle.h"
#include "bhv_danger_area_tackle.h"
#include "body_dribble.h"
#include "body_go_to_point.h"
#include "body_intercept.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "body_dash.h"
#include "body_pass.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/penalty_kick_state.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::isKickTaker( rcsc::PlayerAgent * agent )
{
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();
    int taker_unum
        = 11 - ( ( state->ourTakerCounter() - 1 ) % 11 );
    return taker_unum == agent->world().self().unum();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::PenaltyKickState * state = wm.penaltyKickState();

    switch ( wm.gameMode().type() ) {
    case GameMode::PenaltySetup_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerSetup( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyReady_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerReady( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyTaken_:
        if ( state->currentTakerSide() == agent->world().ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKicker( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalie( agent );
            }
        }
        break;
    case GameMode::PenaltyScore_:
    case GameMode::PenaltyMiss_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyOnfield_:
    case GameMode::PenaltyFoul_:
        break;
    default:
        // nothing to do.
        std::cerr << "Current playmode is NOT a Penalty Shootout???" << std::endl;
        return false;
    }


    if ( wm.self().goalie() )
    {
        return doGoalieWait( agent );
    }
    else
    {
        return doKickerWait( agent );
    }

    // never reach here
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doKickerWait( rcsc::PlayerAgent * agent )
{
	
    double dist_step = ( 9.0 + 9.0 ) / 12;
    rcsc::Vector2D wait_pos;
    wait_pos.x = -6 + agent->world().self().unum();
    wait_pos.y = agent->world().self().unum() % 2 ? 8.0 : -8.0;

    // already there
    if ( agent->world().self().pos().dist( wait_pos ) < 0.7 )
    {
        //rcsc::Bhv_NeckBodyToBall().execute( agent );
		static rcsc::AngleDeg deg = 0.0;
	    deg += 60.0;
	    if( deg == 360.0 )
			deg = 0.0;
	    rcsc::Body_TurnToAngle( deg ).execute( agent );
	    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }
    else
    {
        // no dodge
        Body_GoToPoint( wait_pos, 0.5, rcsc::ServerParam::i().maxDashPower() * 0.8 ).execute( agent );
        //Body_GoToPoint( agent->world().self().pos() , 0.7, rcsc::ServerParam::i().maxDashPower() ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doKickerSetup( rcsc::PlayerAgent * agent )
{
    const rcsc::Vector2D goal_c = rcsc::ServerParam::i().theirTeamGoalPos();
    const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    rcsc::AngleDeg place_angle = 0.0;

    // ball is close enoughly.
    if ( ! Bhv_GoToStaticBall( place_angle ).execute( agent ) )
    {
        rcsc::Body_TurnToPoint( goal_c ).execute( agent );
        if ( opp_goalie )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToPoint( goal_c ) );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doKickerReady( rcsc::PlayerAgent * agent )
{
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();
    // stamina recovering...
    if ( agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() - 10.0
         && ( agent->world().time().cycle()
              - state->time().cycle()
              > rcsc::ServerParam::i().penReadyWait() - 3 )
         )
    {
        return doKickerSetup( agent );
    }

    if ( ! agent->world().self().isKickable() )
    {
        return doKickerSetup( agent );
    }

    return doKicker( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doKicker( rcsc::PlayerAgent * agent )
{
    /////////////////////////////////////////////////
    // case: server does NOT allow multiple kicks
    if ( ! rcsc::ServerParam::i().penAllowMultKicks() )
    {
        return doOneKickShoot( agent );
    }

    /////////////////////////////////////////////////
    // case: server allows multiple kicks

    // get ball
    if ( ! agent->world().self().isKickable() )
    {
        if ( ! Body_Intercept().execute( agent ) )
        {
            Body_GoToPoint( agent->world().ball().pos(), 0.4, rcsc::ServerParam::i().maxDashPower() ).execute( agent );
        }

        if ( agent->world().ball().posCount() > 0 )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        }
        else
        {
            const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
            if ( opp_goalie )
            {
                agent->setNeckAction( new rcsc::Neck_TurnToPoint( opp_goalie->pos() ) );
            }
            else
            {
                agent->setNeckAction( new rcsc::Neck_ScanField() );
            }
        }

        return true;
    }

    // kick decision
    if ( doShoot( agent ) )
    {
        return true;
    }

    return doDribble( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doOneKickShoot( rcsc::PlayerAgent * agent )
{
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();
    const double ball_speed = agent->world().ball().vel().r();
    // ball is moveng --> kick has taken.
    if ( ! rcsc::ServerParam::i().penAllowMultKicks() && ball_speed > 0.3 )
    {
        return false;
    }

    // go to the ball side
    if ( ! agent->world().self().isKickable() )
    {
        Body_GoToPoint( agent->world().ball().pos(), 0.4, rcsc::ServerParam::i().maxDashPower() ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    // turn to the ball to get the maximal kick rate.
    if ( ( agent->world().ball().angleFromSelf() - agent->world().self().body() ).abs() > 3.0 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
        if ( opp_goalie )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToPoint( opp_goalie->pos() ) );
        }
        else
        {
			rcsc::Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
            agent->setNeckAction( new rcsc::Neck_TurnToPoint( goal_c ) );
        }
        return true;
    }

    // decide shot target point
    /*
    rcsc::Vector2D shot_point = rcsc::ServerParam::i().theirTeamGoalPos();

    const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    if ( opp_goalie )
    {
        shot_point.y = rcsc::ServerParam::i().goalHalfWidth() - 1.0;
        if ( opp_goalie->pos().absY() > 0.5 )
        {
            if ( opp_goalie->pos().y > 0.0 )
            {
                shot_point.y *= -1.0;
            }
        }
        else if ( opp_goalie->bodyCount() < 2 )
        {
            if ( opp_goalie->body().degree() > 0.0 )
            {
                shot_point.y *= -1.0;
            }
        }
    }
    */
    

	rcsc::Vector2D shot_point;
	getShootTarget( agent , &shot_point , NULL);
    
    // enforce one step kick
    Body_KickOneStep( shot_point, rcsc::ServerParam::i().ballSpeedMax() ).execute( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doShoot( rcsc::PlayerAgent * agent )
{
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();

    if ( agent->world().time().cycle() - state->time().cycle() > rcsc::ServerParam::i().penTakenWait() - 25 )
    {
        return doOneKickShoot( agent );
    }

    const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();

    if( opp_goalie )
    {
        if( agent->world().self().pos().dist( opp_goalie->pos() ) < 2.0 )
        {
            rcsc::Vector2D goal_pos = rcsc::ServerParam::i().theirTeamGoalPos();
            Body_SmartKick( goal_pos, rcsc::ServerParam::i().ballSpeedMax(), rcsc::ServerParam::i().ballSpeedMax() * 0.99, 1 ).execute( agent );
            agent->setNeckAction( new rcsc::Neck_TurnToPoint( goal_pos ) );
            return true;
        }
    }

    rcsc::Vector2D shot_point;
    double shot_speed;

    if ( getShootTarget( agent, &shot_point, &shot_speed ) )
    {
        Body_SmartKick( shot_point, shot_speed, shot_speed * 0.96, 2 ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToPoint( shot_point ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::getShootTarget( rcsc::PlayerAgent * agent,
                                 rcsc::Vector2D* point, double* first_speed )
{
	/*
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();

    // check if I am in shootable area.
    const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    rcsc::Vector2D goal_pos( rcsc::ServerParam::i().pitchHalfLength(), 0.0 );
    if( agent->world().penaltyKickState()->onfieldSide() == agent->world().ourSide() )
    {
        goal_pos.x *= -1.0;
    }

    // goalie is not found.
    if ( ! opp_goalie )
    {
        if( agent->world().self().pos().dist( goal_pos ) < 20.0 )
        {
            rcsc::Vector2D shot_c(rcsc::ServerParam::i().pitchHalfLength(), 0.0);
            if ( state->onfieldSide() == agent->world().ourSide() )
            {
                shot_c.x *= -1.0;
            }
            if ( point ) *point = shot_c;
            if ( first_speed ) *first_speed = rcsc::ServerParam::i().ballSpeedMax();
            return true;
        }
        return false;
    }

    int best_l_or_r = 0;
    double best_speed = rcsc::ServerParam::i().ballSpeedMax() + 1.0;


    double post_buf = 1.0
        + std::min( 1.0,
                    ( rcsc::ServerParam::i().pitchHalfLength()
                      - agent->world().self().pos().absX() )
                    * 0.1 );

    // consder only 2 angle
    rcsc::Vector2D shot_l( rcsc::ServerParam::i().pitchHalfLength(),
                           -rcsc::ServerParam::i().goalHalfWidth() + post_buf );
    rcsc::Vector2D shot_r( rcsc::ServerParam::i().pitchHalfLength(),
                           rcsc::ServerParam::i().goalHalfWidth() - post_buf );
    if ( state->onfieldSide() == agent->world().ourSide() )
    {
        shot_l *= -1.0;
        shot_r *= -1.0;
    }

    const rcsc::AngleDeg angle_l = ( shot_l - agent->world().ball().pos() ).th();
    const rcsc::AngleDeg angle_r = ( shot_r - agent->world().ball().pos() ).th();

    // !!! Magic Number !!!
    const double goalie_max_speed = 1.0;
    // default player speed max * conf decay
    const double goalie_dist_buf
        = goalie_max_speed * std::min( 5, opp_goalie->posCount() )
        + rcsc::ServerParam::i().catchAreaLength()
        + 0.2;

    const rcsc::Vector2D goalie_next_pos = opp_goalie->pos() + opp_goalie->vel();

    for ( int i = 0; i < 2; i++ )
    {
        const rcsc::Vector2D& target = ( i == 0 ? shot_l : shot_r );
        const rcsc::AngleDeg& angle = ( i == 0 ? angle_l : angle_r );

        double dist2goal = agent->world().ball().pos().dist( target );

        // at first, set minimal speed to reach the goal line
        double tmp_first_speed
            =  ( dist2goal + 5.0 ) * ( 1.0 - rcsc::ServerParam::i().ballDecay() );
        tmp_first_speed = std::max( 1.2, tmp_first_speed );
        bool not_over_max = true;
        while ( not_over_max )
        {
            if ( tmp_first_speed > rcsc::ServerParam::i().ballSpeedMax() )
            {
                not_over_max = false;
                tmp_first_speed = rcsc::ServerParam::i().ballSpeedMax();
            }

            rcsc::Vector2D ball_pos = agent->world().ball().pos();
            rcsc::Vector2D ball_vel =
                rcsc::Vector2D::polar2vector( tmp_first_speed, angle );
            ball_pos += ball_vel;
            ball_vel *= rcsc::ServerParam::i().ballDecay();

            bool goalie_can_reach = false;

            // goalie move at first step is ignored (cycle is set to ZERO),
            // because goalie must look the ball velocity before chasing action.
            double cycle = 0.0;
            while ( ball_pos.absX() < rcsc::ServerParam::i().pitchHalfLength() )
            {
                if ( goalie_next_pos.dist( ball_pos )
                     < goalie_max_speed * cycle + goalie_dist_buf )
                {
                    goalie_can_reach = true;
                    break;
                }

                ball_pos += ball_vel;
                ball_vel *= rcsc::ServerParam::i().ballDecay();
                cycle += 1.0;
            }

            if ( ! goalie_can_reach )
            {
                if ( tmp_first_speed < best_speed )
                {
                    best_l_or_r = i;
                    best_speed = tmp_first_speed;
                }
                break; // end of this angle
            }
            tmp_first_speed += 0.4;
        }
    }


    if ( best_speed <= rcsc::ServerParam::i().ballSpeedMax() )
    {
        if ( point )
        {
            *point = ( best_l_or_r == 0 ? shot_l : shot_r );
        }
        if ( first_speed )
        {
            *first_speed = best_speed;
        }

        return true;
    }

    return false;
    */
    
    ShootTable S_shoot_table;

	const ShootTable::ShotCont & shots = S_shoot_table.getShots( agent );

    // update
    if ( shots.empty() )
    {
        return false;
    }

    ShootTable::ShotCont::const_iterator shot = std::min_element( shots.begin(), shots.end(),ShootTable::ScoreCmp() );

    if ( shot == shots.end() )
    {
        return false;
    }

    rcsc::Vector2D target_point = shot->point_;
    
    rcsc::Vector2D one_step_vel
        = rcsc::KickTable::calc_max_velocity( ( target_point - agent->world().ball().pos() ).th(),
                                        agent->world().self().kickRate(),
                                        agent->world().ball().vel() );
	* point = target_point;
	* first_speed = shot->speed_;
}

/*-------------------------------------------------------------------*/
/*!
  dribble to the shootable point
*/
bool
Bhv_PenaltyKick::doDribble( rcsc::PlayerAgent * agent )
{
	
    static bool S_target_reversed = false;
    const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();
    const rcsc::Vector2D goal_c = rcsc::ServerParam::i().theirTeamGoalPos();
    const double penalty_abs_x = rcsc::ServerParam::i().theirPenaltyAreaLineX();
    const rcsc::PlayerObject * opp_goalie = agent->world().getOpponentGoalie();
    const double goalie_max_speed = 1.0;
    const double my_abs_x = agent->world().self().pos().absX();

    const double goalie_dist = ( opp_goalie
                                 ? ( opp_goalie->pos().dist(agent->world().self().pos())
                                     - goalie_max_speed * std::min(5, opp_goalie->posCount()) )
                                 : 200.0 );
    const double goalie_abs_x = ( opp_goalie
                                  ? opp_goalie->pos().absX()
                                  : 200.0 );

    /////////////////////////////////////////////////
    // dribble parametors

    const double base_target_abs_y = rcsc::ServerParam::i().goalHalfWidth() + 4.0;
    rcsc::Vector2D drib_target = goal_c;
    double drib_power = rcsc::ServerParam::i().maxDashPower();
    int drib_dashes = 10;
    const rcsc::Rect2D safety_rect = rcsc::Rect2D::from_center( agent->world().ball().pos().x + 7.0, agent->world().ball().pos().y, 14.0, 15.0 );
    if ( agent->world().existOpponentIn( safety_rect, 10, false ) )
		drib_dashes = 3;

    /////////////////////////////////////////////////

    // it's too far to the goal.
    // dribble to the shootable area
    
    if ( my_abs_x < penalty_abs_x - 15.0 && goalie_dist > 10.0 )
    {
		//Body_Dash().execute( agent );
		if( doFirstKick( agent ) )
			return true;
		//drib_target.y = 15.0;
	}
    else
    if ( my_abs_x < penalty_abs_x - 6.0 && goalie_dist > 10.0 )
    {
        //drib_power *= 0.8;//0.6;
        //drib_target.y = 13.0;
    }
    else
    {
        if ( goalie_abs_x > my_abs_x )
        {
            if ( goalie_dist < 4.0 )
            {
                S_target_reversed = ! S_target_reversed;
            }

            if ( S_target_reversed )
            {
                if ( agent->world().self().pos().y < -base_target_abs_y + 2.0 )
                {
                    S_target_reversed = false;
                    drib_target.y = base_target_abs_y;
                }
                else
                {
                    drib_target.y = -base_target_abs_y;
                }
            }
            else // == if ( ! S_target_reversed )
            {
                if ( agent->world().self().pos().y > base_target_abs_y - 2.0 )
                {
                    S_target_reversed = true;
                    drib_target.y = -base_target_abs_y;
                }
                else
                {
                    drib_target.y = base_target_abs_y;
                }
            }

            drib_target.x = goalie_abs_x + 1.0;
            drib_target.x = rcsc::min_max( penalty_abs_x - 2.0, drib_target.x, rcsc::ServerParam::i().pitchHalfLength() - 4.0 );

            //double dashes = ( agent->world().self().pos().dist( drib_target ) / rcsc::ServerParam::i().defaultPlayerSpeedMax() );
            //drib_dashes = static_cast<int>(floor(dashes));
            //drib_dashes = rcsc::min_max( 1, drib_dashes, 6 );
        }
    }

    rcsc::Vector2D target_rel = drib_target - agent->world().self().pos();
    double buf = 2.0;
    if ( drib_target.absX() < penalty_abs_x )
    {
        buf += 2.0;
    }
    if ( target_rel.absX() < 5.0
    && ( opp_goalie == NULL || opp_goalie->pos().dist( drib_target ) > target_rel.r() - buf ) )
    {
        if ( ( target_rel.th() - agent->world().self().body() ).abs() < 5.0 )
        {
            double first_speed = rcsc::calc_first_term_geom_series_last( 1.8 , target_rel.r(), rcsc::ServerParam::i().ballDecay() );
            first_speed = std::min( first_speed, rcsc::ServerParam::i().ballSpeedMax() );
            Body_SmartKick( drib_target, first_speed, first_speed * 0.96, 3 ).execute( agent );
        }
        else if ( ( agent->world().ball().rpos()
                    + agent->world().ball().vel()
                    - agent->world().self().vel() ).r()
                  < agent->world().self().playerType().kickableArea() - 0.2 )
        {
            rcsc::Body_TurnToPoint( drib_target ).execute( agent );
        }
        else
        {
            rcsc::Body_StopBall().execute( agent );
        }
    }
    else
    {
        bool dodge_mode = true;
        if ( opp_goalie == NULL
             || ( ( opp_goalie->pos() - agent->world().self().pos() ).th()
                  - ( drib_target - agent->world().self().pos() ).th() ).abs() > 45.0 )
        {
            dodge_mode = false;
        }
        
        dodge_mode = false;

        Body_Dribble( drib_target, 2.0, drib_power, drib_dashes, dodge_mode ).execute( agent );
    }

    if ( opp_goalie )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToPoint( opp_goalie->pos() ) );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToPoint( goal_c ) );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalieWait( rcsc::PlayerAgent * agent )
{
	const rcsc::Vector2D goal_c = rcsc::ServerParam::i().theirTeamGoalPos();
    rcsc::Body_TurnToPoint( goal_c ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalieSetup( rcsc::PlayerAgent * agent )
{
	rcsc::Vector2D move_point( rcsc::ServerParam::i().ourTeamGoalLineX() + rcsc::ServerParam::i().penMaxGoalieDistX() - 0.1, 0.0 );

    if ( Body_GoToPoint( move_point, 0.5, rcsc::ServerParam::i().maxDashPower() ).execute( agent ) )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    // alreadh there
    /*
    if ( std::fabs( 90.0 - agent->world().self().body().abs() ) > 2.0 )
    {
        rcsc::Vector2D face_point( agent->world().self().pos().x, 100.0 );
        if ( agent->world().self().body().degree() < 0.0 )
        {
            face_point.y = -100.0;
        }
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
    }

    rcsc::Vector2D neck_point( 0.0, 0.0 );
    agent->setNeckAction( new rcsc::Neck_TurnToPoint( neck_point ) );
    */
    
    rcsc::Body_TurnToBall().execute( agent );
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalie( rcsc::PlayerAgent * agent )
{
    //const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();
	const rcsc::Vector2D goal_c = rcsc::ServerParam::i().theirTeamGoalPos();
	
    ///////////////////////////////////////////////
    // check if catchabale
    rcsc::Rect2D our_penalty( rcsc::Vector2D( -rcsc::ServerParam::i().pitchHalfLength(),
                                              -rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                              rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() - 1.0,
                                            rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );
    /*
    if ( state->onfieldSide() != agent->world().ourSide() )
    {
        our_penalty.assign( rcsc::Vector2D( rcsc::ServerParam::i().theirPenaltyAreaLineX() + 1.0,
                                            -rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                            rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() - 1.0,
                                          rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );
    }
    */

    if ( agent->world().ball().distFromSelf() < rcsc::ServerParam::i().catchableArea() - 0.05
         && our_penalty.contains( agent->world().ball().pos() ) )
    {
        return agent->doCatch();
    }

    //if( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    if( Bhv_DangerAreaTackle().execute( agent ) )
    {
        return true;
    }

    if ( agent->world().self().isKickable() )
    {
        rcsc::Body_ClearBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }


    ///////////////////////////////////////////////
    // if taker can only one kick, goalie shoud stay the front of goal.
    if ( ! rcsc::ServerParam::i().penAllowMultKicks() )
    {
        // kick has not taken.
        if ( agent->world().ball().vel().r2() < 0.01
             && agent->world().ball().pos().absX() < ( rcsc::ServerParam::i().pitchHalfLength()
                                                       - rcsc::ServerParam::i().penDistX() - 1.0 )
             )
        {
            return doGoalieSetup( agent );
        }
        if ( agent->world().ball().vel().r2() > 0.01 )
        {
            return doGoalieSlideChase( agent );
        }
    }

    ///////////////////////////////////////////////
    //
    return doGoalieBasicMove( agent );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalieBasicMove( rcsc::PlayerAgent * agent )
{
    //const rcsc::PenaltyKickState * state = agent->world().penaltyKickState();

    rcsc::Rect2D our_penalty( rcsc::Vector2D( -rcsc::ServerParam::i().pitchHalfLength(),
                                              -rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                              rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() - 1.0,
                                            rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );
    /*
    if ( state->onfieldSide() != agent->world().ourSide() )
    {
        our_penalty.assign( rcsc::Vector2D( rcsc::ServerParam::i().theirPenaltyAreaLineX() + 1.0,
                                            -rcsc::ServerParam::i().penaltyAreaHalfWidth() + 1.0 ),
                            rcsc::Size2D( rcsc::ServerParam::i().penaltyAreaLength() - 1.0,
                                          rcsc::ServerParam::i().penaltyAreaWidth() - 2.0 ) );
    }
    */

    ////////////////////////////////////////////////////////////////////////
    // get active interception catch point
    const int self_min = agent->world().interceptTable()->selfReachCycle();
    rcsc::Vector2D move_pos = agent->world().ball().inertiaPoint( self_min );
//    if ( our_penalty.contains( move_pos ) )
    if( our_penalty.contains( move_pos ) || !agent->world().existKickableOpponent()
    || agent->world().interceptTable()->opponentReachCycle() < agent->world().interceptTable()->selfReachCycle()
    || agent->world().interceptTable()->selfReachCycle() <= 4 )
    {
        if ( Body_Intercept( false ).execute( agent ) )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBall() );
            return true;
        }
    }

    rcsc::Vector2D my_pos = agent->world().self().pos();
    rcsc::Vector2D ball_pos;
    if ( agent->world().existKickableOpponent() )
    {
        ball_pos = agent->world().opponentsFromBall().front()->pos();
        ball_pos += agent->world().opponentsFromBall().front()->vel();
    }
    else
    {
        ball_pos = rcsc::inertia_n_step_point( agent->world().ball().pos(),
                                               agent->world().ball().vel(),
                                               3,
                                               rcsc::ServerParam::i().ballDecay() );
    }

	/*
    int ourside = 1.0;
    if ( state->onfieldSide() != agent->world().ourSide() )
    {
        //my_pos *= -1.0;
        //ball_pos *= -1.0;
        ourside = -1.0;
    }
    */

    //move_pos = getGoalieMovePos( agent, ball_pos, my_pos, ourside );
    move_pos = getGoalieMovePos( agent, ball_pos, my_pos );

    if ( ! Body_GoToPoint( move_pos, 0.1, rcsc::ServerParam::i().maxDashPower() ).execute( agent ) )
    {
        // already there
        //rcsc::Body_TurnToBall().execute( agent );
        rcsc::AngleDeg face_angle = agent->world().ball().angleFromSelf();
        if ( agent->world().ball().angleFromSelf().isLeftOf( agent->world().self().body() ) )
            face_angle += 90.0;
        else
            face_angle -= 90.0;
        rcsc::Body_TurnToAngle( face_angle ).execute( agent );
    }
    rcsc::Vector2D ball_next = agent->effector().queuedNextBallPos();
    agent->setNeckAction( new rcsc::Neck_TurnToPoint( ball_next ) );

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  ball_pos & my_pos is set to self localization oriented.
  if ( onfiled_side != our_side ), these coordinates must be reversed.
*/
rcsc::Vector2D
Bhv_PenaltyKick::getGoalieMovePos( rcsc::PlayerAgent * agent, const rcsc::Vector2D & ball_pos, const rcsc::Vector2D & my_pos )
{
    const double min_x = ( -rcsc::ServerParam::i().pitchHalfLength() + rcsc::ServerParam::i().catchAreaLength() * 0.9 );
	
	const rcsc::Vector2D goal_pos = rcsc::ServerParam::i().ourTeamGoalPos();

    if ( ball_pos.x < -39.0 )
    {
        if ( ball_pos.absY() < rcsc::ServerParam::i().goalHalfWidth() )
            return rcsc::Vector2D( min_x, ball_pos.y );
        else
            return rcsc::Vector2D( min_x, ( rcsc::sign( ball_pos.y ) * rcsc::ServerParam::i().goalHalfWidth() ) );
    }
    else
    if( my_pos.dist( ball_pos ) < 5.0 )
    {
        rcsc::Vector2D opp_front_pos( agent->world().opponentsFromBall().front()->pos() );
        opp_front_pos.x -= rcsc::ServerParam::i().defaultKickableArea();
        rcsc::Line2D line_to_goal( opp_front_pos, goal_pos );
        rcsc::Vector2D move_pos_l( my_pos.x + std::min( 0.5, std::max( ( opp_front_pos.x - my_pos.x ) , 0.0 ) ) , -1.0 );
        rcsc::Vector2D move_pos_r( my_pos.x + std::min( 0.5, std::max( ( opp_front_pos.x - my_pos.x ) , 0.0 ) ) , 1.0 );
        rcsc::Line2D move_line( move_pos_l, move_pos_r );
        rcsc::Vector2D intersection = move_line.intersection( line_to_goal );
        return intersection;
    }
    else
    {
        rcsc::Line2D line_to_goal( ball_pos, goal_pos );
        rcsc::Vector2D move_pos_l( ball_pos.x - 3.0 , -1.0 );
        rcsc::Vector2D move_pos_r( ball_pos.x - 3.0 , 1.0 );
        rcsc::Line2D move_line( move_pos_l, move_pos_r );
        rcsc::Vector2D intersection = move_line.intersection( line_to_goal );
        return intersection;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalieChaseBall( rcsc::PlayerAgent * agent )
{


    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doGoalieSlideChase( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( std::fabs( 90.0 - wm.self().body().abs() ) > 2.0 )
    {
        rcsc::Vector2D face_point( wm.self().pos().x, 100.0);
        if ( wm.self().body().degree() < 0.0 )
        {
            face_point.y = -100.0;
        }
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    rcsc::Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
    rcsc::Line2D ball_line( ball_ray.origin(), ball_ray.dir() );
    rcsc::Line2D my_line( wm.self().pos(), wm.self().body() );


    rcsc::Vector2D intersection = my_line.intersection( ball_line );
    if ( ! intersection.isValid()
         || ! ball_ray.inRightDir( intersection ) )
    {
        Body_Intercept( false ).execute( agent ); // goalie mode
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    if ( agent->world().self().pos().dist( intersection )
         < rcsc::ServerParam::i().catchAreaLength() * 0.7 )
    {
        rcsc::Body_StopDash( false ).execute( agent ); // not save recovery
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

    rcsc::AngleDeg angle = ( intersection - agent->world().self().pos() ).th();
    double dash_power = rcsc::ServerParam::i().maxDashPower();
    if ( ( angle - agent->world().self().body() ).abs() > 90.0 )
    {
        dash_power *= -1.0;
    }
    agent->doDash( dash_power );
    agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    return true;
}


bool
Bhv_PenaltyKick::goToStaticBall( rcsc::PlayerAgent * agent, rcsc::AngleDeg ball_place_angle )
{
    const rcsc::WorldModel & wm = agent->world();
    const double dir_margin = 15.0;
    rcsc::AngleDeg angle_diff = ( wm.ball().angleFromSelf()
                                  - ball_place_angle );
    if( angle_diff.abs() < dir_margin
        && ( wm.ball().distFromSelf()
             < ( wm.self().playerType().playerSize()
                 + rcsc::ServerParam::i().ballSize()
                 + 0.08 ) )
        )
    {
        //already reach
        return false;
    }

    //decide sub-target point
    rcsc::Vector2D sub_target = wm.ball().pos() + rcsc::Vector2D::polar2vector( 2.3, ball_place_angle + 180.0 );
    
    double dash_power = 20.0;
    if( wm.ball().distFromSelf() > 2.0 )
    {
        //dash_power = get_set_play_dash_power( agent );
        dash_power = rcsc::ServerParam::i().maxDashPower();
    }
    
    //it is necessary to go to sub target point
    if( angle_diff.abs() > dir_margin )
    {
        Body_GoToPoint( sub_target, 0.1, dash_power, 3 ).execute( agent );
    }
    //dir diff is small. go to ball
    else
    {
        //body dir is not right
        if( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 5.0 )
        {
            rcsc::Body_TurnToBall().execute( agent );
        }
        //dash to ball
        else
        {
            agent->doDash( dash_power );
        }
    }
    agent->setNeckAction( new rcsc::Neck_ScanField() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
double
Bhv_PenaltyKick::get_set_play_dash_power( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.gameMode().type() == rcsc::GameMode::PenaltySetup_ )
    {
        return wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
    }

    double rate;
    if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.8 )
    {
        rate = 1.5
            * wm.self().stamina()
            / rcsc::ServerParam::i().staminaMax();
    }
    else
    {
        rate = 0.9
            * ( wm.self().stamina()
                - rcsc::ServerParam::i().recoverDecThrValue() )
            / rcsc::ServerParam::i().staminaMax();
        rate = std::max( 0.0, rate );
    }

    return ( wm.self().playerType().staminaIncMax()
             * wm.self().recovery()
             * rate );
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_PenaltyKick::doFirstKick( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	const rcsc::Vector2D goal_c = rcsc::ServerParam::i().theirTeamGoalPos();
    const rcsc::PlayerObject * opp_goalie = wm.getOpponentGoalie();
    const double goalie_dist =
    ( opp_goalie ? ( opp_goalie->pos().dist( wm.ball().pos() ) - std::min( 5, opp_goalie->posCount() ) ) : 200.0 );
                                 
    if( goalie_dist < 21 )
		return false;
		
	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.55 )
		return false;
	
	double shoot_y = 5.0;
	if( wm.self().unum() % 3 == 1 )
		shoot_y = -5.0;
	else
	if( wm.self().unum() % 3 == 2 )
		shoot_y = 0.0;
		
	rcsc::Vector2D target_point = wm.ball().pos() + rcsc::Vector2D( goal_c.x / 6.8 , shoot_y );
	
	double end_speed = 0.6;
    double first_speed = 2.0;
    do
    {
        first_speed = rcsc::calc_first_term_geom_series_last( end_speed, wm.ball().pos().dist( target_point ) , rcsc::ServerParam::i().ballDecay() );
        if ( first_speed < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        end_speed -= 0.05;
    }
    while ( end_speed > 0.1 );

	first_speed = std::min( first_speed , rcsc::ServerParam::i().ballSpeedMax() ); 
	first_speed *= rcsc::ServerParam::i().ballDecay();
    
	if( Body_KickOneStep( target_point, first_speed ).execute( agent ) )
	{
		Body_Pass::say_pass( agent , wm.self().unum() , target_point );		
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return true;
	}
	
	return false;
}

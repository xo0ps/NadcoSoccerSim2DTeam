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

#include "bhv_set_play.h"

#include "bhv_mark.h"
#include "bhv_set_play_free_kick.h"
#include "bhv_set_play_goal_kick.h"
#include "bhv_set_play_kick_in.h"
#include "bhv_set_play_kick_off.h"
#include "bhv_their_goal_kick_move.h"
#include "bhv_set_play_corner_kick.h"
#include "bhv_set_play_goalie_catch.h"
#include "bhv_set_play_offside.h"
#include "bhv_set_play_backpass.h"
#include "bhv_set_play_indirect_free_kick.h"
#include "body_go_to_point.h"
#include "strategy.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/common/audio_memory.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlay::execute( rcsc::PlayerAgent * agent )
{
    if ( ! agent->world().ball().posValid() )
    {
        return rcsc::Bhv_ScanField().execute( agent );
    }

    switch ( agent->world().gameMode().type() ) {
    
    case rcsc::GameMode::KickOff_:
        if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayKickOff( M_home_pos ).execute( agent );
        }
        else
        {
            rcsc::Vector2D target_point = M_home_pos;
            if ( target_point.x > -0.5 )
            {
                target_point.x = -0.5;
            }
            doBasicTheirSetPlayMove( agent, target_point , true );
        }
        break;
        
    case rcsc::GameMode::KickIn_:
        if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayKickIn( M_home_pos ).execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;
        
    case rcsc::GameMode::GoalKick_:
        if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayGoalKick( M_home_pos ).execute( agent );
        }
        else
        {
            return Bhv_TheirGoalKickMove( M_home_pos ).execute( agent );
        }
        break;

    case rcsc::GameMode::FreeKick_:
		if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayFreeKick( M_home_pos ).execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;
        
    case rcsc::GameMode::CornerKick_:
		if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayCornerKick( M_home_pos ).execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;
        
    case rcsc::GameMode::GoalieCatch_: // after catch
		if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            return Bhv_SetPlayGoalieCatch( M_home_pos ).execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;
        
    case rcsc::GameMode::OffSide_:
		if ( agent->world().gameMode().side() != agent->world().ourSide() )
        {
            return Bhv_SetPlayOffside( M_home_pos ).execute( agent );
            //return Bhv_SetPlayIndirectFreeKick().execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;

    case rcsc::GameMode::BackPass_:
		//return Bhv_SetPlayBackpass( M_home_pos ).execute( agent );
		return Bhv_SetPlayIndirectFreeKick().execute( agent );
		break;
                
    case rcsc::GameMode::IndFreeKick_:
		if ( agent->world().gameMode().side() == agent->world().ourSide() )
        {
            //return Bhv_SetPlayIndirectFreeKick( M_home_pos ).execute( agent );
            return Bhv_SetPlayIndirectFreeKick().execute( agent );
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;
        
    case rcsc::GameMode::CatchFault_:
    case rcsc::GameMode::FreeKickFault_:
    case rcsc::GameMode::FoulCharge_:
    case rcsc::GameMode::FoulPush_:
		if ( agent->world().gameMode().side() != agent->world().ourSide() )
        {
            //return Bhv_SetPlayIndirectFreeKick( M_home_pos ).execute( agent );
            return Bhv_SetPlayIndirectFreeKick( ).execute( agent );
            {
				/*
				if( agent->world().self().pos().dist( agent->world().ball().pos() ) < 30.0 )
				{
					double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
					double dist_thr = agent->world().ball().distFromSelf() * 0.1;
					if ( dist_thr < 0.7 ) dist_thr = 0.7;
					if ( ! Body_GoToPoint( agent->world().ball().pos(), dist_thr, dash_power ).execute( agent ) )
					{
						rcsc::Body_TurnToBall().execute( agent );
					}
					agent->setNeckAction( new rcsc::Neck_ScanField() );
					return true;
				}
				*/
			}
        }
        else
        {
            doBasicTheirSetPlayMove( agent, M_home_pos , true );
        }
        break;

    default:
		if ( agent->world().gameMode().isOurSetPlay( agent->world().ourSide() ) )
		{
			//return Bhv_SetPlayFreeKick( M_home_pos ).execute( agent );
			return Bhv_SetPlayIndirectFreeKick( ).execute( agent );
		}
		else
		{
			doBasicTheirSetPlayMove( agent, M_home_pos , false );
			return true;
		}
        break;
    }
    
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
double
Bhv_SetPlay::get_set_play_dash_power( const rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();

    if ( ! wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        rcsc::Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );
        if ( target_point.x > wm.self().pos().x )
        {
            if ( wm.ball().pos().x < -30.0
                 && target_point.x < wm.ball().pos().x )
            {
                return wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
            }

            double rate = 0.0;
            if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.8 )
            {
                rate = 1.5 * wm.self().stamina() / rcsc::ServerParam::i().staminaMax();
            }
            else
            {
                rate = 0.9 * ( wm.self().stamina() - rcsc::ServerParam::i().recoverDecThrValue() )
						/ rcsc::ServerParam::i().staminaMax();
                rate = std::max( 0.0, rate );
            }

            return ( wm.self().playerType().staminaIncMax()
                     * wm.self().recovery()
                     * rate );
        }
    }

    return wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
}

/*-----------------------------------------------------------------------------*
 * recursive function
 *
 *-----------------------------------------------------------------------------*/
namespace {

rcsc::Vector2D
get_avoid_circle_point( const rcsc::WorldModel & world,
                        const rcsc::Vector2D & point,
                        int depth )
{
    if ( depth > 5 )
    {
        return point;
    }

    if ( world.ball().distFromSelf() < world.self().pos().dist( point )
         && ( ( world.ball().pos() - point ).th()
              - ( world.self().pos() - point ).th() ).abs() < 90.0
         && ( world.ball().angleFromSelf()
              - ( point - world.self().pos() ).th() ).abs() < 90.0
         && ( rcsc::Line2D( world.self().pos(), point).dist2( world.ball().pos() )
              < 10.0 * 10.0 )
         )
    {
        rcsc::Vector2D new_point = world.ball().pos();
        rcsc::AngleDeg self2target = ( point - world.self().pos() ).th();
        if ( world.ball().angleFromSelf().isLeftOf( self2target ) )
        {
            new_point += rcsc::Vector2D::polar2vector( 11.5,
                                                       self2target + 90.0 );
        }
        else
        {
            new_point += rcsc::Vector2D::polar2vector( 11.5,
                                                       self2target - 90.0 );
        }
        // recursive
        return get_avoid_circle_point( world, new_point, depth + 1 );
    }

    return point;
}

} // end noname namespace

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
void
Bhv_SetPlay::doBasicTheirSetPlayMove( rcsc::PlayerAgent * agent,
                                      const rcsc::Vector2D & target_point,
                                      bool mark )
{	
	if( mark )
	{
		rcsc::Vector2D home_pos = Strategy::i().getPosition( agent->world().self().unum() );
		if( Bhv_Mark( "mark" , home_pos ).execute( agent ) )
			return;
	}
    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    rcsc::Vector2D new_target = target_point;

    rcsc::Vector2D ball_to_target = target_point - agent->world().ball().pos();
    double target2ball_dist = ball_to_target.r();
    if ( target2ball_dist < 11.0 )
    {
        new_target.x = agent->world().ball().pos().x - std::sqrt( 11.0 * 11.0 - ball_to_target.y * ball_to_target.y );
        if ( new_target.x < -45.0 )
        {
            new_target = agent->world().ball().pos();
            new_target += ball_to_target.setLengthVector( 11.0 );
            target2ball_dist = 11.0;
        }
    }

    if ( agent->world().self().pos().absY() > rcsc::ServerParam::i().pitchHalfWidth()
         && agent->world().self().pos().x < agent->world().ball().pos().x + 11.0
         && agent->world().ball().pos().x < agent->world().self().pos().x )
    {
        // subtarget may be out of area.
        // at first, player should back to safety area
        new_target = agent->world().ball().pos();
        new_target.x += 12.0;
        new_target.y *= 0.9;
    }
    else
    {
        // recursive search
        //new_target = get_avoid_circle_point( agent->world(), new_target, 0 );
        new_target = get_avoid_circle_point2( agent->world(), new_target );
    }
    
    if( agent->world().gameMode().type() == rcsc::GameMode::GoalieCatch_ )
		new_target.x = std::min( new_target.x , 35.0 );
    
    double dist_thr = agent->world().ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.7 ) dist_thr = 0.7;

    if ( ! Body_GoToPoint( new_target, dist_thr, dash_power ).execute( agent ) )
    {
        //rcsc::Body_TurnToBall().execute( agent );
        //std::cout<<agent->world().self().unum()<<" going => "<<new_target.x<<" , "<<new_target.y<<std::endl;
        static rcsc::AngleDeg deg = 0.0;
        deg += 60.0;
	    if( deg == 360.0 )
			deg = 0.0;
	    rcsc::Body_TurnToAngle( deg ).execute( agent );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}


/*------------------------------------------------------------*/
bool
Bhv_SetPlay::is_delaying_tactics_situation( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const int real_set_play_count = wm.time().cycle() - wm.lastSetPlayStartTime().cycle();
    const int wait_buf = ( wm.gameMode().type() == rcsc::GameMode::GoalKick_ ? 15 : 2 );

    if ( real_set_play_count >= rcsc::ServerParam::i().dropBallTime() - wait_buf )
    {
        return false;
    }

    int our_score = ( wm.ourSide() == rcsc::LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == rcsc::LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );

    if ( wm.audioMemory().recoveryTime().cycle() >= wm.time().cycle() - 10 )
    {
        if ( our_score > opp_score )
        {
            return true;
        }
    }

    long cycle_thr = 5500;

    if ( wm.time().cycle() < cycle_thr )
    {
        return false;
    }

    if ( our_score > opp_score && our_score - opp_score <= 1 )
    {
        return true;
    }
    
    return false;
}



bool
Bhv_SetPlay::is_kicker( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.gameMode().type() == rcsc::GameMode::GoalieCatch_
         && wm.gameMode().side() == wm.ourSide()
         && ! wm.self().goalie() )
    {
        return false;
    }

    int kicker_unum = 0;
    double min_dist2 = std::numeric_limits< double >::max();
    int second_kicker_unum = 0;
    double second_min_dist2 = std::numeric_limits< double >::max();
    for ( int unum = 1; unum <= 11; ++unum )
    {
        if ( unum == wm.teammateGoalieUnum() ) continue;

        rcsc::Vector2D home_pos = Strategy::i().getPosition( unum );
        if ( ! home_pos.isValid() ) continue;

        double d2 = home_pos.dist2( wm.ball().pos() );
        if ( d2 < second_min_dist2 )
        {
            second_kicker_unum = unum;
            second_min_dist2 = d2;

            if ( second_min_dist2 < min_dist2 )
            {
                std::swap( second_kicker_unum, kicker_unum );
                std::swap( second_min_dist2, min_dist2 );
            }
        }
    }

    const rcsc::AbstractPlayerObject * kicker = static_cast< rcsc::AbstractPlayerObject* >( 0 );
    const rcsc::AbstractPlayerObject * second_kicker = static_cast< rcsc::AbstractPlayerObject* >( 0 );

    if ( kicker_unum != 0 )
    {
        kicker = wm.teammate( kicker_unum );
    }

    if ( second_kicker_unum != 0 )
    {
        second_kicker = wm.teammate( second_kicker_unum );
    }

    if ( ! kicker )
    {
        if ( ! wm.teammatesFromBall().empty()
             && wm.teammatesFromBall().front()->distFromBall() < wm.ball().distFromSelf() * 0.9 )
        {
            return false;
        }
        return true;
    }

    if ( kicker
         && second_kicker
         && ( kicker->unum() == wm.self().unum()
              || second_kicker->unum() == wm.self().unum() ) )
    {
        if ( std::sqrt( min_dist2 ) < std::sqrt( second_min_dist2 ) * 0.95 )
        {
            return ( kicker->unum() == wm.self().unum() );
        }
        else if ( kicker->distFromBall() < second_kicker->distFromBall() * 0.95 )
        {
            return ( kicker->unum() == wm.self().unum() );
        }
        else if ( second_kicker->distFromBall() < kicker->distFromBall() * 0.95 )
        {
            return ( second_kicker->unum() == wm.self().unum() );
        }
        else  if ( ! wm.teammatesFromBall().empty()
                   && wm.teammatesFromBall().front()->distFromBall() < wm.self().distFromBall() * 0.95 )
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return ( kicker->unum() == wm.self().unum() );
}



namespace {

bool
can_go_to( const int count,
           const rcsc::WorldModel & wm,
           const rcsc::Circle2D & ball_circle,
           const rcsc::Vector2D & target_point )
{
    rcsc::Segment2D move_line( wm.self().pos(), target_point );

    int n_intersection = ball_circle.intersection( move_line, NULL, NULL );

    if ( n_intersection == 0 )
    {
        return true;
    }

    if ( n_intersection == 1 )
    {
        rcsc::AngleDeg angle = ( target_point - wm.self().pos() ).th();
        if ( ( angle - wm.ball().angleFromSelf() ).abs() > 80.0 )
        {
            return true;
        }
    }

    return false;
}

} // end noname namespace


rcsc::Vector2D
Bhv_SetPlay::get_avoid_circle_point2( const rcsc::WorldModel & wm, const rcsc::Vector2D & target_point )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    const double avoid_radius = SP.centerCircleR() + wm.self().playerType().playerSize();
    const rcsc::Circle2D ball_circle( wm.ball().pos(), avoid_radius );


    if ( can_go_to( -1, wm, ball_circle, target_point ) )
    {
        return target_point;
    }

    rcsc::AngleDeg target_angle = ( target_point - wm.self().pos() ).th();
    rcsc::AngleDeg ball_target_angle = ( target_point - wm.ball().pos() ).th();
    bool ball_is_left = wm.ball().angleFromSelf().isLeftOf( target_angle );

    const int ANGLE_DIVS = 6;
    std::vector< rcsc::Vector2D > subtargets;
    subtargets.reserve( ANGLE_DIVS );

    const int angle_step = ( ball_is_left ? 1 : -1 );

    int count = 0;
    int a = angle_step;

    for ( int i = 1; i < ANGLE_DIVS; ++i, a += angle_step, ++count )
    {
        rcsc::AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        rcsc::Vector2D new_target = wm.ball().pos() + rcsc::Vector2D::from_polar( avoid_radius + 1.0, angle );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    a = -angle_step;
    for ( int i = 1; i < ANGLE_DIVS*2; ++i, a -= angle_step, ++count )
    {
        rcsc::AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        rcsc::Vector2D new_target = wm.ball().pos() + rcsc::Vector2D::from_polar( avoid_radius + 1.0, angle );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    return target_point;
}

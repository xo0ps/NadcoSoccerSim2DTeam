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

#include "bhv_set_play_goal_kick.h"
#include "body_direct_pass.h"
#include "body_leading_pass.h"
#include "body_clear_ball.h"
#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"
#include "body_kick_one_step.h"
#include "body_go_to_point.h"
#include "bhv_mark.h"
#include "body_intercept.h"
#include "body_pass.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/body_turn_to_angle.h>

#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_SetPlayGoalKick::execute( rcsc::PlayerAgent * agent )
{
    //if ( isKicker( agent ) )
    if ( Bhv_SetPlay::is_kicker( agent ) )
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

*/
bool
Bhv_SetPlayGoalKick::isKicker( const rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.setplayCount() < 30 )
    {
        return false;
    }

    if ( wm.allTeammates().empty() )
    {
        return true;
    }

	
	const rcsc::AbstractPlayerCont & team = wm.allTeammates();
	const rcsc::AbstractPlayerCont::const_iterator team_end = team.end();
	for( rcsc::AbstractPlayerCont::const_iterator it = team.begin(); it != team_end; it++ )
    {
		
        if ( ! (*it)->goalie()
             && (*it)->distFromBall() < wm.ball().distFromSelf() )
		{
            // exist other kicker
            return false;
        }
    }
    
    
    //if( ! wm.self().goalie() )
	//	return false;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayGoalKick::doKick( rcsc::PlayerAgent * agent )
{
	if ( doSecondKick( agent ) )
    {
        return;
    }
    
    // go to point to kick the ball
    /*
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        return;
    }
    */

    // already kick point
    
    if ( doKickWait( agent ) )
    {
        return;
    }
    
    /*
  
    */

    if ( doPass( agent ) )
    {
        return;
    }

    if ( doKickToFarSide( agent ) )
    {
        return;
    }

	/*
    const rcsc::WorldModel & wm = agent->world();
    int real_set_play_count = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );
    if ( real_set_play_count <= rcsc::ServerParam::i().dropBallTime() - 10 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return;
    }
    

    Body_ClearBall().execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
    */


}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_SetPlayGoalKick::doMove( rcsc::PlayerAgent * agent )
{
	
	if ( doIntercept( agent ) )
    {
        return;
    }
    
	
	if( Bhv_Mark( "markEscape" , M_home_pos ).execute( agent ) )
		return;

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = agent->world().ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    rcsc::Vector2D move_point = M_home_pos;
    move_point.y += agent->world().ball().pos().y * 0.5;
    
    //std::cout<<agent->world().self().unum()<<" going => "<<move_point.x<<" , "<<move_point.y<<std::endl;
    if ( ! Body_GoToPoint( move_point, dist_thr, dash_power ).execute( agent ) )
    {		
		rcsc::Body_TurnToBall().execute( agent );
    }
    
    if ( //agent->world().self().pos().dist( move_point ) > std::max( agent->world().ball().pos().dist( move_point ) * 0.2 , 0.5 ) + 6.0 || 
	agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.7
       )
    {
        //agent->addSayMessage( new rcsc::WaitRequestMessage() );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

/*--------------------------------------------------*/
bool
Bhv_SetPlayGoalKick::doSecondKick( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.ball().pos().x < -rcsc::ServerParam::i().pitchHalfLength() + rcsc::ServerParam::i().goalAreaLength() + 1.0
         && wm.ball().pos().absY() < rcsc::ServerParam::i().goalAreaWidth() * 0.5 + 1.0 )
    {
        return false;
    }

    if ( wm.self().isKickable() )
    {
        if ( doPass( agent ) )
        {
            return true;
        }
    }

    if ( doIntercept( agent ) )
    {
        return true;
    }

    rcsc::Vector2D ball_final = wm.ball().inertiaFinalPoint();
    
    if ( ! Body_GoToPoint( ball_final, 2.0, rcsc::ServerParam::i().maxDashPower() ).execute( agent ) )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 0.0, 0.0 ) ).execute( agent );
    }

    agent->setNeckAction( new rcsc::Neck_ScanField() );

    //return true;
    return false;
}

/*-------------------------------------*/
bool
Bhv_SetPlayGoalKick::doKickWait( rcsc::PlayerAgent * agent )
{
	/*
    const rcsc::WorldModel & wm = agent->world();

    const int real_set_play_count = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

    if ( real_set_play_count >= rcsc::ServerParam::i().dropBallTime() - 10 )
    {
        return false;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    // face to the ball
    if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 3.0 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() <= 6 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() <= 30 && wm.teammatesFromSelf().empty() )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.setplayCount() >= 15
         && wm.seeTime() == wm.time()
         && wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.6 )
    {
        return false;
    }

    if ( wm.setplayCount() <= 3
         || wm.seeTime() != wm.time()
         || wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.9 )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }
    
    

    static int S_scan_count = -5;

    // already ball point

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D face_point( 40.0, 0.0 );
    const rcsc::AngleDeg face_angle = ( face_point - wm.self().pos() ).th();


    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToPoint( rcsc::Vector2D( 0.0, 0.0 ) ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( ( face_angle - wm.self().body() ).abs() > 5.0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( S_scan_count < 0 )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( S_scan_count < 10 && wm.teammatesFromSelf().empty() )
    {
        S_scan_count++;
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    if ( wm.time().stopped() != 0 )
    {
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    S_scan_count = -5;
    

    return false;
    
    static int s_second_wait_count = 0;
    const long time_diff = agent->world().time().cycle() - agent->effector().getCatchTime().cycle();
    if ( time_diff <= 2 )
    {
        s_second_wait_count = 0;
        rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    
    // after first move
    // check stamina recovery or wait teammate
    rcsc::Rect2D our_pen( rcsc::Vector2D( -52.5, -40.0 ), rcsc::Vector2D( -36.0, 40.0 ) );
    if ( time_diff < 50
         || agent->world().setplayCount() < 3
         || time_diff < rcsc::ServerParam::i().dropBallTime() - 70
       )
    {
        rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    s_second_wait_count++;

    
    // wait see info
    if ( s_second_wait_count < rcsc::ServerParam::i().dropBallTime() - 5 || agent->world().seeTime() != agent->world().time() )
    {
        rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    s_second_wait_count = 0;
    */
    
    rcsc::AngleDeg ball_place_angle = ( agent->world().ball().pos().y > 0.0 ? -90.0 : 90.0 );

    if ( Bhv_PrepareSetPlayKick( ball_place_angle, 10 ).execute( agent ) )
    {
        // go to kick point
        return true;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        rcsc::Body_TurnToBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    return false;
}

/*------------------------------------------------*/
bool
Bhv_SetPlayGoalKick::doPass( rcsc::PlayerAgent * agent )
{
	
    rcsc::Vector2D target_point;
	const rcsc::WorldModel & wm = agent->world();
    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont & mates = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();

    std::vector< rcsc::Vector2D > six_opps;
    for( int i = 0 ; i < 6 ; i++ )
        six_opps.push_back( opps[i]->pos() );
    
    rcsc::Vector2D tmp_2D;
    for( int i = 0 ; i < 5 ; i++ )
        for( int j = i ; j < 6 ; j++ )
            if( six_opps[i].y < six_opps[j].y )
            {
                tmp_2D = six_opps[i];
                six_opps[i] = six_opps[j];
                six_opps[j] = tmp_2D;
            }
            
    for( int i = 0 ; i < 5 ; i++ )
    {
        rcsc::Segment2D seg( six_opps[i], six_opps[i+1] );
        rcsc::Line2D line = seg.line();
        if( line.dist( six_opps[i] ) == seg.dist( six_opps[i] ) )
        {
            rcsc::Vector2D target_pos = ( seg.origin() + seg.terminal() ) / 2.0;
            double min_dist = rcsc::ServerParam::i().pitchLength() * 2.0;
            int unum_receiver = 2;
            for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
            {
                if( min_dist > target_pos.dist( (*it)->pos() ) )
                {
                    min_dist = target_pos.dist( (*it)->pos() );
                    unum_receiver = (*it)->unum();
                }
            }
            
            
            double first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(),
                                           rcsc::ServerParam::i().ballSpeedMax() );
            double simed_speed = first_speed;
            double travel_dist = 0.0;
            int elapsed_time = 0;
            while( travel_dist < target_pos.dist( wm.ball().pos() ) )
            {
                elapsed_time++;
                travel_dist += simed_speed;
                simed_speed *= rcsc::ServerParam::i().ballDecay();
            }
            travel_dist -= simed_speed / rcsc::ServerParam::i().ballDecay();
            if( travel_dist > 10 )
                continue;

            rcsc::Vector2D self2tgt = target_pos - wm.ball().pos();
            self2tgt.setLength( travel_dist );
            
            target_pos = wm.ball().pos() + self2tgt;
            double dist = 100;
			const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_pos , 10 , &dist );
			if( dist < 5.0 ) continue;
            double ball_speed = Body_Pass::first_speed( agent , target_point , 'd' );
            Body_KickOneStep( target_pos, ball_speed ).execute( agent );
			Body_Pass::say_pass( agent , unum_receiver , target_pos );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
        }
    }
    
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
    {
		if( !(*it) ) continue;
        //if( (*it)->posCount() > 3 ) continue;
        if( (*it)->pos().dist( wm.self().pos() ) < 10.0 ) continue;
        
        /*
		const double ball_decay = rcsc::ServerParam::i().ballDecay();
		const double ball_speed_max = rcsc::ServerParam::i().ballSpeedMax();
		int reach_cycle = (int)(log( 1.0 - ( 1.0 - ball_decay ) * wm.self().pos().dist( (*it)->pos() ) / ball_speed_max ) / log( ball_decay ));
		double ball_speed = (*it)->playerTypePtr()->kickableArea() * 1.5 * pow( 1.0 / ball_decay, reach_cycle - 1 );
		double first_speed = std::min( ball_speed_max, ball_speed );
		target_point = wm.ball().pos() * 0.2 + (*it)->pos() * 0.8;
		*/
		double dist = 100;
        const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_point , 10 , &dist );
        if( dist < 5.0 ) continue;
		if( target_point.x > -35.0 || target_point.absY() > 20.0 )
		{
			double ball_speed = Body_Pass::first_speed( agent , target_point , 'd' );
			if( Body_KickOneStep( target_point, ball_speed ).execute( agent ) )
			{
				Body_Pass::say_pass( agent , (*it)->unum() , target_point );
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
		}
	}

    double ball_speed_max = rcsc::ServerParam::i().ballSpeedMax();
    double best_angle = 90.0 * wm.ball().pos().y / wm.ball().pos().absY();
    for( double angle = -80.0; angle <= 80.0; angle += 10.0 )
    {
		if( fabs( angle ) < fabs( best_angle ) )
        {
            best_angle = angle;
        }
    }
    target_point.x = wm.ball().pos().x + rcsc::AngleDeg( best_angle ).cos();
    target_point.y = wm.ball().pos().y + rcsc::AngleDeg( best_angle ).sin();
    target_point = wm.ball().pos() * 0.2 + target_point * 0.8;
    //double dist = 100.0;
    //bool safe = false;
    //const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_point , 10 , &dist );
    //if( dist < 10.0 ) safe = true;
    //if( safe && target_point.x > -40.0 )
    {
        if( Body_KickOneStep( target_point, rcsc::ServerParam::i().ballSpeedMax() ).execute( agent ) )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
            return true;
        }
    }

    return false;
}

/*---------------------------------------------*/
bool
Bhv_SetPlayGoalKick::doIntercept( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.ball().pos().x < -rcsc::ServerParam::i().pitchHalfLength() + rcsc::ServerParam::i().goalAreaLength() + 1.0
         && wm.ball().pos().absY() < rcsc::ServerParam::i().goalAreaWidth() * 0.5 + 1.0 )
    {
        return false;
    }

    if ( wm.self().isKickable() )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    if ( self_min > mate_min )
    {
        return false;
    }

    rcsc::Vector2D trap_pos = wm.ball().inertiaPoint( self_min );
    if ( ( trap_pos.x > rcsc::ServerParam::i().ourPenaltyAreaLineX() - 8.0
           && trap_pos.absY() > rcsc::ServerParam::i().penaltyAreaHalfWidth() - 5.0 )
         || wm.ball().vel().r2() < std::pow( 0.5, 2 ) )
    {
        Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    return false;
}

/*-----------------------------------------*/
bool
Bhv_SetPlayGoalKick::doKickToFarSide( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D target_point( -30.0 , 20.0 );
    if ( wm.ball().pos().y < 0.0 )
    {
        target_point.y *= -1.0;
    }

    double ball_move_dist = wm.ball().pos().dist( target_point );
    double ball_first_speed = rcsc::calc_first_term_geom_series_last( 1.8, ball_move_dist, rcsc::ServerParam::i().ballDecay() );
    ball_first_speed = std::min( rcsc::ServerParam::i().ballSpeedMax(), ball_first_speed );
    ball_first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), ball_first_speed );

    Body_KickOneStep( target_point, ball_first_speed ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
    return true;
}

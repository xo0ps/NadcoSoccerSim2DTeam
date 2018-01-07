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

#include "bhv_goalie_free_kick.h"
#include "body_clear_ball.h"
#include "bhv_goalie_basic_move.h"
#include "body_kick_one_step.h"
#include "body_pass.h"

#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>

#include <rcsc/common/server_param.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/geom/segment_2d.h>

/*-------------------------------------------------------------------*/
bool
Bhv_GoalieFreeKick::execute( rcsc::PlayerAgent * agent )
{
    static bool s_first_move = false;
    static bool s_second_move = false;
    static int s_second_wait_count = 0;

    if ( agent->world().gameMode().type() != rcsc::GameMode::GoalieCatch_
         || agent->world().gameMode().side() != agent->world().ourSide()
         || ! agent->world().self().isKickable() )
    {
        Bhv_GoalieBasicMove().execute( agent );
        return true;
    }


    const long time_diff = agent->world().time().cycle() - agent->effector().getCatchTime().cycle();
    //- M_catch_time.cycle();

    // reset flags & wait
    if ( time_diff <= 2 )
    {
        s_first_move = false;
        s_second_move = false;
        s_second_wait_count = 0;

        doWait( agent );
        return true;
    }

    // first move
    if ( ! s_first_move )
    {
        //rcsc::Vector2D move_target( rcsc::ServerParam::i().ourPenaltyAreaLine() - 0.8, 0.0 );
        rcsc::Vector2D move_target( rcsc::ServerParam::i().ourPenaltyAreaLineX() - 2.5,
                                    agent->world().ball().pos().y > 0.0 ? 10.0 : -10.0 );
        //rcsc::Vector2D move_target( -45.0, 0.0 );
        
		const rcsc::PlayerPtrCont & mates = agent->world().teammatesFromSelf();
		const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();
		for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
		{
			if( !(*it) ) continue;
			if( move_target.dist( (*it)->pos() ) < 3.0 )
			{
				move_target.x -= 1.5;
			}
		}
		
        s_first_move = true;
        s_second_move = false;
        s_second_wait_count = 0;
        agent->doMove( move_target.x, move_target.y );
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
        doWait( agent );
        return true;
    }

    s_second_wait_count++;

    //second move
    if ( ! s_second_move )
    {
        //rcsc::Vector2D move_target( rcsc::ServerParam::i().ourPenaltyAreaLine() - 0.8, 0.0 );
        rcsc::Vector2D move_target( rcsc::ServerParam::i().ourPenaltyAreaLineX() - 1.5,
                                    agent->world().ball().pos().y > 0.0 ? -16.0 : 16.0 );
		if( agent->world().time().cycle() % 30 == 0 )
		{
			move_target.assign( rcsc::ServerParam::i().ourPenaltyAreaLineX() - 1.5,
                                    agent->world().ball().pos().y > 0.0 ? -16.0 : 16.0 );
	        //rcsc::Vector2D move_target( -45.0, 0.0 );
	        /*
			const rcsc::PlayerPtrCont & mates = agent->world().teammatesFromSelf();
			const rcsc::PlayerPtrCont::const_iterator mates_end = mates.end();
			for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
			{
				if( !(*it) ) continue;
				if( move_target.dist( (*it)->pos() ) < 3.0 )
				{
					move_target.x -= 1.5;
				}
			}
			*/
			
			s_second_move = true;
	        s_second_wait_count = 0;
	        agent->doMove( move_target.x, move_target.y );
	        agent->setNeckAction( new rcsc::Neck_ScanField() );
	        s_first_move = true;
			return true;
		}
		else
		{
			doWait( agent );
			s_first_move = true;
			return true;
	    }
    }

    // wait see info
    if ( s_second_wait_count < 5 || agent->world().seeTime() != agent->world().time() )
    {
        doWait( agent );
        return true;
    }

    s_first_move = true;
    s_second_move = true;
    s_second_wait_count = 0;

    // register kick intention
    doKick( agent );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_GoalieFreeKick::doKick( rcsc::PlayerAgent * agent )
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
			if( dist < 10.0 ) continue;
            Body_KickOneStep( target_pos, first_speed ).execute( agent );
			Body_Pass::say_pass( agent , unum_receiver , target_pos );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return;
        }
    }
    
    for( rcsc::PlayerPtrCont::const_iterator it = mates.begin(); it != mates_end; ++it )
    {
		if( !(*it) ) continue;
        //if( (*it)->posCount() > 3 ) continue;
        if( (*it)->pos().dist( wm.self().pos() ) < 10.0 ) continue;
        
		const double ball_decay = rcsc::ServerParam::i().ballDecay();
		const double ball_speed_max = rcsc::ServerParam::i().ballSpeedMax();
		int reach_cycle = (int)(log( 1.0 - ( 1.0 - ball_decay ) * wm.self().pos().dist( (*it)->pos() ) / ball_speed_max ) / log( ball_decay ));
		double ball_speed = (*it)->playerTypePtr()->kickableArea() * 1.5 * pow( 1.0 / ball_decay, reach_cycle - 1 );
		double first_speed = std::min( ball_speed_max, ball_speed );
		target_point = wm.ball().pos() * 0.2 + (*it)->pos() * 0.8;
		double dist = 100;
        const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_point , 10 , &dist );
        if( dist < 10.0 ) continue;
		if( target_point.x > -35.0 || target_point.absY() > 20.0 )
		{
			if( Body_KickOneStep( target_point, first_speed ).execute( agent ) )
			{
				Body_Pass::say_pass( agent , (*it)->unum() , target_point );
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return;
			}
		}
	}

	/*
    if  ( Body_DirectPass("playOn").evaluate( agent ,
                                          & target_point,
                                          NULL,
                                          NULL )
          && target_point.dist( rcsc::Vector2D(-50.0, 0.0) ) > 20.0
          )
    {
        double opp_dist = 100.0;
        const rcsc::PlayerObject * opp
            = agent->world().getOpponentNearestTo( target_point,
                                                   10,
                                                   & opp_dist );
        if ( ! opp
             || opp_dist > 5.0 )
        {
            Body_DirectPass("playOn").execute( agent );
            agent->setNeckAction( new rcsc::Neck_ScanField() );
            return;
        }
    }
    */

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
    double dist = 100.0;
    bool safe = false;
    const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_point , 10 , &dist );
    if( dist < 10.0 ) safe = true;
    if( safe && target_point.x > -40.0 )
    {
        if( Body_KickOneStep( target_point, ball_speed_max ).execute( agent ) )
        {
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
            return;
        }
    }
    Body_ClearBall().execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );

}

/*-------------------------------------------------------------------*/
/*!

*/
void
Bhv_GoalieFreeKick::doWait( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D face_target( 0.0, 0.0 );
    static rcsc::AngleDeg deg = 0.0;

    if ( wm.self().pos().x > rcsc::ServerParam::i().ourPenaltyAreaLineX()
         - rcsc::ServerParam::i().ballSize() - wm.self().playerType().playerSize() - 0.5 )
        face_target.assign( rcsc::ServerParam::i().ourPenaltyAreaLineX() - 1.0, 0.0 );

    rcsc::Body_TurnToPoint( face_target ).execute( agent );
    deg += 60.0;
    if( deg == 360.0 )
		deg = 0.0;
    rcsc::Body_TurnToAngle( deg ).execute( agent );
    agent->setNeckAction( new rcsc::Neck_ScanField() );
}

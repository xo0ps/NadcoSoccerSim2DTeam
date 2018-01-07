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

#include "bhv_mark.h"
#include "strategy.h"
#include "sample_player.h"
#include "bhv_offmid_defmid_move.h"
#include "body_go_to_point.h"
#include "body_intercept.h"
#include "body_pass.h"
#include "bhv_through_pass_cut.h"
#include "bhv_set_play.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/body_turn_to_ball.h>

#include <rcsc/geom/sector_2d.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_Mark::execute( rcsc::PlayerAgent * agent )
{

    if( agent->world().self().isFrozen() )
        return false;

	if( agent->world().self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.6 )
		return false;

	if( M_mode == "markPass" )
	{
		if( markPassLine( agent ) )
		{
			return true;
		}
	}
	if( M_mode == "markEscape" )
	{
		//markEscape( agent );
		if( markEscape( agent ) )
		{
			return true;
		}
	}
	if( M_mode == "mark" )
	{
		if( mark( agent ) )
		{
			return true;
		}
	}
	
	rcsc::Body_TurnToBall().execute( agent );
	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
	return false;
}


/*-----------------------------------------------------------------*/

bool Bhv_Mark::mark( rcsc::PlayerAgent * agent )
{
	
	//if ( ! SamplePlayer::instance().mark() )
	if ( ! Strategy::i().mark() )
    {
		return false;
    }
    
	const rcsc::WorldModel & wm = agent->world();
	
	if( wm.gameMode().side() == wm.ourSide() )
	{
		bool mark = false;
		const rcsc::AbstractPlayerCont & team1 = wm.allTeammates();
		const rcsc::AbstractPlayerCont::const_iterator team_end1 = team1.end();
		for( rcsc::AbstractPlayerCont::const_iterator it = team1.begin(); it != team_end1; it++ )
	    {
	        if ( (*it)->distFromBall() < wm.ball().distFromSelf() )
	        {
				//I am not kicker
				mark = true;
				break;
	        }
	    }
	    
		if(! mark )
			return false;
	}
	/*
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();

    if ( ( ! wm.existKickableTeammate() && self_min < 4 ) || ( self_min < mate_min ) 
	|| ( ! wm.existKickableOpponent() && ! wm.existKickableTeammate() && self_min < mate_min ) )
    {
        Body_Intercept().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }
    */

	/*
	rcsc::Vector2D point( 100 , 100 );
    if ( wm.audioMemory().passTime().cycle() >= wm.time().cycle() - 1
         && ( ! wm.audioMemory().pass().empty() )
       )
    {
		point = wm.audioMemory().pass().front().receive_pos_;
		if( wm.audioMemory().pass().front().receiver_ == wm.self().unum() )
		{
			Body_GoToPoint( point, wm.self().pos().dist( point ) * 0.05 , rcsc::ServerParam::i().maxDashPower() ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
			return true;
		}
	}
	*/
		
    double dist_opp_to_home = 100.0;
    //const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( M_home_pos , 10 , &dist_opp_to_home );
    //const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( wm.self().pos() , 10 , &dist_opp_to_home );
    /*
    if( wm.gameMode().type() == rcsc::GameMode::KickIn_ 
     || wm.gameMode().type() == rcsc::GameMode::GoalKick_
     || wm.gameMode().type() == rcsc::GameMode::FreeKick_ 
     || wm.gameMode().type() == rcsc::GameMode::CornerKick_ 
     || wm.gameMode().type() == rcsc::GameMode::IndFreeKick_ )
    {
		opp = wm.getOpponentNearestTo( wm.self().pos() , 10 , &dist_opp_to_home );
	}
	*/

    rcsc::Vector2D nearest_opp_pos( 100 , 100 );
    /*
    if ( opp )
    {
        nearest_opp_pos = opp->pos();
    }
	else
	{
		return false;
	}
	*/
	
	rcsc::Vector2D point( 100 , 100 );
	bool found = false;
	rcsc::AngleDeg deg;
	//const rcsc::PlayerPtrCont & team = wm.opponentsFromSelf();
	const rcsc::PlayerPtrCont & team = wm.opponentsFromBall();
	const rcsc::PlayerPtrCont::const_iterator end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator it = team.begin(); it != end; ++it )
	{
		if(! (*it) )
			continue;
		if( (*it)->goalie() )
			continue;
		rcsc::Vector2D point = agent->effector().getPointtoPos();
		if( (*it)->pos().dist( point ) < 3.0 )
			//continue;
		if( (*it)->pos().dist( wm.ball().pos() ) > 50 )
			continue;
		if( (*it)->pos().dist( wm.ball().pos() ) < 4.0 )
			continue;
		//if( (*it)->pos().dist( M_home_pos ) > 15.0 )
		//	continue;
		if( (*it)->pos().x > 36.0 && (*it)->pos().absY() < 17.0 )
			continue;
		//if( (*it)->pos().dist( point ) < 2.5 )
		//	continue;
		rcsc::Circle2D blocked( (*it)->pos() , 2.0 );
		if( wm.existTeammateIn( blocked , 10 , false ) )
			continue;
		rcsc::AngleDeg deg = ( wm.ball().pos() - (*it)->pos() ).th();
		//rcsc::Sector2D pass( wm.ball().pos() , 0.5 , wm.ball().pos().dist( (*it)->pos() ) , deg - 15 , deg + 15 );
		rcsc::Vector2D a = (*it)->pos() - rcsc::Vector2D::polar2vector( 1.5 , deg + 90.0 );
		rcsc::Vector2D b = (*it)->pos() + rcsc::Vector2D::polar2vector( 1.5 , deg + 90.0 );
		rcsc::Triangle2D pass( wm.ball().pos() , a , b );
		if( wm.existTeammateIn( pass , 10 , false ) )
		{
			continue;
		}
		nearest_opp_pos = (*it)->pos();
		deg = (*it)->body();
		found  = true;
		break;
	}
	
	if(! found )
		return false;
		
	int unum = 0;
	double dist = 1000.0;
	//bool found1 = false;
	const rcsc::AbstractPlayerCont & team1 = wm.allTeammates();
	const rcsc::AbstractPlayerCont::const_iterator team_end = team1.end();
	for( rcsc::AbstractPlayerCont::const_iterator  tm = team1.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().dist( nearest_opp_pos ) < dist )
		{
			dist = (*tm)->pos().dist( nearest_opp_pos );
			unum = (*tm)->unum();
		}
		
		/*
		if( (*tm)->isSelf() )
		{
			found1 = true;
			break;
		}
		*/
	}		
	/*
	if( ! found1 )
		return false;
	*/
	
	if( unum == 0 )
		return false;
	
	if( wm.self().unum() != unum )
		return false;
	
	//std::cout<<wm.self().unum()<<" mark 1"<<std::endl;
	
    rcsc::AngleDeg block_angle = ( wm.ball().pos() - nearest_opp_pos ).th();
    rcsc::Vector2D block_point = nearest_opp_pos + rcsc::Vector2D::polar2vector( 1.0 , block_angle );

	rcsc::Circle2D blocked( block_point , 2.0 );
	if( wm.existTeammateIn( blocked , 10 , true ) )
		return false;
	
	/*
	if( wm.self().pos().dist( block_point ) > 20 )
	{
		rcsc::Line2D pass_route( wm.ball().pos() , nearest_opp_pos );
		rcsc::Line2D my_dist( pass_route.perpendicular( wm.self().pos() ) );
		block_point = my_dist.intersection( pass_route );
	}
	*/
	
	/*
	if( block_point.dist( M_home_pos ) > 20 )
		return false;
	*/
	
    //double dash_power = rcsc::ServerParam::i().maxDashPower() * 0.8;
    //double dash_power = rcsc::ServerParam::i().maxDashPower();
    double dash_power = wm.self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
    //double dist_thr = block_point.dist( wm.self().pos() ) * 0.05;
    //if ( dist_thr < 0.5 )
	//	dist_thr = 0.5;
	
	double dist_thr = 0.5;
	
	std::vector< rcsc::Vector2D >dropballs;
	dropballs.push_back( rcsc::Vector2D( 36 , 20 ) );
	dropballs.push_back( rcsc::Vector2D( 36 , -20 ) );
	dropballs.push_back( rcsc::Vector2D( -36 , 20 ) );
	dropballs.push_back( rcsc::Vector2D( -36 , -20 ) );
	dropballs.push_back( rcsc::Vector2D( 0 , 0 ) );
	
	rcsc::Vector2D nearest;
	double min = 1000.0;
	for( uint i = 0 ; i < dropballs.size() ; i++ )
	{
		if( wm.self().pos().dist( dropballs[i] ) < min )
		{
			nearest = dropballs[i];
			min = wm.self().pos().dist( dropballs[i] );
		}
	}
	
	Body_Pass::say_pass( agent , wm.self().unum() , block_point );
	agent->setArmAction( new rcsc::Arm_PointToPoint( block_point ) );
    if ( ! Body_GoToPoint( block_point, dist_thr , dash_power ).execute( agent ) )
    {
		rcsc::Body_TurnToAngle( deg ).execute( agent );
		if( wm.time().cycle() % 2 )
			agent->setNeckAction( new rcsc::Neck_TurnToPoint( nearest ) );
		else
			agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return true;
	}
	
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    //std::cout<<"["<<wm.time().cycle()<<"] "<<wm.self().unum()<<" is marking"<<std::endl;
    return true;
    
}

/*-----------------------------------------------------------------*/

bool Bhv_Mark::markPassLine( rcsc::PlayerAgent * agent )
{
	//return false;
	
	const rcsc::WorldModel & wm = agent->world();
	
	/*
	if( wm.self().isKickable() )
		return false;
	*/
	
	if( wm.existKickableTeammate() )
		return false;
	
	if( wm.self().isFrozen() )
		return false;
	
	if( ! wm.existKickableOpponent() )
		return false;
	
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();

    if ( ( ! wm.existKickableTeammate() && self_min < 4 ) || ( self_min < mate_min ) 
	|| ( ! wm.existKickableOpponent() && ! wm.existKickableTeammate() && self_min < mate_min ) )
    {
        Body_Intercept( ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToBall() );
        return true;
    }

	/*
	if( wm.self().pos().dist( wm.ball().pos() ) > 20 )
		return false;
	*/
	if( wm.self().pos().x > 0 )
		return false;
	
	const rcsc::PlayerObject * opp = wm.interceptTable()->fastestOpponent();
	if( ! opp || opp->isGhost() )
		return false;
		
	rcsc::Vector2D kickable = opp->pos();
	rcsc::Vector2D block_point( 100 , 100 );
	
	//std::cout<<"mark pass 1"<<std::endl;
	rcsc::AngleDeg deg;
	const rcsc::PlayerPtrCont & team = wm.opponentsFromSelf();
	const rcsc::PlayerPtrCont::const_iterator end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator it = team.begin(); it != end; ++it )
	{
		if(! (*it) || (*it)->isGhost() )
			continue;
		if( (*it)->pos().x > 0.0 )
			continue;
		if( (*it) == opp )
			continue;
		rcsc::AngleDeg deg = ( kickable - (*it)->pos() ).th();		
		block_point = (*it)->pos() + rcsc::Vector2D::polar2vector( 1.0 , deg );
		/*
		rcsc::Sector2D pass( wm.ball().pos() , 0.0 , wm.ball().pos().dist( wm.self().pos() ) , deg - 16 , deg + 16 );
		if( wm.existTeammateIn( pass , 10 , false ) )
			continue;
		if( wm.self().pos().dist( block_point ) > 10.0 )
		{
			rcsc::Line2D pass_route( kickable , (*it)->pos() );
			rcsc::Line2D my_dist( pass_route.perpendicular( wm.self().pos() ) );
			block_point = rcsc::Line2D::intersection( pass_route , my_dist );
		}
		*/
		if( block_point.absX() > 48 || block_point.absY() > 32 )
			continue;
		deg = (*it)->body();
		break;
	}
	
	if( block_point.x == 100 )
		return false;
	
	//std::cout<<"mark pass 2"<<std::endl;

    double dash_power = rcsc::ServerParam::i().maxDashPower();
	
    double dist_thr = block_point.dist( wm.ball().pos() ) * 0.05;
    if ( dist_thr < 0.2 )
		dist_thr = 0.2;
	Body_Pass::say_pass( agent , wm.self().unum() , block_point );
	
    if ( ! Body_GoToPoint( block_point, dist_thr , dash_power ).execute( agent ) )
    {
		rcsc::Body_TurnToAngle( deg ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}
	
	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
	return false;
	
}

/*-----------------------------------------------------------------*/

bool Bhv_Mark::markEscape( rcsc::PlayerAgent * agent )
{
	
	if ( ! Strategy::i().mark_escape() )
    {
		return false;
    }
    
    const rcsc::WorldModel & wm = agent->world();

	rcsc::Vector2D target_point = M_home_pos;
    const rcsc::PlayerObject * nearest_opp = wm.getOpponentNearestToSelf( 5 );

    if ( nearest_opp && nearest_opp->distFromSelf() < 3.0 )
    {
        rcsc::Vector2D add_vec = ( wm.ball().pos() - target_point );
        add_vec.setLength( 3.0 );

        if ( wm.time().cycle() % 40 < 30 )
            target_point += add_vec.rotatedVector( 90.0 );
        else
            target_point += add_vec.rotatedVector( -90.0 );

        target_point.x = rcsc::min_max( - rcsc::ServerParam::i().pitchHalfLength(), target_point.x, rcsc::ServerParam::i().pitchHalfLength() );
        target_point.y = rcsc::min_max( - rcsc::ServerParam::i().pitchHalfWidth(), target_point.y, rcsc::ServerParam::i().pitchHalfWidth() );
    }
	
	double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
	if ( ! Body_GoToPoint( target_point, 1.0, dash_power ).execute( agent ) )
    {
		static rcsc::AngleDeg deg = 0.0;
        deg += 60.0;
	    if( deg == 360.0 )
			deg = 0.0;
	    rcsc::Body_TurnToAngle( deg ).execute( agent );
	    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
    }
    
    agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
	return true;
    /*
	if( wm.self().pos().dist( wm.ball().pos() ) > 30 )
	{
        rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return true;
	}
	
	rcsc::AngleDeg deg1 = ( wm.ball().pos() - wm.self().pos() ).th();
	rcsc::Vector2D a = wm.self().pos() - rcsc::Vector2D::polar2vector( 1.5 , deg1 + 90.0 );
	rcsc::Vector2D b = wm.self().pos() + rcsc::Vector2D::polar2vector( 1.5 , deg1 + 90.0 );
	rcsc::Triangle2D pass( wm.ball().pos() , a , b );
	if( wm.existOpponentIn( pass , 10 , true ) )
		return mark( agent );
	
	//if( Bhv_ThroughPassCut().execute( agent ) )
	//	return true;
	
	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
	return false;
	*/
/* 	
 * OLD MARK ESCAPE => NEED TO CHECK
	
	rcsc::AngleDeg deg = ( wm.ball().pos() - wm.self().pos() ).th();
	rcsc::Vector2D a = wm.self().pos() - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = wm.self().pos() + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D pass( wm.ball().pos() , a , b );
	//rcsc::Sector2D pass( wm.ball().pos() , 0.0 , wm.ball().pos().dist( wm.self().pos() ) , deg - 21 , deg + 21 );
	if( ! wm.existOpponentIn( pass , 10 , true ) )
	{
        rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		//std::cout<<"mark escape 0.5"<<std::endl;
		return true;
	}
	
	//std::cout<<"mark escape 1"<<std::endl;
	
	bool stop = false;
	double step = 1.0;
	rcsc::Vector2D move_point = wm.self().pos();
	while( ! stop && ( move_point.dist( wm.self().pos() ) < 20.0 ) )
	{
		rcsc::AngleDeg deg = ( move_point - wm.ball().pos() ).th() + 90.0;
		move_point += rcsc::Vector2D::polar2vector( step , deg );
		if( move_point.absX() > 52 || move_point.absY() > 33.5 )
		{
			step = -1.0;
			continue;
		}
		rcsc::AngleDeg angle = ( move_point - wm.ball().pos() ).th();
		//rcsc::Sector2D pass_route( wm.ball().pos() , 0.0 , wm.ball().pos().dist( move_point ) , angle - 16 , angle + 16 );
		rcsc::Vector2D a = wm.self().pos() - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
		rcsc::Vector2D b = wm.self().pos() + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
		rcsc::Triangle2D pass_route( wm.ball().pos() , a , b );
		if( ! wm.existOpponentIn( pass , 10 , true ) )
			stop = true;
	}
	
	static rcsc::Vector2D my_pos = wm.self().pos();
	static rcsc::GameTime lastTime = wm.time();
	if( lastTime.cycle() < wm.time().cycle() - 200 )
	{
		std::cout<<wm.self().unum()<<" : mark escape expired"<<std::endl;
		my_pos = wm.self().pos();
		lastTime = wm.time();
		return false;
	}

	std::cout<<wm.self().unum()<<" : mark escape 2"<<std::endl;
	rcsc::Vector2D move_point = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , deg + 90 );
	double dash_power = rcsc::ServerParam::i().maxDashPower() * 0.8;
	
    double dist_thr = move_point.dist( wm.self().pos() ) * 0.05;
    if ( dist_thr < 0.2 )
		dist_thr = 0.2;
	
	if ( ! Body_GoToPoint( move_point, dist_thr , dash_power ).execute( agent ) )
    {
		std::cout<<wm.self().unum()<<" : mark escape 3"<<std::endl;
		rcsc::Body_TurnToBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}

*/


/* 	NEW MARK ESCAPE
		
	bool found = false;
	rcsc::AngleDeg deg;
	rcsc::Vector2D nearest_opp_pos;
	const rcsc::PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end();
	for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(); it != end; ++it )
	{
		if(! (*it) )
			continue;
		if( (*it)->pos().dist( wm.ball().pos() ) > 40 )
			continue;
		if( (*it)->pos().dist( wm.ball().pos() ) < 2.0 )
			continue;
		if( (*it)->pos().x > 40 && (*it)->pos().absY() < 17 )
			continue;
		rcsc::Circle2D blocked( (*it)->pos() , 2.0 );
		if( wm.existTeammateIn( blocked , 10 , true ) )
			continue;
		nearest_opp_pos = (*it)->pos();
		deg = (*it)->body();
		found  = true;
		break;
	}
	
	if(! found )
		return false;
	
    rcsc::AngleDeg block_angle = ( wm.ball().pos() - nearest_opp_pos ).th();
    rcsc::Vector2D block_point = nearest_opp_pos + rcsc::Vector2D::polar2vector( 1.0 , block_angle );
		
    double dash_power = rcsc::ServerParam::i().maxDashPower() * 0.8;
    double dist_thr = block_point.dist( wm.self().pos() ) * 0.05;
    if ( dist_thr < 0.5 )
		dist_thr = 0.5;
											 
    if ( ! Body_GoToPoint( block_point, dist_thr , dash_power ).execute( agent ) )
    {
		rcsc::Body_TurnToAngle( deg ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
		return true;
	}
	
	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	return false;
*/
}

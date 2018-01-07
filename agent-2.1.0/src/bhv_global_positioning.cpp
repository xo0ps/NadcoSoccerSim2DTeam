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

#include "strategy.h"
#include "bhv_global_positioning.h"
#include "body_go_to_point.h"
#include "body_pass.h"
#include "bhv_cross_move.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_GlobalPositioning::execute( rcsc::PlayerAgent * agent )
{

	rcsc::Vector2D pos = M_home_pos;
	rcsc::Vector2D ball_pos = agent->world().ball().pos();
	
	double dash_power = Bhv_CrossMove::get_dash_power( agent );
	
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_Cross
	 || Strategy::get_ball_area( ball_pos ) == Strategy::BA_CrossBlock )
	{
		pos = cross( agent );
		dash_power = agent->world().self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
	}
	else
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_DribbleAttack )
	{
		pos = dribbleAttack( agent );
		dash_power *= 1.2;
	}
	else
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_DribbleBlock )
	{
		pos = dribbleBlock( agent );
		//dash_power *= 1.2;
	}
	else
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_ShootChance )
	{
		pos = shootchance( agent );
		dash_power *= 1.2;
	}
	else
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_DefMidField )
	{
		pos = defmid( agent );
		//dash_power *= 1.2;
	}
	else
	if( Strategy::get_ball_area( ball_pos ) == Strategy::BA_OffMidField )
	{
		//pos = offmid( agent );
		pos = defmid( agent );
		dash_power *= 1.2;
	}
	else
	{
		pos = M_home_pos;
	}
	
    //pos = M_home_pos;
    if( pos.x < -52.5 )
		pos.x = -50.0;
    if( pos.x > 52.5 )
		pos.x = 50.0;
    if( pos.y < -32 )
		pos.x = -31;
    if( pos.y > 32 )
		pos.y = 31;
    
    if( pos.x < -47.0 && pos.absY() < 17.0 )
		pos.x = std::max( M_home_pos.x , -47.0 );
	
	if( agent->world().self().unum() == 2 || agent->world().self().unum() == 3 )
	if( agent->world().ball().pos().x > agent->world().self().pos().x )
	{
		std::vector< rcsc::Vector2D >opponents;
		const rcsc::PlayerPtrCont & opp_team = agent->world().opponentsFromSelf();
		const rcsc::PlayerPtrCont::const_iterator opp_end = opp_team.end();
		for( rcsc::PlayerPtrCont::const_iterator opp = opp_team.begin(); opp != opp_end; opp++ )
		{
			if( (! (*opp)) )
				continue;
			if( (*opp)->goalie() || (*opp)->unum() > 11 || (*opp)->unum() < 2 )
				continue;
			opponents.push_back( (*opp)->pos() );
		}
		//pos.x = min - 2.0;
		//dash_power = agent->world().self().getSafetyDashPower( rcsc::ServerParam::i().maxDashPower() );
		
		for( uint i = 0 ; i < opponents.size() - 1 ; i++ )
		{
			for( uint j = i ; j < opponents.size() ; j++ )
			{
				if( opponents[i].x > opponents[j].x )
				{
					rcsc::Vector2D tmp = opponents[i];
					opponents[i] = opponents[j];
					opponents[j] = tmp;
				}
			}
		}
		double min = 1000.0;
		rcsc::Vector2D tmp;
		for( uint i = 0 ; i < 3 ; i++ )
		{
			if( opponents[i].absY() < min )
			{
				min = opponents[i].absY();
				tmp = opponents[i] + rcsc::Vector2D( -4.0 , 0.0 );
			}
		}
		
		if( agent->world().self().unum() == 2 )
		{
			const rcsc::AbstractPlayerObject * tm = agent->world().teammate( 3 );
			if( tm )
			{
				rcsc::Vector2D tmpos = tm->pos();
				if( agent->world().self().pos().dist( tmp ) < tmpos.dist( tmp ) )
				{
					//pos = tmp;
				}
			}
			
		}
		if( agent->world().self().unum() == 3 )
		{
			const rcsc::AbstractPlayerObject * tm = agent->world().teammate( 2 );
			if( tm )
			{
				rcsc::Vector2D tmpos = tm->pos();
				if( agent->world().self().pos().dist( tmp ) < tmpos.dist( tmp ) )
				{
					//pos = tmp;
				}
			}
			
		}
	}

    
    
    //double dash_power = Bhv_CrossMove::getDashPower( agent, pos );
    //double dash_power = getDashPower( agent, pos );
	/*
	dash_power *= 1.2;
	dash_power = std::min( dash_power , sp.maxDashPower() );
	if( pos.x < 0 && wm.existKickableOpponent() )
		dash_power = sp.maxDashPower();
	*/
	//double dist_thr = agent->world().self().pos().dist( pos ) * 0.07;
	//if( dist_thr < 0.5 )
	//	dist_thr = 0.5;
	
	rcsc::Vector2D target( 52 , pos.y );
	rcsc::AngleDeg deg1 = ( agent->world().self().pos() - target ).th();
	if( ! Body_GoToPoint( pos, 0.5 , dash_power ).execute( agent ) )
	{
	//Body_GoToPoint( pos, 0.9 , dash_power ).execute( agent );
		//if( std::fabs( wm.self().body().degree() - ( my_pos - ball_pos ).th().degree() ) > 90.0 )
			rcsc::Body_TurnToBall().execute( agent );
			//rcsc::AngleDeg deg2( deg1.degree() / 2.0 );
			//rcsc::Body_TurnToAngle( deg2 ).execute( agent );
			//rcsc::Body_TurnToPoint( target ).execute( agent );
	}
	/*
	if ( agent->world().ball().distFromSelf() < 20.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
	else
	if ( agent->world().ball().distFromSelf() < 15.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    else
    	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	*/
	
	rcsc::Vector2D defense ( agent->world().theirDefenseLineX() , agent->world().self().pos().y );
	rcsc::Vector2D offside ( agent->world().offsideLineX() , agent->world().self().pos().y );
	int count = agent->world().offsideLineCount();
	
	if( count > 4 )
	{
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( defense ) );
		return true;
	}
	
	if ( agent->world().ball().distFromSelf() > 20.0 )
    	agent->setNeckAction( new rcsc::Neck_ScanField() );
	else
	if ( agent->world().ball().distFromSelf() > 15.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    else
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    	
	return true;
	
}

/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::cross( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    //if ( wm.existKickableTeammate() && ( unum == 11 || unum == 10 ) )
    if ( unum == 11 || unum == 10 )
    {
		pos.x = std::max( ball_pos.x , offside - 1.0 );
	}
	
    //if ( wm.existKickableTeammate() && ( unum == 9 || unum == 8 || unum == 7 ) )
    if ( unum == 9 || unum == 8 || unum == 7 )
    {
		pos.x = std::max( ball_pos.x , offside - 3.0 );
	}

	//if ( wm.existKickableTeammate() && unum == 6 )
	if ( unum == 6 )
    {
		pos.x = std::max( ball_pos.x , offside - 5.0 );
	}
	
    /*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 12.5 , deg + 12.5 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass 
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass  = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 12.5 , deg + 12.5 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}
    
/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::dribbleAttack( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    //if ( wm.existKickableTeammate() && ( unum == 9 || unum == 8 ) )
    if ( unum == 9 || unum == 8 )
    {
		pos.x = std::min( ball_pos.x + 15.0 , offside - 2.0 );
	}
	
    //if ( wm.existKickableTeammate() && ( unum == 11 || unum == 10 ) )
    if ( unum == 11 || unum == 10 )
    {
		pos.x = std::max( ball_pos.x , offside - 1.0 );
	}
	
    //if ( wm.existKickableTeammate() && unum == 7 )
    if ( unum == 7 || unum == 6 )
    {
		//pos.x = ball_pos.x;
	}
    
    /*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 12.5 , deg + 12.5 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 12.5 , deg + 12.5 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}
    
	
/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::dribbleBlock( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    //if ( wm.existKickableTeammate() && ( unum == 9 || unum == 8 ) )
    if ( unum == 9 || unum == 8 )
    {
		pos.x = std::min( pos.x + 5.0 , offside - 2.0 );
	}
	
    //if ( wm.existKickableTeammate() && ( unum == 11 || unum == 10 ) )
    if ( unum == 11 || unum == 10 )
    {
		pos.x = std::max( pos.x , offside - 1.0 );
	}
	
    //if ( wm.existKickableTeammate() && unum == 7 )
    if ( unum == 7 || unum == 6 )
    {
		//pos.x = ball_pos.x;
	}

    /*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 12.5 , deg + 12.5 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 12.5 , deg + 12.5 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}
	
/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::shootchance( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	
    rcsc::Vector2D shoot_pos( 52.5 , 0 );
    //if ( wm.existKickableTeammate() && ( unum == 11 || unum == 10 ) )
    if ( unum == 11 || unum == 10 )
    {
		pos = rcsc::Vector2D( std::max( ball_pos.x , offside - 1.0 ) , 7 );
		
	    if ( unum == 10 )
			pos.y = -7;
	}
	
    //if ( wm.existKickableTeammate() && ( unum == 9 || unum == 8 ) )
    if ( unum == 9 || unum == 8 )
    {
		pos = rcsc::Vector2D( std::max( ball_pos.x , offside - 2.0 ) , 11 );
		
	    if ( unum == 8 )
			pos.y = -11;
	}
	
    //if ( wm.existKickableTeammate() && unum == 7 )
    if ( unum == 7 )
    {
		pos = rcsc::Vector2D( std::max( ball_pos.x , offside - 5.0 ) , 1 );
	}

    /*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 10 , deg + 10 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 10 , deg + 10 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}
	
 
/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::defmid( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();
	

    //if ( wm.existKickableTeammate() && my_pos.x > -10 && ( unum == 11 || unum == 10 ) )
    //if ( my_pos.x > -10 && ( unum == 11 || unum == 10 ) )
    if ( unum == 11 || unum == 10 )
    {
		pos.x = std::max( pos.x , offside - 1.0 );
	}
		
    //if ( wm.existKickableTeammate() && my_pos.x > -10 && ( unum == 9 || unum == 8 ) )
    //if ( my_pos.x > -10 && ( unum == 9 || unum == 8 ) )
    if ( unum == 9 || unum == 8 )
    {
		pos.x = std::min( pos.x + 5.0 , offside - 2.0 );
	}
	
    //if ( wm.existKickableTeammate() && my_pos.x > -10 && unum == 6 )
    //if ( my_pos.x > -10 && unum == 6 )
    if ( unum == 6 || unum == 7 )
    {
		//pos.x = ball_pos.x;
	}

    /*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 12.5 , deg + 12.5 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 12.5 , deg + 12.5 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}


/*------------------------------------------------------*/
rcsc::Vector2D
Bhv_GlobalPositioning::offmid( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	rcsc::Vector2D pos = M_home_pos;
	int unum = wm.self().unum();
    double offside = wm.offsideLineX();
	//const rcsc::ServerParam & sp = rcsc::ServerParam::i();

	//if ( wm.existKickableTeammate() && ( unum == 11 || unum == 10 ) )
	if ( unum == 11 || unum == 10 )
    {
		pos.x = std::max( pos.x , offside - 1.0 );
	}
	
    //if ( wm.existKickableTeammate() && ( unum == 9 || unum == 8 ) )
    if ( unum == 9 || unum == 8 )
    {
		pos.x = std::max( pos.x , offside - 2.0 );
	}
	
    //if ( wm.existKickableTeammate() && unum == 6 )
    if ( unum == 6 )
    {
		//pos.x = ball_pos.x;
	}

	//if ( wm.existKickableTeammate() && unum == 7 )
	if ( unum == 7 )
    {
		//pos.x = ball_pos.x;
	}
	/*
    bool pass = false;
	rcsc::AngleDeg deg = ( ball_pos - pos ).th();
	
	rcsc::Vector2D a = pos - rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Vector2D b = pos + rcsc::Vector2D::polar2vector( 2.0 , deg + 90.0 );
	rcsc::Triangle2D route( ball_pos , a , b );
	
	rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( pos ) , deg - 12.5 , deg + 12.5 );
	if( ! wm.existOpponentIn( route , 10 , true ) )
	{
		double dist = 100.0;
		const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( pos , 10 , &dist );
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( pos , 10 , &dist );
		if( tm && opp )
			pass = true;
		if( pass
			&& ( my_pos.dist( pos ) < opp->pos().dist( pos )
			|| tm->pos().dist( pos ) < opp->pos().dist( pos ) )
		  )
			pass = true;
		else
			pass = false;
		if( pass )
			Body_Pass::passRequest( agent , pos );
	}
	else
	if( my_pos.x < ball_pos.x + 7.0 )
	{
		rcsc::Sector2D route( ball_pos , 0.0 , ball_pos.dist( my_pos ) , deg - 12.5 , deg + 12.5 );
		if( ! wm.existOpponentIn( route , 10 , true ) )
		{
			Body_Pass::passRequest( agent , my_pos );
		}
	}
	*/
	return pos;
}

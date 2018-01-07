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
#include "bhv_spread_positioning.h"
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
Bhv_SpreadPositioning::execute( rcsc::PlayerAgent * agent )
{
	
	const rcsc::WorldModel & wm = agent->world();


	rcsc::Vector2D pos = M_home_pos;
	rcsc::Vector2D ball_pos = wm.ball().pos();

/*
			
	if( wm.self().isFrozen() )
		return false;

	if( ( wm.ball().pos().y < 0.0 && wm.ball().pos().x < 20.0 && wm.self().unum() == 2 ) ||
		( wm.ball().pos().y < 0.0 && wm.ball().pos().x > 20.0 && wm.self().unum() == 11 ) ||
		( wm.ball().pos().y > 0.0 && wm.ball().pos().x < 20.0 && wm.self().unum() == 3 ) ||
		( wm.ball().pos().y > 0.0 && wm.ball().pos().x > 20.0 && wm.self().unum() == 10 ) )
	{
		
	}
	else
	{
		return false;
	}

	std::vector< const rcsc::AbstractPlayerObject * >teammates;
	const rcsc::AbstractPlayerCont & team = wm.allTeammates();
	rcsc::AbstractPlayerCont::const_iterator team_end = team.end();
	for( rcsc::AbstractPlayerCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( fabs( (*tm)->pos().x - wm.self().pos().x ) > 20.0 )
			continue;
		teammates.push_back( *tm );
	}
	
	if( teammates.size() < 2 )
		return false;
	
	for( uint i = 0 ; i < teammates.size() - 1 ; i++ )
	{
		for( uint j = i ; j < teammates.size() ; j++ )
		{
			if( fabs( teammates[i]->pos().y ) < fabs( teammates[j]->pos().y ) )
			{
				const rcsc::AbstractPlayerObject * tmp = teammates[i];
				teammates[i] = teammates[j];
				teammates[j] = tmp;
			}
		}
	}
	
	int unum1 = teammates[0]->unum();
	int unum2 = teammates[1]->unum();
	
	if( unum1 != wm.self().unum() && unum2 != wm.self().unum() )
	{
		return false;
	}
	
	rcsc::Vector2D target( wm.self().pos().x , std::max( wm.self().pos().y , 28.0 ) );
	if( wm.self().pos().y < 0.0 )
		target.y *= -1.0;

	double dash_power = Bhv_CrossMove::get_dash_power( agent );
	
	std::cout<<"["<<wm.time().cycle()<<"] side is "<<wm.self().unum()<<std::endl;

	if(! Body_GoToPoint( target , 1.0 , dash_power ).execute( agent ) )
	{
		rcsc::Vector2D front( 52 , target.y );
		rcsc::AngleDeg deg1 = ( wm.self().pos() - front ).th();
		rcsc::Body_TurnToAngle( deg1 ).execute( agent );
		return true;
	}
*/

	rcsc::Vector2D target;
	if( wm.self().unum() == 8 || wm.self().unum() == 9 )
	{
		target.assign( std::min( pos.x , wm.offsideLineX() - 1.0 ) , 28.0 );
		//target.assign( wm.offsideLineX() - 1.0 , 28.0 );
		if( pos.y < 0.0 )
			target.y *= -1.0;
	}
	else
		return false;
	
	double dash_power = Bhv_CrossMove::get_dash_power( agent );
	
	if(! Body_GoToPoint( target , 1.0 , dash_power ).execute( agent ) )
	{
		rcsc::Vector2D front( 52 , target.y );
		rcsc::AngleDeg deg1 = ( wm.self().pos() - front ).th();
		rcsc::Body_TurnToAngle( rcsc::AngleDeg ( deg1.degree() / 2.0 ) ).execute( agent );
		//rcsc::Body_TurnToPoint( front ).execute( agent );
	}
	
	rcsc::Vector2D defense ( wm.theirDefenseLineX() , wm.self().pos().y );
	rcsc::Vector2D offside ( wm.offsideLineX() , wm.self().pos().y );
	int count = wm.offsideLineCount();
	
	if( count > 4 )
	{
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( defense ) );
		return true;
	}
	
	if ( wm.ball().distFromSelf() > 20.0 )
    	agent->setNeckAction( new rcsc::Neck_ScanField() );
	else
	if ( wm.ball().distFromSelf() > 15.0 )
    	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    else
    	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	
	return true;
	
}

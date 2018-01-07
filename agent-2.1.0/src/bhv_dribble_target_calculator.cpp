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

#include "bhv_dribble_target_calculator.h"
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DribbleTargetCalculator::calculate( const rcsc::WorldModel & wm , rcsc::Vector2D * dribble_target , const bool cross )
{
	
	//const rcsc::WorldModel & wm = agent->world();
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	double target_dist = M_dist2;
	double check_dist = M_dist;
	rcsc::AngleDeg target_deg( 0.0 );
	//rcsc::AngleDeg step( 30.0 );
	double step = 30.0;
	std::vector< rcsc::Vector2D >targets;
	

	if( ! cross )
	{
		rcsc::AngleDeg first( -90.0 );
		rcsc::AngleDeg end( 90.0 );
		for( target_deg = rcsc::AngleDeg( first ) ; target_deg.degree() <= end.degree() ; target_deg += rcsc::AngleDeg( 20.0 ) )
		{
			rcsc::Vector2D target = ball_pos + rcsc::Vector2D::polar2vector( target_dist , target_deg.degree() );
			if( target.x > 50.0 )
				target.x = 50.0;
			if( target.y > 31 )
				target.y = 31;
			if( target.y < -31 )
				target.y = -31;
			rcsc::Sector2D route( my_pos , 0.5 , check_dist , target_deg.degree() - step , target_deg.degree() + step );
			if( ! wm.existOpponentIn( route , 10 , true ) )
			{
				targets.push_back( target );
			}
		
		}
		
		if( targets.size() < 1 )
		{
			std::cout<<"dribble target false"<<std::endl;
			return false;
		}
		
		for( uint i = 0 ; i < targets.size() - 1 ; i++ )
			for( uint j = i + 1 ; j < targets.size() ; j++ )
				if( targets[i].x < targets[j].x )
				{
					rcsc::Vector2D tmp = targets[i];
					targets[i] = targets[j];
					targets[j] = tmp;
				}
	}
	else
	{
		rcsc::AngleDeg first( -180.0 );
		rcsc::AngleDeg end( 90.0 );
		for( target_deg = rcsc::AngleDeg( first ) ; target_deg.degree() <= end.degree() ; target_deg += rcsc::AngleDeg( 20.0 ) )
		{
			rcsc::Vector2D target = ball_pos + rcsc::Vector2D::polar2vector( target_dist , target_deg.degree() );
			if( target.x > 50.0 )
				target.x = 50.0;
			if( target.y > 31 )
				target.y = 31;
			if( target.y < -31 )
				target.y = -31;
			rcsc::Sector2D route( my_pos , 0.5 , check_dist , target_deg.degree() - step , target_deg.degree() + step );
			if( ! wm.existOpponentIn( route , 10 , true ) )
			{
				targets.push_back( target );
			}
		
		}
		
		if( targets.size() < 1 )
		{
			std::cout<<"dribble target false"<<std::endl;
			return false;
		}
		
		for( uint i = 0 ; i < targets.size() - 1 ; i++ )
			for( uint j = i + 1 ; j < targets.size() ; j++ )
				if( targets[i].absY() > targets[j].absY() )
				{
					rcsc::Vector2D tmp = targets[i];
					targets[i] = targets[j];
					targets[j] = tmp;
				}
	}
		
	* dribble_target = targets[0];
	return true;
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Samira KARIMZADEH
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

#include "bhv_dribble_target_calculator2.h"

#include <rcsc/player/world_model.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/geom/triangle_2d.h>
#include <vector>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DribbleTargetCalculator2::calculate( const rcsc::WorldModel & wm , rcsc::Vector2D * dribble_target )
{

    std::vector< rcsc::AngleDeg >teta;
    double mx = dribble_target->x;
    double my = dribble_target->y;
    rcsc::AngleDeg ang;
    
    if( mx > 37 )
		return false;
    
    if( my > 0 && my < 17 )
    {	
		teta.push_back(rcsc::AngleDeg(10));
		teta.push_back(rcsc::AngleDeg(50));
		teta.push_back(rcsc::AngleDeg(-30));
		teta.push_back(rcsc::AngleDeg(30));
		teta.push_back(rcsc::AngleDeg(-10));
    }
    else
    if( my < 0 && my > -17 )
    {	
		teta.push_back(rcsc::AngleDeg(10));
		teta.push_back(rcsc::AngleDeg(30));
		teta.push_back(rcsc::AngleDeg(-50));
		teta.push_back(rcsc::AngleDeg(10));
		teta.push_back(rcsc::AngleDeg(-30));
    }
    else
    if( my <= -17 )
    {	
		teta.push_back(rcsc::AngleDeg(10));
		teta.push_back(rcsc::AngleDeg(50));
		teta.push_back(rcsc::AngleDeg(30));
    }
    else
    if( my >= 17 )
    {	
		teta.push_back(rcsc::AngleDeg(-10));
		teta.push_back(rcsc::AngleDeg(-50));
		teta.push_back(rcsc::AngleDeg(-30));
    }
    
    std::vector< rcsc::AngleDeg >::const_iterator it1;
    for( it1 = teta.begin(); it1 != teta.end(); ++it1 )
    {
		rcsc::Vector2D a = *dribble_target;
		rcsc::Vector2D b,c;
		b = b.setPolar(8.0 , it1->degree()) + a;
		c = c.setPolar(8.0 , it1->degree()-20) + a;
		if(! wm.existOpponentIn(rcsc::Triangle2D(a,b,c) , 10 , 1) )
		{
		    ang = it1->degree() - 10;
		    if( std::fabs( ang.degree() ) > 120 )
			{
			    continue;
			}
		    
		    rcsc::Vector2D drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 7.0 , ang );
		    * dribble_target = drib_target;
		    return true;
		}
    }
    
    return false;
}

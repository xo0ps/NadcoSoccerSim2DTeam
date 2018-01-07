// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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

#include "bhv_basic_offensive_kick.h"
#include "body_dribble.h"
#include "body_kick_to_corner.h"
#include "body_pass.h"
#include "body_direct_pass.h"
#include "body_leading_pass.h"
#include "body_through_pass.h"
#include "body_dash.h"
#include "bhv_dribble_target_calculator.h"
#include "bhv_dribble_target_calculator2.h"

#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_BasicOffensiveKick::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerObject * nearest_opp = ( opps.empty() ? static_cast< rcsc::PlayerObject * >( 0 ) : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const rcsc::Vector2D nearest_opp_pos = ( nearest_opp
                                             ? nearest_opp->pos()
                                             : rcsc::Vector2D( -1000.0, 0.0 ) );

    
    if( wm.self().pos().x > wm.offsideLineX() - 15 )
	{
		if( Body_ThroughPass::test( agent ) && Body_ThroughPass("toBehindDefenders").execute( agent ) )
		{
			return true;
		}
	}
	
	if( Body_LeadingPass("old").execute( agent ) )
	{
		return true;
	}		
    
    static int counter = 0;
	if( Body_Dash::test( agent ) )
	{
		//std::cout<<"counter = "<<counter<<std::endl;
		if( counter == 1 )
		{
			if( Body_Dash().execute( agent ) )
			{
				return true;
			}
		}
		counter++;
	}
	else
	{
		counter = 0;
	}
    
    rcsc::Vector2D point( -100 , -100 );
	Body_DirectPass::test_new( agent , &point );
	if( point.x > wm.ball().pos().x + 2.0 )
	{
		bool safety = true;
		const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
		for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end; ++it )
		{
			if ( (*it)->pos().dist( point ) < 3.0 )
			{
				safety = false;
			}
		}

		if ( safety )
		{
			if( Body_DirectPass("playOn").execute( agent ) )
			{
				return true;
			}	
		}

	}
			
    if ( nearest_opp_dist < 7.0 )
    {
        //if( Body_DirectPass("playOn").execute( agent ) )
        //{
        //    //agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        //    return true;
        //}
    }

	rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0 );
	if( body_dir_drib_target.y > 32 )
		body_dir_drib_target.y = 32;
	if( body_dir_drib_target.y < -32 )
		body_dir_drib_target.y = -32;
	
	Bhv_DribbleTargetCalculator( 10.0 , 3.0 ).calculate( wm , &body_dir_drib_target , false );
	//Bhv_DribbleTargetCalculator2().getTarget( wm , &body_dir_drib_target );
	rcsc::AngleDeg deg = ( body_dir_drib_target - wm.ball().pos() ).th();
	rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0 , deg - 30.0, deg + 30.0 );
	
    // dribble to my body dir
    if ( nearest_opp_dist < 5.0
         && nearest_opp_dist > ( rcsc::ServerParam::i().tackleDist()
                                 + rcsc::ServerParam::i().defaultPlayerSpeedMax() * 1.5 )
         && wm.self().body().abs() < 70.0 )
    {
        //const rcsc::Vector2D body_dir_drib_target = wm.self().pos()
        //    + rcsc::Vector2D::polar2vector(5.0, wm.self().body());
		
		
        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( //body_dir_drib_target.x < rcsc::ServerParam::i().pitchHalfLength() - 1.0
             //&& body_dir_drib_target.absY() < rcsc::ServerParam::i().pitchHalfWidth() - 1.0
             //&& 
             max_dir_count < 3
             )
        {
            // check opponents
            // 10m, +-30 degree
            //const rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0, wm.self().body() - 30.0, wm.self().body() + 30.0 );
            // opponent check with goalie
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
                Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower(), 2 ).execute( agent );
                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
                return true;
            }
        }
    }

	/*
    rcsc::Vector2D drib_target( 50.0, wm.self().pos().absY() );
    if ( drib_target.y < 20.0 ) drib_target.y = 20.0;
    if ( drib_target.y > 29.0 ) drib_target.y = 27.0;
    if ( wm.self().pos().y < 0.0 ) drib_target.y *= -1.0;
    const rcsc::AngleDeg drib_angle = ( drib_target - wm.self().pos() ).th();
	*/
	
    // opponent is behind of me
    if ( nearest_opp_pos.x < wm.self().pos().x + 1.0 )
    {
        // check opponents
        // 15m, +-30 degree
        //const rcsc::Sector2D sector( wm.self().pos(), 0.5, 15.0, drib_angle - 30.0, drib_angle + 30.0 );
        // opponent check with goalie
        
        //Bhv_DribbleTargetCalculator( 10.0 , 3.0 ).calculate( wm , &body_dir_drib_target );
		//rcsc::AngleDeg deg = ( body_dir_drib_target - wm.ball().pos() ).th();
		//rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0 , deg - 30.0, deg + 30.0 );
		
        if ( ! wm.existOpponentIn( sector, 10, true ) )
        {
            const int max_dash_step = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
            if ( wm.self().pos().x > 35.0 )
            {
                //body_dir_drib_target.y *= ( 10.0 / body_dir_drib_target.absY() );
            }
            
            Body_Dribble( body_dir_drib_target , 1.0, rcsc::ServerParam::i().maxDashPower(), std::min( 5, max_dash_step ) ).execute( agent );
        }
        else
        {
            
            Body_Dribble( body_dir_drib_target , 1.0, rcsc::ServerParam::i().maxDashPower(), 2 ).execute( agent );
        }
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is far from me
    if ( nearest_opp_dist > 5.0 )
    {
        Body_Dribble( body_dir_drib_target , 1.0, rcsc::ServerParam::i().maxDashPower() * 0.8 , 1 ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }

    // opp is near

    // can pass
    //if ( rcsc::Body_Pass().execute( agent ) )
    //if( Body_DirectPass("playOn").execute( agent ) )
    //{
        //agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    //    return true;
    //}

    // opp is far from me
    if ( nearest_opp_dist > 3.0 )
    {
        Body_Dribble( body_dir_drib_target , 1.0, rcsc::ServerParam::i().maxDashPower() * 0.4 , 1 ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }

    if ( nearest_opp_dist > 2.5 )
    {
        rcsc::Body_HoldBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }

    if ( wm.self().pos().x > wm.offsideLineX() - 10.0 )
    {
        //Body_KickToCorner( (wm.self().pos().y < 0.0) ).execute( agent );
        //agent->setNeckAction( new rcsc::Neck_ScanField() );
    }
    else
    {
        rcsc::Body_AdvanceBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_ScanField() );
    }

    return true;
}

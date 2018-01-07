// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

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

#include "strategy.h"
#include "sample_player.h"
#include "body_selection_pass.h"
#include "body_leading_pass.h"
#include "body_leading_pass2.h"
#include "body_direct_pass.h"
#include "body_direct_pass2.h"
#include "body_through_pass.h"
#include "body_through_pass2.h"
#include "bhv_dribble_kick.h"
#include "body_dash.h"
#include "body_clear_ball.h"
#include "body_dribble.h"
#include "bhv_offmid_defmid_kick.h"
#include "bhv_danger_kick.h"
#include "bhv_block.h"
#include "bhv_tactics.h"
#include "body_pass.h"
#include "bhv_offensive_planner.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"
#include "bhv_dribble_target_calculator.h"
#include "bhv_dribble_target_calculator2.h"
#include "bhv_basic_offensive_kick.h"
#include "body_kick_to_center.h"
#include "bhv_dribble_target_calculator2.h"

#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/intention_dribble2008.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/server_param.h>


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DribbleKick::execute( rcsc::PlayerAgent * agent )
{
	//return Bhv_BasicOffensiveKick().execute( agent );
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    if( Strategy::i().rc() )
	{
		return RC( agent );
	}
    
	if( Strategy::i().fast_pass() )
	{
		if( Body_Pass::requestedPass2( agent ) )
		{
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
		
		if( Body_ThroughPass("toBehindDefenders").execute( agent ) )
		{
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
		
		if ( Strategy::i().old_pass() )
	    {	
			if( Body_LeadingPass("old").execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
			if( Body_DirectPass("old").execute( agent ) )
			{		
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
			{
				rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
				rcsc::Body_HoldBall( false , target, target ).execute( agent );
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
		}
	    else
		if ( Strategy::i().selection_pass() )
	    {
			if( Body_SelectionPass().execute( agent ) )
			{		
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
			{
				rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
				rcsc::Body_HoldBall( false , target, target ).execute( agent );
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
			}
	    }
		else
		{
			offensivePlan( agent );
	        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	        return true;
		}
	}
	else
	if( Strategy::i().decision_pass() )
	{
				
		/*--------------------------------------------*/
		    if( wm.ball().pos().x < 0 )
			{
				if( Bhv_Tactics( "substitueKick" ).execute( agent ) )
				{
					return true;
				}
				
				if( wm.self().pos().x > wm.offsideLineX() - 27 )
				{
					if( Body_ThroughPass::test( agent ) && Body_ThroughPass("toBehindDefenders").execute( agent ) )
					{
						return true;
					}
				}
				
				rcsc::Vector2D point( -100 , -100 );
				if( Body_DirectPass::test_new( agent , &point ) )
				{
					if( point.x > wm.ball().pos().x + 2.0 )
					{
						if( Body_DirectPass("playOn").execute( agent ) )
						{
							return true;
						}	
					}
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
				
				{
					rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0 );
					if( body_dir_drib_target.y > 32 )
						body_dir_drib_target.y = 32;
					if( body_dir_drib_target.y < -32 )
						body_dir_drib_target.y = -32;
					const int max_dash_step = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
					Bhv_DribbleTargetCalculator( 10.0 , 3.0 ).calculate( wm , &body_dir_drib_target , false );
					//Bhv_DribbleTargetCalculator2().getTarget( wm , &body_dir_drib_target );
					rcsc::AngleDeg deg = ( body_dir_drib_target - wm.ball().pos() ).th();
					rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0 , deg - 30.0, deg + 30.0 );
		            if ( ! wm.existOpponentIn( sector, 10, true ) )
					{
						Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 2, false ).execute( agent );
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			            //agent->setIntention( new rcsc::IntentionDribble2008( 
						//									body_dir_drib_target,
						//									2.0,
						//									2.0,
						//									std::min( 5, max_dash_step ),
						//									rcsc::ServerParam::i().maxDashPower(),
						//									false,
						//									wm.time() ) );
						//agent->addSayMessage( new rcsc::DribbleMessage( body_dir_drib_target, std::min( 5, max_dash_step )));
						return true;
					}
					/*
					else
					if ( Body_KickToCenter::get_best_point( agent, NULL ) )
				    {
				        Body_KickToCenter().execute( agent );
						return true;
					}
					*/
					else
					{
						const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
						const rcsc::PlayerObject * nearest_opp =
						( opps.empty() ? static_cast< rcsc::PlayerObject * >( 0 ) : opps.front() );
						const double nearest_opp_dist = ( nearest_opp ? nearest_opp->distFromSelf() : 1000.0 );
						const rcsc::Vector2D nearest_opp_pos =
						( nearest_opp ? nearest_opp->pos() : rcsc::Vector2D( -1000.0, 0.0 ) );
						
						if( nearest_opp_dist < 4.0
						&& nearest_opp_pos.x > wm.self().pos().x
						&& std::fabs( nearest_opp_pos.y - wm.self().pos().y ) < 8.0 )
						{
							if( Body_DirectPass("playOn").execute( agent ) )
							{
								return true;
							}
						}
						else
						if( nearest_opp_dist < 7.0
						&& nearest_opp_pos.x > wm.self().pos().x
						&& std::fabs( nearest_opp_pos.y - wm.self().pos().y ) < 8.0 )
						{
							rcsc::Body_HoldBall().execute( agent );
							agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
							return true;
						}
						else
						{
							if( Body_DirectPass("playOn").execute( agent ) )
							{
								return true;
							}
						}
					}
					{					
						rcsc::Body_HoldBall().execute( agent );
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
						return true;
					}
				}
			}
			else
			{
				if( wm.self().pos().x > wm.offsideLineX() - 27 )
				{
					if( Body_ThroughPass::test( agent ) && Body_ThroughPass("toBehindDefenders").execute( agent ) )
					{
						return true;
					}
					
					/*------------------------------------------------------------------*/
					
					/*
					static rcsc::Vector2D target( 100,100 );
					static int unum = 0;
					static double first_speed = 4.0;
					static int counter = 0;
					
					if( counter == 4 )
					{
						//if (! Body_SmartKick( target , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
							Body_KickOneStep( target , first_speed , true ).execute( agent );
						std::cout<<"dribble through"<<std::endl;
						counter = 0;
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
						return true;
					}
					
					if( Body_ThroughPass::test1( agent , &target , &first_speed , &unum ) )
					{
						Body_Pass::say_pass( agent , unum , target );
						counter++;
						rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 5.0 , 0 );
						Body_Dribble( body_dir_drib_target,
			                                    2.0,
			                                    rcsc::ServerParam::i().maxDashPower(),
			                                    2
			                                    ).execute( agent );
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
						std::cout<<"dribble "<<counter<<std::endl;
						return true;
					}
					else
						counter = 0;
					
					/*------------------------------------------------------------------*/
					
				}
				
				
				rcsc::Vector2D point( -100 , -100 );
				if( Body_DirectPass::test_new( agent , &point ) )
				{
					if( point.x > wm.ball().pos().x + 2.0 )
					{
						if( Body_DirectPass("playOn").execute( agent ) )
						{
							return true;
						}
					}
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
		
				{
					
					rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0 );
					if( body_dir_drib_target.y > 32 )
						body_dir_drib_target.y = 32;
					if( body_dir_drib_target.y < -32 )
						body_dir_drib_target.y = -32;
					
					Bhv_DribbleTargetCalculator( 10.0 , 3.0 ).calculate( wm , &body_dir_drib_target , false );
					//Bhv_DribbleTargetCalculator2().getTarget( wm , &body_dir_drib_target );
					rcsc::AngleDeg deg = ( body_dir_drib_target - wm.ball().pos() ).th();
					rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0 , deg - 30.0, deg + 30.0 );
		            if ( ! wm.existOpponentIn( sector, 10, true ) )
					{
						Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 2, false ).execute( agent );
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		                //agent->setIntention( new rcsc::IntentionDribble2008( 
						//							body_dir_drib_target,
						//							2.0,
						//							2.0,
						//							2.0,
						//							rcsc::ServerParam::i().maxDashPower(),
						//							false,
						//							wm.time() ) );
		                //agent->addSayMessage( new rcsc::DribbleMessage( body_dir_drib_target, 2.0));
			            const rcsc::PlayerObject * tm = wm.getTeammateNearestToSelf( 10, false );
						if( tm && wm.self().pos().x > 15 )
						{
							rcsc::Vector2D target( 50.0 , 15.0 );
							if( wm.ball().pos().y < 0.0 )
								target.y = -15.0;
							Body_Pass::say_pass( agent , tm->unum() , target );
						}
						return true;
					}
					else
					if ( Body_KickToCenter::get_best_point( agent, NULL ) )
				    {
				        //Body_KickToCenter().execute( agent );
						//return true;
					}
					else
					{
						const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
						const rcsc::PlayerObject * nearest_opp =
						( opps.empty() ? static_cast< rcsc::PlayerObject * >( 0 ) : opps.front() );
						const double nearest_opp_dist = ( nearest_opp ? nearest_opp->distFromSelf() : 1000.0 );
						const rcsc::Vector2D nearest_opp_pos =
						( nearest_opp ? nearest_opp->pos() : rcsc::Vector2D( -1000.0, 0.0 ) );
						
						if( nearest_opp_dist < 4.0
						&& nearest_opp_pos.x > wm.self().pos().x
						&& std::fabs( nearest_opp_pos.y - wm.self().pos().y ) < 8.0 )
						{
							if( Body_DirectPass("playOn").execute( agent ) )
							{
								return true;
							}
						}
						else
						if( nearest_opp_dist < 7.0
						&& nearest_opp_pos.x > wm.self().pos().x
						&& std::fabs( nearest_opp_pos.y - wm.self().pos().y ) < 8.0 )
						{
							rcsc::Body_HoldBall().execute( agent );
							agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
							return true;
						}
						else
						{
							if( Body_DirectPass("playOn").execute( agent ) )
							{
								return true;
							}
						}
					}
					{					
						rcsc::Body_HoldBall().execute( agent );
						agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
						return true;
					}
					/*
					int max_dir_count = 0;
			        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );
			
			        //if ( max_dir_count < 3 )
			        {
			            const rcsc::Sector2D sector( wm.self().pos(), 0.5, 10.0, wm.self().body() - 30.0, wm.self().body() + 30.0 );
			            if ( ! wm.existOpponentIn( sector, 10, true ) )
			            {
			                Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower(), 2, false ).execute( agent );
			                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			                //agent->setIntention( new rcsc::IntentionDribble2008( 
							//							body_dir_drib_target,
							//							2.0,
							//							2.0,
							//							2.0,
							//							rcsc::ServerParam::i().maxDashPower(),
							//							false,
							//							wm.time() ) );
			                //agent->addSayMessage( new rcsc::DribbleMessage( body_dir_drib_target, 2.0));
			                return true;
			            }
			            else
			            {
							/*
							//if( Body_DirectPass("old").execute( agent ) )
							if( Body_DirectPass("playOn").execute( agent ) )
							{
								agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
								return true;
							}
							*/
							/*
							{
								Body_Dribble( body_dir_drib_target,
			                                    2.0,
			                                    rcsc::ServerParam::i().maxDashPower(),
			                                    2
			                                    ).execute( agent );
				                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				                //agent->setIntention( new rcsc::IntentionDribble2008( 
								//							body_dir_drib_target,
								//							2.0,
								//							2.0,
								//							2.0,
								//							rcsc::ServerParam::i().maxDashPower(),
								//							false,
								//							wm.time() ) );
				                agent->addSayMessage( new rcsc::DribbleMessage( body_dir_drib_target, 2.0));
				                return true;
							}
							
							{
								rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
								rcsc::Body_HoldBall( false , target, target ).execute( agent );
								agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
								return true;
							}
						}
					}
					*/
					
				}
			}
			std::cout<<"dribble false"<<std::endl;
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		    return false;

		/*--------------------------------------------*/
	}
	
	
    if( Body_Pass::requestedPass2( agent ) )
	{
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}


	if( Body_Dash().execute( agent ) )
	{
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}


    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< rcsc::PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
    const rcsc::Vector2D nearest_opp_pos = ( nearest_opp
                                             ? nearest_opp->pos()
                                             : rcsc::Vector2D( -1000.0, 0.0 ) );

	
	if ( Strategy::i().off_planner() )
	    if(wm.self().pos().x > 0)
	    {
			rcsc::Vector2D target_point(0,0);
			if(Bhv_OffensivePlanner::isShootLineExist2(agent , target_point, false))
			{
				std::cout<<"dribble kick isShootLine"<<std::endl;
			    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			    return true;
			}
		
			if(Bhv_OffensivePlanner::keepShootChance(agent))
			{	
				std::cout<<"dribble kick keepShootChance"<<std::endl;
			    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			    return true;
			}
	    }


    if ( nearest_opp_dist < 5.0
         && nearest_opp_dist > ( sp.tackleDist()
                                 + sp.defaultPlayerSpeedMax() * 1.5 )
         && wm.self().body().abs() < 90.0 )
    {
        rcsc::Vector2D body_dir_drib_target( 40 , -20 );
		if( wm.self().pos().y > 0 )
			body_dir_drib_target = Vector2D( 40 , 20 );

        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( //body_dir_drib_target.x < rcsc::sp.pitchHalfLength() - 1.0
             //&& body_dir_drib_target.absY() < rcsc::sp.pitchHalfWidth() - 1.0
             max_dir_count < 3
             )
        {
            // check opponents
            // 5m, +-25 degree
            const rcsc::Sector2D sector( wm.self().pos(),
                                         0.5, 10.0,
                                         wm.self().body().degree() - 30.0,
                                         wm.self().body().degree() + 30.0 );
            // opponent check with goalie
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
				rcsc::Vector2D body_dir_drib_target( 50 , wm.self().pos().y );
				const int max_dash_step = 
				wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
				Body_Dribble( body_dir_drib_target,
									2.0,
									rcsc::ServerParam::i().maxDashPower(),
									std::min( 5 , max_dash_step )
									).execute( agent );
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				return true;
            }
            else
            {
				if( wm.self().pos().x > 0 )
				{
					rcsc::Vector2D body_dir_drib_target( 50 , wm.self().pos().y );
					const int max_dash_step = 
					wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
					Body_Dribble( body_dir_drib_target,
										2.0,
										rcsc::ServerParam::i().maxDashPower(),
										std::min( 5 , max_dash_step )
										).execute( agent );
					agent->setNeckAction( new rcsc::Neck_TurnToBall() );
					return true;
				}
				else
				{
					offensivePlan( agent );
					agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
					return true;
				}
			}
        }
    }

	
	rcsc::Vector2D drib_target( 40 , -20 );
	if( wm.self().pos().y > 0 )
		drib_target = rcsc::Vector2D( 40 , 20 );

    const rcsc::AngleDeg drib_angle = ( drib_target - wm.self().pos() ).th();

    // opponent is behind of me
    if ( nearest_opp_pos.x < wm.self().pos().x + 1.0 )
    {
        // check opponents
        // 5m, +-45 degree
        const rcsc::Sector2D sector( wm.self().pos(),
                                     0.5, 10.0,
                                     drib_angle.degree() - 30.0,
                                     drib_angle.degree() + 30.0 );
        // opponent check with goalie
        if ( ! wm.existOpponentIn( sector, 10, true ) )
        {
			const int max_dash_step = 
			wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( drib_target ) );
            Body_Dribble( drib_target,
                                2.0,
                                sp.maxDashPower(),
                                std::min( 5 , max_dash_step )
                                ).execute( agent );
            agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
            return true;
        }
        else
        {
			offensivePlan( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
        }
    }

    if ( nearest_opp_dist > 5.0 )
    {
		rcsc::Vector2D body_dir_drib_target( 50 , wm.self().pos().y );
		const int max_dash_step = 
		wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
		Body_Dribble( body_dir_drib_target,
							2.0,
							rcsc::ServerParam::i().maxDashPower(),
							std::min( 5 , max_dash_step )
							).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return true;
    }
	else
    if ( nearest_opp_dist > 4.0 )
    {
		offensivePlan( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
	}
	else
	{
		rcsc::Body_AdvanceBall().execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}
	
    return true;
}

/*--------------------------------------------------------------*/

void
Bhv_DribbleKick::offensivePlan( rcsc::PlayerAgent * agent )
{
	if ( Strategy::i().old_pass() )
    {	
		if( Body_LeadingPass("old").execute( agent ) )
		{
			return;
		}
		if( Body_DirectPass("old").execute( agent ) )
		{
			return;
		}
		{		
			rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
			rcsc::Body_HoldBall( false , target, target ).execute( agent );
			return;
		}
	}
    else
	if ( Strategy::i().selection_pass() )
    {
		if( Body_SelectionPass().execute( agent ) )
		{
			return;
		}
		{
			rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
			rcsc::Body_HoldBall( false , target, target ).execute( agent );
			return;
		}
    }
    else
    {	
		if( agent->world().ball().pos().x > 0 )
		{	
			if( Body_LeadingPass("toCorners").execute( agent ) )
			{
				return;
			}
			if( Body_ThroughPass("pointTo").execute( agent ) )
			{
				return;
			}
			if( Body_ThroughPass("toBehindDefenders").execute( agent ) )
			{
				return;
			}
			if( Body_LeadingPass("playOn").execute( agent ) )
			{
				return;
			}
			if( Body_DirectPass("playOn").execute( agent ) )
			{
				return;
			}
			{
				rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
				rcsc::Body_HoldBall( false , target, target ).execute( agent );
				return;
			}
		}
		else
		{
			if( Body_LeadingPass("offendResponse").execute( agent ) )
			{
				return;
			}
			if( Body_LeadingPass("playOn").execute( agent ) )
			{
				return;
			}
			if( Body_DirectPass("playOn").execute( agent ) )
			{
				return;
			}
			{
				rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
				rcsc::Body_HoldBall( false , target, target ).execute( agent );
				return;
			}
		}
	}
}



/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_DribbleKick::RC( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
	const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerObject * nearest_opp = ( opps.empty() ? static_cast< rcsc::PlayerObject * >( 0 ) : opps.front() );
    const double nearest_opp_dist = ( nearest_opp ? nearest_opp->distFromBall() : 1000.0 );
    const rcsc::Vector2D nearest_opp_pos = ( nearest_opp ? nearest_opp->pos() : rcsc::Vector2D( -1000.0, 0.0 ) );

	if( wm.self().pos().x > wm.offsideLineX() - 27 )
	{
		//if( Body_ThroughPass::test( agent ) && Body_ThroughPass("toBehindDefenders").execute( agent ) )
		if( Body_ThroughPass2().execute( agent ) )
		{
			return true;
		}
	}
	
	static int counter = 0;
	if( Body_Dash::test( agent ) )
	{
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
	if( Body_DirectPass::test_new( agent , &point ) )
	{
		if( point.x > wm.ball().pos().x + 2.0 )
		{
			if( Body_DirectPass("playOn").execute( agent ) )
			{
				return true;
			}	
		}
	}
	
	
	if( wm.ball().pos().x < 10.0 )
	{
		if( Bhv_Tactics( "substitueKick" ).execute( agent ) )
		{
			return true;
		}
		
		if( Bhv_Tactics( "timeKill" ).execute( agent ) )
		{
			return true;
		}
	}
	
	rcsc::Vector2D body_dir_drib_target = wm.self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0 );
	Bhv_DribbleTargetCalculator( 10.0 , 5.0 ).calculate( wm , &body_dir_drib_target , false );
	//Bhv_DribbleTargetCalculator2().calculate( wm , &body_dir_drib_target );
		
	double deg = ( body_dir_drib_target - wm.ball().pos() ).th().degree();
	const rcsc::Sector2D sector( wm.self().pos(), 0.5 , 5.0 , deg - 25.0, deg + 25.0 );
	
	if ( nearest_opp_pos.x < wm.self().pos().x - 1.0 )
    {
		//const int max_dash_step = wm.self().playerType().cyclesToReachDistance( wm.self().pos().dist( body_dir_drib_target ) );
		//Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), std::min( 5 , max_dash_step ) ).execute( agent );
		Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 3 ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }

	/*
	if ( nearest_opp_dist > 5.0 )
	{
		Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower(), 6 ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}
	
	if ( nearest_opp_dist > rcsc::ServerParam::i().tackleDist() + rcsc::ServerParam::i().defaultPlayerSpeedMax() )
    {
        int max_dir_count = 0;
        wm.dirRangeCount( wm.self().body(), 20.0, &max_dir_count, NULL, NULL );

        if ( max_dir_count < 3 )
        {
            if ( ! wm.existOpponentIn( sector, 10, true ) )
            {
                Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower(), 6 ).execute( agent );
                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
                return true;
            }
            else
            {
				Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower() * 0.6 , 4 ).execute( agent );
                agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
                return true;
			}
        }
        else
        {
			Body_HoldBall( true , body_dir_drib_target ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
		
		if( Body_DirectPass2().execute( agent ) )
		{
			return true;
		}
    }
    * 
	if ( nearest_opp_dist > 3.0 )
    {
        Body_Dribble( body_dir_drib_target, 1.0, rcsc::ServerParam::i().maxDashPower() * 0.2 , 1 , false ).execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }
    */
	
	if ( nearest_opp_dist > 5.0 )
	{
		Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower() * 0.8 , 4 ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
	}

	if ( nearest_opp_dist > 2.0 && std::fabs( nearest_opp_pos.y - wm.self().pos().y ) > 5.0 )
    {
		Body_Dribble( body_dir_drib_target, 2.0, rcsc::ServerParam::i().maxDashPower() * 0.6 , 4 ).execute( agent );
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
    }
	
	if ( nearest_opp_dist > 2.0 )
	{
		if( Body_DirectPass2().execute( agent ) )
		{
			return true;
		}
		else
		{
			Body_Dribble( wm.ball().pos() + rcsc::Vector2D( -3.0 , 0.0 ) , 1.0, rcsc::ServerParam::i().maxDashPower() , 2 ).execute( agent );
			agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
			return true;
		}
	}
	
    {
        Body_HoldBall().execute( agent );
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
        return true;
    }
	
	std::cout<<"dribble false"<<std::endl;
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    return false;
}

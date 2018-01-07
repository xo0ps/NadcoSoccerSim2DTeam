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

#include "bhv_best_action.h"
#include "sample_player.h"
#include "strategy.h"
#include "body_selection_pass.h"
#include "bhv_through_pass_cut.h"
#include "bhv_global_positioning.h"
#include "body_leading_pass.h"
#include "body_direct_pass.h"
#include "body_through_pass.h"
#include "body_dash.h"
#include "body_clear_ball.h"
#include "body_dribble.h"
#include "bhv_block.h"
#include "body_shoot.h"
#include "body_pass.h"
#include "body_kick_to_corner.h"
#include "bhv_tactics.h"
#include "bhv_basic_tackle.h"
#include "bhv_defense_breaker.h"
#include "bhv_hassle.h"
#include "bhv_mark.h"
#include "bhv_through_pass_cut.h"
#include "body_intercept.h"

#include <rcsc/action/body_advance_ball.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/common/audio_memory.h>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_BestAction::execute( rcsc::PlayerAgent * agent )
{
	switch( calculate( agent ) )
	{
		DIRECT:
			if( Body_LeadingPass("playOn").execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best lead"<<std::endl;
				return true;
			}
			else
			{
				if( Body_DirectPass("playOn").execute( agent ) )
				{
					agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
					std::cout<<"best direct"<<std::endl;
					return true;
				}	
			}
			return false;
			break;
		THROUGH:
			if( Body_ThroughPass("toBehindDefenders").execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best through"<<std::endl;
				return true;
			}
			return false;
			break;
		DASH:
			if( Body_Dash().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				std::cout<<"best dash"<<std::endl;
				return true;
			}
			return false;
			break;
		DRIBBLE:
		{
			rcsc::Vector2D body_dir_drib_target( 50 , agent->world().self().pos().y );
			//rcsc::Vector2D = Bhv_DribbleTargetCalculator().execute( agent );
			const int max_dash_step = agent->world().self().playerType().cyclesToReachDistance( agent->world().self().pos().dist( body_dir_drib_target ) );
			if( Body_Dribble( body_dir_drib_target,
								2.0,
								rcsc::ServerParam::i().maxDashPower(),
								std::min( 5 , max_dash_step )
								).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				std::cout<<"best dribble"<<std::endl;
				return true;
			}
			return false;
			break;
		}
		TAC_KICK:
			if( Bhv_Tactics( "oneTwo" ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best tactic kick"<<std::endl;
				return true;
			}
			return false;
			break;
		CLEAR:
			if( Body_ClearBall().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best clear"<<std::endl;
				return true;
			}
			return false;
			break;
		CORNER:
			if( Body_KickToCorner( true ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best corner"<<std::endl;
				return true;
			}
			return false;
			break;
		SHOOT:
			if( Body_Shoot().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				std::cout<<"best shoot"<<std::endl;
				return true;
			}
			return false;
			break;
		ADVANCE:
			if( rcsc::Body_AdvanceBall().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best advance"<<std::endl;
				return true;
			}
			return false;
			break;
		HOLD:
		{
			rcsc::Vector2D target = agent->world().self().pos() + rcsc::Vector2D::polar2vector( 3.0 , 0.0 );
			if( rcsc::Body_HoldBall( false , target, target ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best hold"<<std::endl;
				return true;
			}
			return false;
			break;
		}
		TACKLE:
			if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best tackle"<<std::endl;
				return true;
			}
			return false;
			break;
		BLOCK:
			if( Bhv_Block().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				std::cout<<"best block"<<std::endl;
				return true;
			}
			return false;
			break;
		DEF_BREAK:
			if( Bhv_DefenseBreaker().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best defense break"<<std::endl;
				return true;
			}
			return false;
			break;
		POSITION:
		{
			rcsc::Vector2D pos = Strategy::i().getPosition( agent->world().self().unum() );
			if( Bhv_GlobalPositioning( pos ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best positioning"<<std::endl;
				return true;
			}
			return false;
			break;
		}
		HASSLE:
		{
			rcsc::Vector2D pos = Strategy::i().getPosition( agent->world().self().unum() );
			if( Bhv_Hassle( pos ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToBall() );
				std::cout<<"best hassle"<<std::endl;
				return true;
			}
			return false;
			break;
		}
		MARK:
		{
			rcsc::Vector2D pos = Strategy::i().getPosition( agent->world().self().unum() );
			if( Bhv_Mark( "mark" , pos ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best hassle"<<std::endl;
				return true;
			}
			return false;
			break;
		}
		TAC_MOVE:
			if( Bhv_Tactics( "oneTwo" ).execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best tactic move"<<std::endl;
				return true;
			}
			return false;
			break;
		TH_CUT:
			//if( Bhv_ThroughPassCut().execute( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best through cut"<<std::endl;
				return true;
			}
			return false;
			break;
		INTERCEPT:
			if( Body_Intercept( false ).execute( agent ) )
			{
				if ( agent->world().ball().distFromSelf() < rcsc::ServerParam::i().visibleDistance() )	            
	                agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	            else
	                agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
				std::cout<<"best intercept"<<std::endl;
				return true;
			}
			return false;
			break;
		RUN:
			if( Body_Pass::requestedRun( agent ) )
			{
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				std::cout<<"best run"<<std::endl;
				return true;
			}
			return false;
			break;
		
		default:
			std::cout<<"no best"<<std::endl;
			return false;
			break;
	}
	
	return false;
}

/*------------------------------------------*/
int
Bhv_BestAction::calculate( rcsc::PlayerAgent * agent )
{
	std::vector< std::pair< int , type > >action;
	action.clear();
	int val = 0;
	type typ;

	{
		val = direct( agent );
		val *= 2;
		typ = DIRECT;
		action.push_back( std::make_pair( val , typ ) );
		
		val = through( agent );
		val *= 2;
		typ = THROUGH;
		action.push_back( std::make_pair( val , typ ) );
		
		val = dash( agent );
		val *= 2;
		typ = DASH;
		action.push_back( std::make_pair( val , typ ) );
		
		val = dribble( agent );
		val *= 2;
		typ = DRIBBLE;
		action.push_back( std::make_pair( val , typ ) );
		
		val = tactic_kick( agent );
		val *= 2;
		typ = TAC_KICK;
		action.push_back( std::make_pair( val , typ ) );
		
		val = clear( agent );
		val *= 2;
		typ = CLEAR;
		action.push_back( std::make_pair( val , typ ) );
		
		val = corner( agent );
		val *= 2;
		typ = CORNER;
		action.push_back( std::make_pair( val , typ ) );
		
		val = shoot( agent );
		val *= 2;
		typ = SHOOT;
		action.push_back( std::make_pair( val , typ ) );
		
		val = advance( agent );
		val *= 2;
		typ = ADVANCE;
		action.push_back( std::make_pair( val , typ ) );
		
		val = hold( agent );
		val *= 2;
		typ = HOLD;
		action.push_back( std::make_pair( val , typ ) );
		
		val = tackle( agent );
		val *= 2;
		typ = TACKLE;
		action.push_back( std::make_pair( val , typ ) );
		
		val = block( agent );
		val *= 2;
		typ = BLOCK;
		action.push_back( std::make_pair( val , typ ) );
		
		val = defense_breaker( agent );
		val *= 2;
		typ = DEF_BREAK;
		action.push_back( std::make_pair( val , typ ) );
		
		val = positioning( agent );
		val *= 2;
		typ = POSITION;
		action.push_back( std::make_pair( val , typ ) );
		
		val = hassle( agent );
		val *= 2;
		typ = HASSLE;
		action.push_back( std::make_pair( val , typ ) );
		
		val = mark( agent );
		val *= 2;
		typ = MARK;
		action.push_back( std::make_pair( val , typ ) );
		
		val = tactic_move( agent );
		val *= 2;
		typ = TAC_MOVE;
		action.push_back( std::make_pair( val , typ ) );
		
		val = through_cut( agent );
		val *= 2;
		typ = TH_CUT;
		action.push_back( std::make_pair( val , typ ) );
		
		val = intercept( agent );
		val *= 2;
		typ = INTERCEPT;
		action.push_back( std::make_pair( val , typ ) );
		
		val = run( agent );
		val *= 2;
		typ = RUN;
		action.push_back( std::make_pair( val , typ ) );
	}
	
	val = 1;
	for( uint i = 0 ; i < action.size() ; i++ )
	{
		if( action[i].first >= val )
		{
			val = action[i].first;
			typ = action[i].second;
		}
	}
	
	action.clear();
	return typ;
}

/*------------------------------------------*/
int
Bhv_BestAction::direct( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	
    if ( ! wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;

	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::through( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return 0;
        
	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::dash( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().dash() )
    {
		return 0;
    }

    if ( ! wm.self().isKickable() )
        return 0;
	
	if( wm.self().pos().x < -20 )
		return 0;
	
	if( wm.self().pos().x > 40 && wm.self().pos().absY() < 17 )
		return 0;
		
	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.45 )
		return 0;

	if( wm.self().isFrozen() )
		return 0;

	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::dribble( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( ! wm.self().isKickable() )
        return 0;
	
	
	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::tactic_kick( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().tactics() )
    {
		return 0;
    }
    
    if ( ! wm.self().isKickable() )
        return 0;
    
	
	if( wm.self().isFrozen() )
		return 0;

	return 0;
}
/*------------------------------------------*/
int
Bhv_BestAction::clear( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( ! wm.self().isKickable() )
        return 0;
        
	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::corner( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	
	if ( ! wm.self().isKickable() )
        return 0;
    
    if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::shoot( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( ! wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;
	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::advance( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( ! wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;

	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::hold( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	
	if ( ! wm.self().isKickable() )
        return 0;
        
	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::tackle( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().tackle() )
    {
		return 0;
    }

	if( ! wm.existKickableOpponent() && wm.self().isKickable() )
		return 0;
	if( wm.existKickableTeammate() || wm.self().isKickable() )
		return 0;

	if ( wm.self().isKickable() )
        return 0;

	if( wm.self().isFrozen() )
		return 0;
	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::block( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().block() )
    {
		return 0;
    }
	if( wm.existKickableTeammate() )//&& (! wm.existKickableOpponent() ) )
		return 0;
	
	if( wm.self().isFrozen() )
		return 0;

	
	if ( wm.self().isKickable() )
        return 0;
        
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::defense_breaker( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().def_break() )
    {
		return 0;
    }

	
	if ( wm.self().isKickable() )
        return 0;

	if( wm.self().isFrozen() )
		return 0;

        
	return 0;
}
/*------------------------------------------*/
int
Bhv_BestAction::positioning( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	
	if ( wm.self().isKickable() )
        return 0;
        
	if( wm.self().isFrozen() )
		return 0;
    
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::hassle( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().hassle() )
    {
		return 0;
    }

	if ( wm.self().isKickable() )
        return 0;

	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::mark( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().mark() )
    {
		return 0;
    }


	//if ( ! SamplePlayer::instance().mark_escape() )
    {
		return 0;
    }

	if ( wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;
	
	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::tactic_move( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	//if ( ! SamplePlayer::instance().tactics() )
    {
		return 0;
    }

	if ( wm.self().isKickable() )
        return 0;

	if( wm.self().isFrozen() )
		return 0;

	return 0;
}
/*------------------------------------------*/
int
Bhv_BestAction::through_cut( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;

	return 100;
}
/*------------------------------------------*/
int
Bhv_BestAction::intercept( rcsc::PlayerAgent * agent )
{
	const rcsc::WorldModel & wm = agent->world();
	
	if ( wm.self().isKickable() )
        return 0;
	
	if( wm.self().isFrozen() )
		return 0;
	
	const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min < mate_min + 3
                   && self_min < opp_min + 4 )
              )
         )
    {
		return 100;
	}

	return 0;
}
/*------------------------------------------*/
int
Bhv_BestAction::run( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.self().isKickable() )
        return 0;

	if( wm.self().isFrozen() )
		return 0;
    
	if( wm.audioMemory().runRequest().empty() )
		return 0;

	if( wm.audioMemory().runRequestTime().cycle() <= wm.time().cycle() - 5 ) 
		return 0;
		
	if( wm.self().unum() != wm.audioMemory().runRequest().front().runner_ )
		return 0;
	
	if( wm.self().pos().dist( wm.audioMemory().runRequest().front().pos_ ) < 5.0 )
		return 0;

	return 100;
}

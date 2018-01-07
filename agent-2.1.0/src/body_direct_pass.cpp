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

#include "body_pass.h"
#include "body_direct_pass.h"
#include "body_kick_multi_step.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/common/server_param.h>

/*-------------------------------------------------------------------*/

bool Body_DirectPass::execute( rcsc::PlayerAgent * agent )
{

	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return false;
        
    rcsc::Vector2D target_point(50.0, 0.0);
    double first_speed = 0.0;
    int receiver = 0;

	if( M_mode == "playOn" )
	{
		if ( ! evaluate( agent , &target_point, &first_speed, &receiver ) )
			return false;
	}
	if( M_mode == "indirect" )
	{
		if ( ! indirectPass( agent , &target_point, &first_speed, &receiver ) )
			if ( ! evaluate( agent , &target_point, &first_speed, &receiver ) )
				return false;
	}
	if( M_mode == "short" )
	{
		if ( ! shortPass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	}
	if( M_mode == "deadBall" )
	{
		if ( ! deadBallPass( agent , &target_point, &first_speed, &receiver ) )
			return false;
	}

	if( M_mode == "old" )
	{
		if ( ! old_direct( agent , &target_point, &first_speed, &receiver ) )
			//if ( ! old_short( agent , &target_point, &first_speed, &receiver ) )
				return false;
	}
	
	if( receiver == 0 )
	{
		std::cout<<"hold ball"<<std::endl;
		rcsc::Body_HoldBall().execute( agent );
		return false;
	}
	
	
	/*
    Body_KickMultiStep( target_point, first_speed, false ).execute( agent );
	
	//if ( agent->world().gameMode().type() != GameMode::PlayOn )
    {
        //agent->setIntention( new rcsc::IntentionKick( target_point, first_speed , 3, false, wm.time() ) );
    }
	
    int kick_step = ( wm.gameMode().type() != rcsc::GameMode::PlayOn
                      && wm.gameMode().type() != rcsc::GameMode::GoalKick_
                      ? 1
                      : 3 );
    if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, kick_step ).execute( agent ) )
    {
		first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
		Body_KickOneStep( target_point, first_speed , true ).execute( agent );
		
        if ( wm.gameMode().type() != rcsc::GameMode::PlayOn
             && wm.gameMode().type() != rcsc::GameMode::GoalKick_ )
        {
            first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
            Body_KickOneStep( target_point, first_speed , true ).execute( agent );

        }
        else
            return false;
    }
    */
    
    
	if(! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
	{
		if( agent->world().existKickableOpponent() )
	    {
	        Body_KickOneStep( target_point , first_speed ).execute( agent );
	    }
	}

    
    //if (! Body_SmartKick( target_point , first_speed , first_speed * 0.96 , 3 ).execute( agent ) )
	//	Body_KickOneStep( target_point , first_speed , true ).execute( agent );
		

    Body_Pass::say_pass( agent , receiver , target_point );
    //std::cout<<"["<<wm.time().cycle()<<"] point=("<<target_point.x<<","<<target_point.y<<") => "<<receiver<<std::endl;
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    return true;
}

/*--------------------------------------------------------------*/

bool Body_DirectPass::evaluate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{		
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< std::pair< rcsc::PlayerObject * , double > > teammates;
	int unum = 11;
	bool found = false;
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 10 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if ( (*tm)->isTackling() )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.x > offside - 1.0 )
			continue;
		if( tm_pos.x > 50.0 || tm_pos.absY() > 31.0 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 4.0 )
			continue;
		//if( ( tm_pos.x < my_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 40.0 && ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) ) )
		//	continue;
		if( tm_pos.x < my_pos.x - 15.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( Body_Pass::exist_opponent3( agent , tm_pos ) )
			continue;
			
		double value = 10.0;
		double distance = -1.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		if( distance > 0.0 )
			value *= distance;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		value *= std::fabs( tm_pos.x - my_pos.x );
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = -1;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		if( count > 0 )
			value /= count;
		value /= ( std::fabs( tm_pos.y - my_pos.y ) + 1.0 );
		if( (*tm)->posCount() > 0 )
			value /= (*tm)->posCount();
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value *= 100;

		teammates.push_back( std::make_pair( *tm , value ) );
		found = true;
	}
	
	if(! found)
		return false;
		
	std::pair< rcsc::PlayerObject * , double >tmp;
	for(uint i = 0 ; i < teammates.size() - 1 ; i++)
	{
		for(uint j = i + 1 ; j < teammates.size() ; j++)
		{
			if( teammates[i].second < teammates[j].second )
			{
				tmp = teammates[ i ];
				teammates[ i ] = teammates[ j ];
				teammates[ j ] = tmp;
			}
		}
	}
	
	for( uint i = 0 ; i < teammates.size() ; i ++)
	{
		double first_speed1 = Body_Pass::first_speed( agent , teammates[i].first->pos() , 'd' );
		rcsc::Vector2D first( 0 , 0 );
		if ( teammates[i].first->velCount() < 3 )
	        first = teammates[i].first->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( teammates[i].first->velCount() + 1, 2 );
		
		if(! Body_Pass::exist_opponent3( agent , teammates[i].first->pos() + first ) )
		{
		    * target_point = teammates[i].first->pos() + first;
			* receiver = teammates[i].first->unum();
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
	}
	
    return false;
    
}

/*--------------------------------------------------------------*/

bool Body_DirectPass::shortPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver )
{
	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D tm_pos( 52 , 0 );
	bool found = false;
	int unum;
	double offside = wm.offsideLineX();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -36 && (*tm)->pos().absY() < 18 )
			continue;
		
		tm_pos = (*tm)->pos();
		
		if( tm_pos.x < my_pos.x - 20.0 )
			continue;
		if( tm_pos.x > offside + 1.0 )
			continue;
		if( tm_pos.absY() > 34 )
			continue;
		
		double dist = my_pos.dist( tm_pos );
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.5 , dist , deg - 10 , deg + 10 );
		if( ! wm.existOpponentIn( pass , 10 , true ) )
		{
			unum = (*tm)->unum();
			found = true;
			break;
		}
	}
	
	if( found )
	{
		double first_speed1 = Body_Pass::first_speed( agent , tm_pos , 'd' );
		rcsc::AngleDeg deg = ( tm_pos - wm.ball().pos() ).th();
		if(! Body_Pass::exist_opponent3( agent , tm_pos ) )
		{
			* target_point = tm_pos;
			* receiver = unum;
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		
		/*
		if(! Body_Pass::exist_opponent( agent , tm_pos , deg.degree() ) )
		{
			* target_point = tm_pos;
			* receiver = unum;
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		if(! Body_Pass::exist_opponent2( agent , teammates[i] ) )
		{
			* target_point = teammates[i];
			* receiver = unum;
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		*/
	}
	
	return false;
}

/*--------------------------------------------------------------*/

bool Body_DirectPass::indirectPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver )
{
	return false;
	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	double defenseLineX = 0.0;
	int count = 0;
	const rcsc::PlayerPtrCont & opponent = wm.opponentsFromBall();
	const rcsc::PlayerPtrCont::const_iterator opponent_end = opponent.end();
	for ( rcsc::PlayerPtrCont::const_iterator  opp = opponent.begin(); opp != opponent_end; opp++ )
	{
		if( (! (*opp)) || (*opp)->isGhost() )
			continue;
		if( (*opp)->pos().dist( wm.ball().pos() ) > 36 )
			continue;		
		if( (*opp)->pos().absY() > 34 )
			continue;
		if( (*opp)->pos().x < wm.ball().pos().x - 5.0 )
			continue;
			
		defenseLineX += (*opp)->pos().x;
		count++;
	}
	if( count == 0 )
		return false;
	defenseLineX /= count;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = rcsc::inertia_final_distance( max_ball_s,ball_d );
	
	double teammates[12];
	for(int i = 0 ; i < 12 ; i++)
		teammates[i] = 0.0;
		
	rcsc::Vector2D my_pos = wm.self().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -30 && (*tm)->pos().absY() < 17 )
			continue;
		
		rcsc::Vector2D tm_pos = (*tm)->pos();
		int unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );

		if( tm_pos.x < my_pos.x - 10.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( tm_pos.x > offside - 1.0 )
			continue;
		if( tm_pos.absY() > 32 )
			continue;
		
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.5 , dist , deg - 12.5 , deg + 12.5);
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		
		if( tm_pos.x > defenseLineX )
			teammates[ unum ] += 200;
			
		rcsc::Sector2D front( tm_pos , 0.5 , 10 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			teammates[ unum ] += 200;
			
		if( my_pos.dist( tm_pos ) > 20 )
			teammates[ unum ] += 200;
			
		if( tm_pos.x > my_pos.x + 30 )
			teammates[ unum ] += 300;
		else
		if( tm_pos.x > my_pos.x + 20 )
			teammates[ unum ] += 200;
		else
		if( tm_pos.x > my_pos.x + 10 )
			teammates[ unum ] += 100;
		else
			teammates[ unum ] += 0;
		
	}
	
	double tmp;
	for(int i = 0 ; i < 11 ; i++)
		for(int j = i + 1 ; j < 12 ; j++)
			if( teammates[ i ] < teammates [ j ] )
			{
				tmp = teammates[ i ];
				teammates[ i ] = teammates[ j ];
				teammates[ j ] = tmp;
			}
	
	for( int i = 0 ; i < 12 ; i ++)
	{
		const rcsc::AbstractPlayerObject * tm = wm.teammate( i );
		if( (! tm) || tm->isGhost() || tm->posCount() > 3  )
			continue;
		if( tm->goalie() || tm->unum() > 11 || tm->unum() < 2 )
			continue;
		
		rcsc::Vector2D tm_pos = tm->pos();
		double end_speed = 1.35;
	    double first_speed1 = 10.0;
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, my_pos.dist( tm_pos ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.1;
	    }
	    while ( end_speed > 0.8 );
	
		first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
		//first_speed1 *= ServerParam::i().ballDecay();
		double next_speed = first_speed1 * rcsc::ServerParam::i().ballDecay();
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		double dist = my_pos.dist( tm_pos );
		rcsc::Vector2D first_vel = rcsc::Vector2D::polar2vector( first_speed1 , deg );
		bool pass = true;
		const rcsc::PlayerPtrCont::const_iterator opp_end = wm.opponentsFromSelf().end();
		for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin(); it != opp_end; ++it )
		{
	        if ( ! (*it) || (*it)->posCount() > 10 )
				continue;
	        if ( (*it)->isGhost() && (*it)->posCount() > 3 )
				continue;	
	        if ( ( (*it)->angleFromSelf() - deg ).abs() > 100.0 )
	            continue;
			double dash = 0.8 * std::min( 5, (*it)->posCount() );
			rcsc::Vector2D dist2pos = (*it)->pos() - wm.ball().pos() - first_vel;
			dist2pos.rotate( -deg );
			if ( dist2pos.x <= 0.0 || dist2pos.x >= dist )
				continue;
            double opp_dist = dist2pos.absY() - dash - rcsc::ServerParam::i().defaultKickableArea() - 0.1;
            if ( opp_dist >= 0.0 )
				continue;
			else
				pass = false;
            double projection = rcsc::calc_length_geom_series( next_speed, dist2pos.x, rcsc::ServerParam::i().ballDecay() );
            if ( projection >= 0.0 && opp_dist >= projection )
				continue;
			pass = false;
        }
		
		/*
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 10 , deg + 10 );
		if(! wm.existOpponentIn( pass, 10 , false ) )
		{
			* target_point = tm_pos;
			* receiver = tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		*/
		if( pass )
		{
			* target_point = tm_pos;
			* receiver = tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"indirect pass => "<<tm->unum()<<std::endl;
			return true;
		}
	}
	return false;
}

/*--------------------------------------------------------------*/

bool Body_DirectPass::deadBallPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver )
{
	return false;
	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D tm_pos( 0 , 0 );
	bool found = false;
	int unum = 6;
	const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -36 && (*tm)->pos().absY() < 18 )
			continue;
		
		tm_pos = (*tm)->pos();
		
		double dist = my_pos.dist( tm_pos );
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.5 , dist , deg - 12.5 , deg + 12.5);
		if( ! wm.existOpponentIn( pass , 10 , true ) )
		{
			unum = (*tm)->unum();
			found = true;
			break;
		}
	}
	
	if( found )
	{
		double end_speed = 1.35;
	    double first_speed1 = 100.0;
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, my_pos.dist( tm_pos ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.1;
	    }
	    while ( end_speed > 0.8 );
	
		first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		* target_point = tm_pos;
		* receiver = unum;
		* first_speed = first_speed1;
		//std::cout<<"deadBall pass => "<<unum<<std::endl;
		return true;
	}
	
	rcsc::Body_HoldBall().execute( agent );
	return false;
}



/*--------------------------------------------*/
void
Body_DirectPass::fill_vector( rcsc::PlayerAgent * agent )
{
				
    const rcsc::WorldModel & wm = agent->world();

	//double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	int unum = 11;
	rcsc::Vector2D dummy( 0 , 0 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.absX() > 50 || tm_pos.absY() > 32 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 2.0 )
			continue;
		if( ( tm_pos.x < my_pos.x + 5.0 )
			&& (*tm)->angleFromSelf().abs() > 40.0
			&& ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) )
		  )
			continue;
		if( tm_pos.x < my_pos.x - 8.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		teammates[ unum ] = tm_pos;
		double distance = 100.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		value[ unum ] += 10.0 * std::max( 0.0, 30.0 - distance );
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		value[ unum ] += 10.0 * (*tm)->posCount();
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		value[ unum ] += 10.0 * std::max( 5.0, std::fabs( tm_pos.y - my_pos.y ) );
		if( tm_pos.x > my_pos.x + 30.0 )
			value[ unum ] += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value[ unum ] += 200;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value[ unum ] += 100;
		else
		if( tm_pos.x > my_pos.x )
			value[ unum ] += 50;
		else
			value[ unum ] += 0;
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value[ unum ] +=  10.0 * count;
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 300;
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        teammates[ unum ] += first;
	    }
	    M_pass.push_back( std::make_pair( teammates[ unum ] , unum ) );
	    //pass pass_route( teammates[ unum ] , value[ unum ] , unum );
	    pass pass_route;
	    pass_route.point = teammates[ unum ];
	    pass_route.value = value[ unum ];
	    pass_route.unum = unum;
	    P_pass.push_back( pass_route );
	}

}

/*--------------------------------------------------------------*/

bool Body_DirectPass::old_direct( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point, double * first_speed, int * receiver )
{	
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	int unum = 0;
	bool found = false;
	rcsc::Vector2D dummy( 0 , 0 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
		
		rcsc::Vector2D tm_pos = (*tm)->pos();
		rcsc::Vector2D my_pos = wm.self().pos();
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		
		if( tm_pos.x < my_pos.x - 15.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( tm_pos.absY() > 31 )
			continue;
		
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.ball().pos().x > 36 )
		{
			if( wm.existOpponentIn( pass , 10 , true ) )
				continue;
		}
		else
		//if( ! Body_Pass::can_pass( agent , tm_pos ) )
		if( Body_Pass::exist_opponent3( agent , tm_pos ) )
			continue;

		teammates[ unum ] = tm_pos;
		if( tm_pos.x > offside - 1.0 )
			value[ unum ] -= 300;
		
		if( tm_pos.x > my_pos.x + 25.0 )
			value[ unum ] += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value[ unum ] += 200;
		else
		if( tm_pos.x > my_pos.x + 15.0 )
			value[ unum ] += 100;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value[ unum ] += 50;
		else
			value[ unum ] += 0;
		
		rcsc::Circle2D round( tm_pos , 2.0 );
		if( wm.existOpponentIn( round , 10 , true ) )
			value[ unum ] -= 200;
		
		rcsc::Sector2D front( tm_pos , 0.0 , 5.0 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 100;
		
		found = true;
	}
	
	if(! found)
		return false;
		
	rcsc::Vector2D tmp;
	for(int i = 0 ; i < 11 ; i++)
		for(int j = i + 1 ; j < 12 ; j++)
			if( value[ i ] < value[ j ] )
			{
				tmp = teammates[ i ];
				//unum = i;
				teammates[ i ] = teammates[ j ];
				//i = j;
				teammates[ j ] = tmp;
				//j = unum;
			}
	
	for( int i = 0 ; i < 12 ; i ++)
	{
		//double first_speed1 = Body_Pass::first_speed( agent , teammates[i] , 'd' );
		double end_speed = 1.5;
	    double first_speed1 = 3.0;
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, wm.ball().pos().dist( teammates[i] ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.1;
	    }
	    while ( end_speed > 0.8 );
	
		first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		double dist = 1000;
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( teammates[i] , 10 , &dist );
		if( (! tm) )
			continue;
		if( tm->goalie() || tm->unum() > 11 || tm->unum() < 2 )
			continue;
		rcsc::AngleDeg deg = ( teammates[i] - wm.ball().pos() ).th();
		//if(! Body_Pass::exist_opponent3( agent , teammates[i] ) )
		rcsc::Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.ball().pos().x > 36 )
		{
			if( ! wm.existOpponentIn( pass , 10 , true ) )
			{
				* target_point = teammates[i];
				* receiver = tm->unum();
				* first_speed = first_speed1;
				//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
				return true;	
			}
		}
		else
		//if( ! Body_Pass::can_pass( agent , tm_pos ) )
		if(! Body_Pass::exist_opponent3( agent , teammates[i] ) )
		{
			* target_point = teammates[i];
			* receiver = tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		
		/*
		if(! Body_Pass::exist_opponent( agent , teammates[i] , deg.degree() ) )
		{
			* target_point = teammates[i];
			* receiver = tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		if(! Body_Pass::exist_opponent2( agent , teammates[i] ) )
		{
			* target_point = teammates[i];
			* receiver = tm->unum();
			* first_speed = first_speed1;
			//std::cout<<"direct pass => "<<tm->unum()<<std::endl;
			return true;
		}
		*/
	
	}
	
    return false;
}

/*--------------------------------------------------------------*/

bool Body_DirectPass::old_short( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver )
{
		
    const rcsc::WorldModel & wm = agent->world();

	if(! ( target_point && first_speed && receiver ) )
		return false;
	
	rcsc::Vector2D my_pos;
	rcsc::Vector2D tm_pos;
	bool found = false;
	int unum;
	double offside = wm.offsideLineX();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromBall();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
		
		tm_pos = (*tm)->pos();
		my_pos = wm.self().pos();
		
		if( tm_pos.x < my_pos.x - 20.0 )
			continue;
		if( tm_pos.x > offside - 1.0 )
			continue;
		if( tm_pos.absY() > 34 )
			continue;
		
		double dist = my_pos.dist( tm_pos );
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		rcsc::Sector2D pass( wm.ball().pos() , 0.5 , dist , deg - 12.5 , deg + 12.5);
		if( ! wm.existOpponentIn( pass , 10 , true ) )
		{
			unum = (*tm)->unum();
			found = true;
			break;
		}
	}
	if( found )
	{
		double end_speed = 1.5;
	    double first_speed1 = 100.0;
	    do
	    {
	        first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, my_pos.dist( tm_pos ) , rcsc::ServerParam::i().ballDecay() );
	        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
	        {
	            break;
	        }
	        end_speed -= 0.1;
	    }
	    while ( end_speed > 0.8 );
	
		first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
		* target_point = tm_pos;
		* receiver = unum;
		* first_speed = first_speed1;
		//cout<<"short pass => "<<unum<<endl;
		return true;
	}

	return false;
}

/*----------------------------------*/
bool
Body_DirectPass::test( rcsc::PlayerAgent * agent )
{
	
	const rcsc::WorldModel & wm = agent->world();

	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	int unum = 11;
	bool found = false;
	rcsc::Vector2D dummy( 0 , 0 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
		
		rcsc::Vector2D tm_pos = (*tm)->pos();
		rcsc::Vector2D my_pos = wm.self().pos();
		double dist = my_pos.dist( tm_pos );
		if( tm_pos.x < my_pos.x - 10.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( tm_pos.absY() > 31 )
			continue;
		
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.ball().pos().x > 36 )
		{
			if( wm.existOpponentIn( pass , 10 , true ) )
				continue;
		}
		else
		//if( ! Body_Pass::can_pass( agent , tm_pos ) )
		if( Body_Pass::exist_opponent3( agent , tm_pos ) )
			continue;
			
		unum = (*tm)->unum();
		teammates[ unum ] = tm_pos;
		if( tm_pos.x > offside - 1.0 )
			value[ unum ] -= 200;
		
		if( tm_pos.x > my_pos.x + 25.0 )
			value[ unum ] += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value[ unum ] += 200;
		else
		if( tm_pos.x > my_pos.x + 15.0 )
			value[ unum ] += 100;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value[ unum ] += 50;
		else
			value[ unum ] += 0;
		
		rcsc::Circle2D round( tm_pos , 2.0 );
		if( wm.existOpponentIn( round , 10 , true ) )
			value[ unum ] -= 200;
		
		rcsc::Sector2D front( tm_pos , 0.0 , 5.0 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 100;
		
		found = true;
	}
	
	if(! found)
		return false;
		
	rcsc::Vector2D tmp;
	double val = -10000;
	for(int i = 0 ; i < 12 ; i++)
		if( value[ i ] > val )
		{
			tmp = teammates[ i ];
			val = value[ i ];
		}
	{
		if( tmp.x < wm.ball().pos().x - 1.0 )
			return false;
		if( tmp.dist( wm.ball().pos() ) < 4.0 )
			return false;
		if( tmp.dist( wm.ball().pos() ) > 32.0 )
			return false;
		
		//AngleDeg deg = ( teammates[0] - wm.ball().pos() ).th();
		//double dist = wm.ball().pos().dist( teammates[0] );
		//Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12.5 , deg + 12.5 );
		//if( wm.existOpponentIn( pass, 10 , false ) )
		//{
		//	return false;
		//}
		
	}
	
	return true;
}


bool
Body_DirectPass::test( rcsc::PlayerAgent * agent , rcsc::Vector2D * point )
{

	const rcsc::WorldModel & wm = agent->world();

	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	int unum = 11;
	bool found = false;
	rcsc::Vector2D dummy( 0 , 0 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->pos().x < -29 && (*tm)->pos().absY() < 18 )
			continue;
		
		rcsc::Vector2D tm_pos = (*tm)->pos();
		rcsc::Vector2D my_pos = wm.self().pos();
		double dist = my_pos.dist( tm_pos );
		if( tm_pos.x < my_pos.x - 10.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( tm_pos.absY() > 31 )
			continue;
		
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		//if( Body_Pass::exist_opponent( agent , tm_pos , deg.degree() ) )
		
		if( wm.ball().pos().x > 36 )
		{
			if( wm.existOpponentIn( pass , 10 , true ) )
				continue;
		}
		else
		//if( ! Body_Pass::can_pass( agent , tm_pos ) )
		if( Body_Pass::exist_opponent3( agent , tm_pos ) )
			continue;
			
		unum = (*tm)->unum();
		teammates[ unum ] = tm_pos;
		if( tm_pos.x > offside - 1.0 )
			value[ unum ] -= 200;
		
		if( tm_pos.x > my_pos.x + 25.0 )
			value[ unum ] += 300;
		else
		if( tm_pos.x > my_pos.x + 20.0 )
			value[ unum ] += 200;
		else
		if( tm_pos.x > my_pos.x + 15.0 )
			value[ unum ] += 100;
		else
		if( tm_pos.x > my_pos.x + 10.0 )
			value[ unum ] += 50;
		else
			value[ unum ] += 0;
		
		rcsc::Circle2D round( tm_pos , 2.0 );
		if( wm.existOpponentIn( round , 10 , true ) )
			value[ unum ] -= 200;
		
		rcsc::Sector2D front( tm_pos , 0.0 , 5.0 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 100;
		
		found = true;
	}
	
	if(! found)
		return false;
		
	rcsc::Vector2D tmp;
	double val = -10000;
	for(int i = 0 ; i < 12 ; i++)
		if( value[ i ] > val )
		{
			tmp = teammates[ i ];
			val = value[ i ];
		}
	{
		//if( tmp.x < wm.ball().pos().x - 1.0 )
		//	return false;
		if( tmp.dist( wm.ball().pos() ) < 4.0 )
			return false;
		if( tmp.dist( wm.ball().pos() ) > 32.0 )
			return false;
		
		//AngleDeg deg = ( teammates[0] - wm.ball().pos() ).th();
		//double dist = wm.ball().pos().dist( teammates[0] );
		//Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12.5 , deg + 12.5 );
		//if( wm.existOpponentIn( pass, 10 , false ) )
		//{
		//	return false;
		//}
	}
	
	*point = tmp;
	return true;
}



bool
Body_DirectPass::test_new( rcsc::PlayerAgent * agent , rcsc::Vector2D * point )
{
	const rcsc::WorldModel & wm = agent->world();

	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
    std::vector< std::pair< rcsc::PlayerObject * , double > > teammates;
	int unum = 11;
	bool found = false;
	rcsc::Vector2D target = * point;
	
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 10 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if ( (*tm)->isTackling() )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.x > offside - 1.0 )
			continue;
		if( tm_pos.x > 50.0 || tm_pos.absY() > 31.0 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 4.0 )
			continue;
		//if( ( tm_pos.x < my_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 40.0 && ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) ) )
		//	continue;
		if( tm_pos.x < my_pos.x - 15.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		if( Body_Pass::exist_opponent3( agent , tm_pos ) )
			continue;
			
		double value = 10.0;
		double distance = -1.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		if( distance > 0.0 )
			value *= distance;
		value *= std::fabs( tm_pos.x - my_pos.x );
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = -1;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		if( count > 0 )
			value /= count;
		value /= ( std::fabs( tm_pos.y - my_pos.y ) + 1.0 );
		if( (*tm)->posCount() > 0 )
			value /= (*tm)->posCount();
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value *= 100;

		teammates.push_back( std::make_pair( *tm , value ) );
		found = true;
	}
	
	if(! found)
		return false;
		
	std::pair< rcsc::PlayerObject * , double >tmp;
	for(uint i = 0 ; i < teammates.size() - 1 ; i++)
	{
		for(uint j = i + 1 ; j < teammates.size() ; j++)
		{
			if( teammates[i].second < teammates[j].second )
			{
				tmp = teammates[ i ];
				teammates[ i ] = teammates[ j ];
				teammates[ j ] = tmp;
			}
		}
	}
	
	rcsc::Vector2D first( 0 , 0 );
	if ( teammates[0].first->velCount() < 3 )
        first = teammates[0].first->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( teammates[0].first->velCount() + 1, 2 );
	
	if(! Body_Pass::exist_opponent3( agent , teammates[0].first->pos() + first ) )
	{
	    * point = teammates[0].first->pos() + first;
		return true;
	}
	
    return false;
}


bool
Body_DirectPass::test_new( rcsc::PlayerAgent * agent  )
{
	const rcsc::WorldModel & wm = agent->world();

	//if( Bhv_Pass::isLastValid( wm , &target_point , &first_speed , &receiver ) )
	//	return true;
	
	double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.8 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	int unum = 11;
	bool found = false;
	rcsc::Vector2D dummy( 0 , 0 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	rcsc::Vector2D my_pos = wm.self().pos();
	rcsc::Vector2D ball_pos = wm.ball().pos();
	const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
	for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
	{
		if( (! (*tm)) || (*tm)->posCount() > 8 )
			continue;
		if( (*tm)->isGhost() && (*tm)->posCount() > 3 )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if( tm_pos.x < -29 && tm_pos.absY() < 18 )
			continue;
		if( tm_pos.x > offside - 0.5 )
			continue;
		if( tm_pos.absX() > 50 || tm_pos.absY() > 32 )
			continue;
		unum = (*tm)->unum();
		double dist = my_pos.dist( tm_pos );
		if( dist < 4.0 )
			continue;
		//if( ( tm_pos.x < my_pos.x + 5.0 ) && (*tm)->angleFromSelf().abs() > 40.0 && ( !( tm_pos.x > wm.ourDefenseLineX() + 10.0 ) ) )
		//	continue;
		if( tm_pos.x < my_pos.x - 5.0 )
			continue;
		if( dist > max_dp_dist )
			continue;
		teammates[ unum ] = tm_pos;
		double distance = 100.0;
		wm.getOpponentNearestTo( tm_pos , 20 , &distance );
		value[ unum ] += 10.0 * distance;
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		value[ unum ] += 10.0 * ( 10.0 / ( (*tm)->posCount() ) );
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		value[ unum ] += 100.0 * ( 10.0 / std::fabs( tm_pos.y - my_pos.y ) );
		value[ unum ] += 20.0 * std::fabs( tm_pos.x - my_pos.x );
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		int count = 0;
		wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		value[ unum ] +=  10.0 * ( 10.0 / count );
		rcsc::Sector2D front( tm_pos , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 300;
		
		if ( (*tm)->velCount() < 3 )
	    {
	        rcsc::Vector2D first = (*tm)->vel() / rcsc::ServerParam::i().defaultPlayerDecay() * std::min( (*tm)->velCount() + 1, 2 );
	        teammates[ unum ] += first;
	    }

		found = true;
	}
	
	if(! found)
		return false;
		
	rcsc::Vector2D tmp;
	double val = -10000;
	for(int i = 0 ; i < 12 ; i++)
		if( value[ i ] > val )
		{
			tmp = teammates[ i ];
			val = value[ i ];
		}
	{
		if( tmp.x < wm.ball().pos().x + 4.0 )
			return false;
		if( tmp.dist( wm.ball().pos() ) < 4.0 )
			return false;
		if( tmp.dist( wm.ball().pos() ) > 32.0 )
			return false;
		
		//AngleDeg deg = ( teammates[0] - wm.ball().pos() ).th();
		//double dist = wm.ball().pos().dist( teammates[0] );
		//Sector2D pass( wm.ball().pos() , 0.0 , dist , deg - 12.5 , deg + 12.5 );
		//if( wm.existOpponentIn( pass, 10 , false ) )
		//{
		//	return false;
		//}
		
		if( Body_Pass::exist_opponent3( agent , tmp ) )
		{
			return false;
		}
	}
	
	return true;
}

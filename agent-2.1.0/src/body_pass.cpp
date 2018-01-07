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

#include <vector>
#include "strategy.h"
#include "bhv_predictor.h"
#include "body_pass.h"
#include "body_through_pass.h"
#include "body_kick_multi_step.h"
#include "body_kick_one_step.h"
#include "body_go_to_point.h"
#include "body_intercept.h"
#include "body_smart_kick.h"

#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/body_turn_to_angle.h>
#include <rcsc/geom/sector_2d.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/audio_sensor.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/server_param.h>
#include <rcsc/common/audio_memory.h>

#define SEARCH_UNTIL_MAX_SPEED_AT_SAME_POINT
/*-------------------------------------------------------------------*/

bool Body_Pass::isLastValid( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , double * first_speed , int * receiver )
{

/*
	if(! ( target_point && first_speed && receiver ) )
		return false;
    
    const WorldModel & wm = agent->world();
    
    static GameTime lastTime( 0, 0 );
    static bool lastValid = false;
    static Vector2D lastTarget;
    static double lastSpeed = 0.0;
    static int LastUnum = Unum_Unknown;

    if ( lastTime == wm.time() )
    {
        if ( lastValid )
        {
            *target_point = lastTarget;
            *first_speed = lastSpeed;
            *receiver = lastUnum;
            return true;
        }
        return false;
    }

    lastTime = wm.time();
    lastValid = false;
    
    
    
    
    
    
    
        static GameTime lastTime( 0, 0 );
    static bool isLastValid = false;
    static Vector2D lastPoint;
    static double lastSpeed = 0.0;
    static int lastGiver = Unum_Unknown;

    if ( lastTime != world.time() )
    {
	lastTime    = world.time();
	isLastValid = false;

	const rcsc::PlayerPtrCont & opps = world.opponentsFromSelf();
	const rcsc::PlayerPtrCont & team = world.teammatesFromSelf();

    	const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
	const rcsc::PlayerPtrCont::const_iterator team_end = team.end();

	const double ball_speed = world.ball().vel().r();
	
	Vector2D myPos = world.self().pos();
        for ( rcsc::PlayerPtrCont::const_iterator itus = team.begin(); itus != team_end; ++itus )
	{
	
	    if ( (*itus)->isGhost() ) continue;
	    Vector2D teammate_pos = (*itus)->pos() ;
	    if ( fabs( teammate_pos.y - myPos.y ) > 18 ) continue;
	    if ( teammate_pos.x > myPos.x + 30 || teammate_pos.x < myPos.x - 1.0 ) continue; 
            bool flag = false;
	    for ( rcsc::PlayerPtrCont::const_iterator itop = opps.begin(); itop != opps_end; ++itop )
            {
		if ( (*itop)->isGhost() ) continue;
                if ( (*itop)->pos().dist( teammate_pos ) < 4.0 )
		{
		    flag = true;
		    break;
		}
	    }
	    if ( ! flag )
	    {
    		lastPoint = teammate_pos;
    		lastSpeed = ball_speed;
    		lastGiver = (*itus)->unum();
    		isLastValid = true;
		break;
	    }
	}
	
    }
    if ( isLastValid )
    {
        if ( target_point )	*target_point = lastPoint;
        if ( first_speed )	*first_speed  = lastSpeed;
        if ( receiver )		*receiver     = lastGiver;
    }

    return isLastValid;
 
*/

	return false;
}

/*----------------------------------------------------------*/

bool Body_Pass::simulate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , double * first_speed , int * receiver )
{
	return false;
}

/*--------------------------------------------------------*/

bool Body_Pass::passRequest( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point )
{
	if ( agent->world().self().isKickable() )
        return false;
	
    if ( agent->config().useCommunication() )
    {
		agent->addSayMessage( new rcsc::PassRequestMessage( pass_point ) );
		//std::cout<<agent->world().self().unum()<<" pass request"<<std::endl;
		return true;
    }
    
	return false;
}

/*--------------------------------------------------*/

bool Body_Pass::requestedPass( rcsc::PlayerAgent * agent )
{
	
    if ( ! agent->world().self().isKickable() )
        return false;

	if( agent->world().audioMemory().passRequest().empty() )
		return false;

	if( agent->world().audioMemory().passRequestTime().cycle() != agent->world().time().cycle() ) 
		return false;

    const rcsc::WorldModel & wm = agent->world();
    
    double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	rcsc::Vector2D target_point = wm.audioMemory().passRequest().front().pos_;
	int unum = wm.audioMemory().passRequest().front().sender_;
	rcsc::Vector2D my_pos = wm.self().pos();
	
	const rcsc::AbstractPlayerObject * tm = wm.teammate( unum );
	
	if( (! tm) || tm->isGhost() || tm->posCount() > 3  )
		return false;
	if( tm->goalie() || tm->unum() > 11 || tm->unum() < 2 )
		return false;
	
	double dist = my_pos.dist( target_point );
	if( tm->pos().x < my_pos.x - 5.0 )
		return false;
	if( dist > max_dp_dist )
		return false;
	if( tm->pos().absY() > 34 )
		return false;
	if( tm->pos().x > offside - 1.0 )
		return false;
				
	rcsc::AngleDeg deg = ( my_pos - target_point ).th();
	rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
	if( wm.existOpponentIn( pass , 10 , true ) )
		return false;

	rcsc::Circle2D round( target_point , 3.0 );
	if( wm.existOpponentIn( round , 10 , true ) )
		return false;
	
	double end_speed = 0.9;
    double first_speed = 100.0;
    do
    {
        first_speed = rcsc::calc_first_term_geom_series_last( end_speed, my_pos.dist( target_point ) , rcsc::ServerParam::i().ballDecay() );
        if ( first_speed < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        end_speed -= 0.1;
    }
    while ( end_speed > 0.5 );

	first_speed = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed );
	first_speed *= rcsc::ServerParam::i().ballDecay();
	
    //Body_KickMultiStep( target_point, first_speed, false ).execute( agent );
	
    int kick_step = ( wm.gameMode().type() != rcsc::GameMode::PlayOn
                      && wm.gameMode().type() != rcsc::GameMode::GoalKick_
                      ? 1
                      : 3 );
    if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, kick_step ).execute( agent ) )
    {
        if ( wm.gameMode().type() != rcsc::GameMode::PlayOn
             && wm.gameMode().type() != rcsc::GameMode::GoalKick_ )
        {
            first_speed = std::min( wm.self().kickRate() * rcsc::ServerParam::i().maxPower(), first_speed );
            Body_KickOneStep( target_point, first_speed , true ).execute( agent );

        }
        else
            return false;
    }
    
    if ( agent->config().useCommunication()
         && unum != rcsc::Unum_Unknown )
    {
        rcsc::Vector2D target_buf = target_point - my_pos;
        target_buf.setLength( 1.0 );

        agent->addSayMessage( new rcsc::PassMessage( unum,
                                               target_point + target_buf,
                                               agent->effector().queuedNextBallPos(),
                                               agent->effector().queuedNextBallVel() ) );
    }
    
	return true;

}

/*--------------------------------------------------*/

bool Body_Pass::requestedPass2( rcsc::PlayerAgent * agent )
{
	//return false;
	
	//return Body_ThroughPass("pointTo").execute( agent );
	
    if ( ! agent->world().self().isKickable() )
        return false;

	if( agent->world().audioMemory().passRequest().empty() )
		return false;
		
	if( agent->world().audioMemory().passRequestTime().cycle() != agent->world().time().cycle() ) 
		return false;
		
    const rcsc::WorldModel & wm = agent->world();
    
    double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = 0.9 * rcsc::inertia_final_distance( max_ball_s,ball_d );
	rcsc::Vector2D target_point( 52 , 0 );
	int unum = 11;
	bool found = false;
	rcsc::Vector2D ball_pos = wm.ball().pos();
	std::vector< rcsc::Vector2D > teammates(12);
	std::vector< double > value(12);
	rcsc::Vector2D dummy( 100 , 100 );
	for(int i = 0 ; i < 12 ; i++)
	{
		teammates[i] = dummy;
		value[i] = 0;
	}
	
	const std::vector< rcsc::AudioMemory::PassRequest >::const_iterator end = wm.audioMemory().passRequest().end();
	for( std::vector< rcsc::AudioMemory::PassRequest >::const_iterator tm = wm.audioMemory().passRequest().begin(); tm != end; tm++ )
    {	
		target_point = tm->pos_;
		unum = tm->sender_;
		
		double dist = ball_pos.dist( target_point );
		
		if( dist > max_dp_dist )
			continue;
		if( target_point.x < -29 && target_point.absY() < 18 )
			continue;
		if( target_point.absX() > 48 || target_point.absY() > 32 )
			continue;
		if( dist < 2.0 )
			continue;
		rcsc::AngleDeg deg = ( ball_pos - target_point ).th();
		if( exist_opponent3( agent , target_point ) )
			continue;
		if( target_point.x < ball_pos.x )
			continue;
		//double distance = 100.0;
		//wm.getOpponentNearestTo( target_point , 20 , &distance );
		//value[ unum ] += 10.0 * std::max( 0.0, 30.0 - distance );
		/*
		rcsc::AngleDeg deg = ( my_pos - tm_pos ).th();
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
		if( wm.existOpponentIn( pass , 10 , true ) )
			continue;
		*/
		//if( tm_pos.x > offside + 0.5 )
		//	value[ unum ] -= 300;
		//value[ unum ] += 10.0 * std::max( 5.0, std::fabs( target_point.y - ball_pos.y ) );
		if( target_point.x > ball_pos.x + 25.0 )
			value[ unum ] += 300;
		else
		if( target_point.x > ball_pos.x + 15.0 )
			value[ unum ] += 200;
		else
		if( target_point.x > ball_pos.x + 10.0 )
			value[ unum ] += 100;
		else
		if( target_point.x > ball_pos.x )
			value[ unum ] += 50;
		else
			value[ unum ] += 0;
		deg = ( ball_pos - target_point ).th();
		//int count = 0;
		//wm.dirRangeCount( deg , 20.0, &count, NULL, NULL );
		//value[ unum ] +=  10.0 * count;
		rcsc::Sector2D front( target_point , 0.0 , 10.0 , -45 , 45 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
			value[ unum ] += 300;
			
		teammates[ unum ] = target_point;
		
		if( teammates[ unum ].x - wm.self().pos().x > 20.0 )
			value[ unum ] += 400;
		else
		if( teammates[ unum ].x - wm.self().pos().x > 15.0 )
			value[ unum ] += 300;
		else
		if( teammates[ unum ].x - wm.self().pos().x > 10.0 )
			value[ unum ] += 200;
		else
		if( target_point.x - wm.self().pos().x > 5.0 )
			value[ unum ] += 100;
		else
			value[ unum ] += 0;
		
		rcsc::Circle2D round( teammates[ unum ] , 2.0 );
		if( wm.existOpponentIn( round , 10 , true ) )
			value[ unum ] -= 100;
		
		if( teammates[ unum ].x > offside )
			value[ unum ] += 200;
	
		if( teammates[ unum ].absY() > 26 )
			value[ unum ] += 300;
		else
		if( teammates[ unum ].absY() > 18 )
			value[ unum ] += 200;
		else
			value[ unum ] += 100;
		
		int tm_reach = wm.interceptTable()->teammateReachCycle();
		int opp_reach = wm.interceptTable()->opponentReachCycle();
		
		if( tm_reach < opp_reach )
			value[ unum ] -= 500;
		
		found = true;
	}
	
	if( ! found )
		return false;
	
	rcsc::Vector2D tmp;
	double vtmp;
	for(int i = 0 ; i < 11 ; i++)
		for(int j = i + 1 ; j < 12 ; j++)
			if( value[ i ] < value[ j ] )
			{
				tmp = teammates[ i ];
				vtmp = value[ i ];
				teammates[ i ] = teammates[ j ];
				value[ i ] = value[ j ];
				teammates[ j ] = tmp;
				value[ j ] = vtmp;
			}

	
	//for( int i = 0 ; i < 6 ; i++ )
	//	std::cout<<"["<<wm.time().cycle()<<"] => "<<value[i]<<std::endl;
	//std::cout<<"\n";
	
	double first_speed1 = first_speed( agent , teammates[ 0 ] , 'l' );
	double dist_to_point = 1000.0;
	const rcsc::PlayerObject * receiver_tm = wm.getTeammateNearestTo( teammates[ 0 ] , 10 , &dist_to_point );
	if( (! receiver_tm) )
		return false;
	if( receiver_tm->goalie() || receiver_tm->unum() > 11 || receiver_tm->unum() < 2 )
		return false;
	int receiver = receiver_tm->unum();
	
    
    if (! Body_SmartKick( teammates[ 0 ] , first_speed1 , first_speed1 * 0.96 , 3 ).execute( agent ) )
		Body_KickOneStep( teammates[ 0 ] , first_speed1 , true ).execute( agent );
		
    if ( agent->config().useCommunication()
         && receiver != rcsc::Unum_Unknown )
    {
		say_pass( agent , receiver , teammates[ 0 ] );
    }
    
	return false;
}

/*--------------------------------------------------------------*/

bool Body_Pass::runRequest( rcsc::PlayerAgent * agent , const int runner , rcsc::Vector2D pass_point )
{	
    if ( agent->config().useCommunication() )
    {
		//agent->addSayMessage( new rcsc::RunRequestMessage( runner , pass_point ) );
		return true;
    }
    
	return false;
}

/*--------------------------------------------------------------*/

bool Body_Pass::requestedRun( rcsc::PlayerAgent * agent )
{
	
	if( agent->world().audioMemory().runRequest().empty() )
		return false;

	if( agent->world().audioMemory().runRequestTime().cycle() <= agent->world().time().cycle() - 5 ) 
		return false;
		
    const rcsc::WorldModel & wm = agent->world();
    
    int unum = wm.audioMemory().runRequest().front().runner_;
	if( wm.self().unum() != unum )
		return false;
		
	rcsc::Vector2D target_point = wm.audioMemory().runRequest().front().pos_;
	rcsc::Vector2D my_pos = wm.self().pos();
	
	if( my_pos.dist( target_point ) < 1.0 )
		return true;
	
	double dist = my_pos.dist( target_point ) * 0.1;
	if( dist < 0.7 )
		dist = 0.7;
		
	if( ! Body_GoToPoint( target_point , 0.5 , rcsc::ServerParam::i().maxDashPower() ).execute( agent ) )
	{
		if( wm.self().body().abs() > 90.0 )
			rcsc::Body_TurnToAngle( 0.0 ).execute( agent );	
		agent->setNeckAction( new rcsc::Neck_TurnToBall() );
		return true;
	}
	
	agent->setNeckAction( new rcsc::Neck_TurnToBall() );
	return false;

}

/*----------------------------------------------*/
double 
Body_Pass::theta( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point )
{
	double theta = 10.0;
	return theta;
}
	
double
Body_Pass::first_speed( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point , char type )
{
	double es = end_speed( agent , pass_point , type );
	//double es = 1.5;
    double first_speed1 = 4.0;
    do
    {
        first_speed1 = rcsc::calc_first_term_geom_series_last( es, agent->world().ball().pos().dist( pass_point ), rcsc::ServerParam::i().ballDecay() );
        if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        es -= 0.05;
    }
    while ( es > 0.8 );

	first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
	//first_speed1 *= rcsc::ServerParam::i().ballDecay();

	return first_speed1;
}
	
bool
Body_Pass::exist_opponent( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point )
{
	//return (! can_pass( agent , pass_point ) );
	
	rcsc::Vector2D ball = agent->world().ball().pos();
	rcsc::Vector2D self = agent->world().self().pos();
	rcsc::AngleDeg theta = ( ball - pass_point ).th();
	double dist = ball.dist( pass_point );
	double deg = 0.5 * dist + 2.5;
	if( dist < 15 )
		deg = 10;
	if( dist > 25 )
		deg = 15;
	//rcsc::Sector2D pass( ball , 0.0 , ball.dist( pass_point ) , theta - deg , theta + deg );
	rcsc::Sector2D pass( ball , 0.0 , ball.dist( pass_point ) , theta - 10 , theta + 10 );
	
	//rcsc::Triangle2D pass(  ball,
	//						pass_point + rcsc::Vector2D::polar2vector( ball.dist( self ) * std::tan( deg ) , theta + 90 ) , 
	//						pass_point - rcsc::Vector2D::polar2vector( ball.dist( self ) * std::tan( deg ) , theta + 90 ) );
	
	if(! agent->world().existOpponentIn( pass, 10 , false ) )
		return false;

	return true;
}

bool
Body_Pass::say_pass( rcsc::PlayerAgent * agent , int unum , rcsc::Vector2D pass_point )
{

	rcsc::Vector2D buff = agent->world().ball().pos() - pass_point;
	buff.setLength( 1.0 );
	
    rcsc::Vector2D ball_vel( 0.0, 0.0 );
    if ( ! agent->effector().queuedNextBallKickable() )
		ball_vel = agent->effector().queuedNextBallVel();

	agent->addSayMessage( new rcsc::PassMessage( unum,
												 buff + pass_point,
												 agent->effector().queuedNextBallPos(),
												 ball_vel ) );
	return true;
}

double
Body_Pass::end_speed( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point , char type )
{
	double distance = pass_point.dist( agent->world().ball().pos() );
	if( type == 'd' )
	{		
		if ( distance >= 20.0 )
	    {
	        //return 3.0;
	        return 2.5;
	    }
	    else if ( distance >= 12.0 )
	    {
	        //return 2.5;
	        return 2.0;
	    }
	    else if ( distance >= 7.0 )
	    {
	        return 1.8;
	    }
	    else
	    {
	        return 1.4;
	    }
	}
	else
	if( type == 'l' )
	{		
		if ( distance >= 20.0 )
	    {
	        //return 3.0;
	        return 1.6;
	    }
	    else if ( distance >= 8.0 )
	    {
	        //return 2.5;
	        return 1.1;
	    }
	    else if ( distance >= 5.0 )
	    {
	        return 0.9;
	    }
	    else
	    {
	        return 0.7;
	    }
	}
	else
	{		
		if ( distance >= 20.0 )
	    {
	        //return 3.0;
	        //return 1.3;
	        return 0.8;
	    }
	    else if ( distance >= 8.0 )
	    {
	        //return 2.5;
	        //return 1.1;
	        return 0.8;
	    }
	    else if ( distance >= 5.0 )
	    {
	        //return 0.9;
	        return 0.8;
	    }
	    else
	    {
	        //return 0.7;
	        return 0.8;
	    }
	}
	return 10.0;
}


bool
Body_Pass::exist_opponent2( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point )
{

	/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return false;

	
}


bool
Body_Pass::can_pass( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point )
{
		/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return false;
	
}

/*--------------------------------------------------------*/
bool
Body_Pass::exist_opponent3( rcsc::PlayerAgent * agent , rcsc::Vector2D target_point , bool through )
{
	
	
	//return ( exist_opponent2( agent , target_point ) );
	return ( exist_opponent( agent , target_point ) );

	//if( !through )
	//	return (! can_pass( agent , target_point ) );
	
		/*
	 * 
	 *  CODES ARE REMOVED
	 * 
	 * 
	*/
	
	return false;

}

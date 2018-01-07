// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Samira KARIMZADEH
 Copyright (C) Elham IRAVANI
 Modified By Mahdi SADEGHI

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

#include "body_pass.h"
#include "bhv_offensive_planner.h"
#include "strategy.h"
#include "sample_player.h"
#include "body_smart_kick.h"
#include "body_kick_multi_step.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/say_message_builder.h>

#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>


/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::execute( rcsc::PlayerAgent * agent , rcsc::Vector2D target_point , int receiver , double first_speed )
{
	
	//if ( ! SamplePlayer::instance().off_planner() )
	if ( ! Strategy::i().off_planner() )
    {
		return false;
    }
    
	const rcsc::WorldModel & wm = agent->world();
	
    if ( ! wm.self().isKickable() )
        return false;
//execute
        
    //Body_KickMultiStep( target_point, first_speed, false ).execute( agent );
	
//    if ( agent->world().gameMode().type() != GameMode::PlayOn )
//        agent->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );

    if ( ! Body_SmartKick( target_point, first_speed, first_speed * 0.96, 3 ).execute( agent ) )
    {
		return false;
    }
    
    if ( agent->config().useCommunication()
         && receiver != rcsc::Unum_Unknown )
    {
		Body_Pass::say_pass( agent , receiver , target_point );
    }
    std::cout<<"execute"<<std::endl;
	return true;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::createShootLine( rcsc::PlayerAgent * agent )
{
	
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D opGoalie_pos = (wm.getOpponentGoalie())->pos();
    rcsc::Vector2D kick_point (52.5,-6.0);
    rcsc::Vector2D left_point(52.5 , -7);
    rcsc::Vector2D right_point(52.5 , 7);
    rcsc::Vector2D target_point(52.5,0.0);
    double step = 0.5;
    double max_score = 0;

    while( kick_point.y <= 6 )
    {
		if( (! wm.existOpponentIn( rcsc::Triangle2D(left_point , my_pos , right_point),10,1))
		    && (opGoalie_pos.dist(kick_point) > rcsc::ServerParam::i().catchableArea()+0.5) )
		{
		    rcsc::Line2D shootLine (my_pos , kick_point);
		    double score = opGoalie_pos.dist(kick_point)*shootLine.dist(opGoalie_pos);
		    if( score > max_score)
		    {
	    		 max_score = score;
	    	     target_point = kick_point;
	    	}
		}
		kick_point.y += step;
    }

    if(max_score > 0)
    {
		//Body_KickOneStep( target_point,ServerParam::i().ballSpeedMax()).execute( agent );
		return true;
    }
    
	return false;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::keepShootChance( rcsc::PlayerAgent * agent )
{
    //std::cout<<agent->world().self().unum()<<"keepShootChance\n";
    const rcsc::WorldModel & wm = agent->world();
    std::vector<int> pass;
    std::vector<int> shoot;
    std::vector<double>shootable_x;
    
    const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
    for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
    {
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
		    continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
		    continue;
		if( (*tm)->pos().x < 16 )
		    continue;
		
		rcsc::Vector2D tm_pos = (*tm)->pos();
		if(isShootLineExist2(agent , tm_pos , true))
		{
		    shoot.push_back((*tm)->unum());
		    shootable_x.push_back((*tm)->pos().x);
		}
    }
    if(shoot.empty())
		return false;
    
    for(unsigned int i = 0 ; i < shootable_x.size() - 1 ; i++)
		for(unsigned int j = i+1 ; j < shootable_x.size() ; j++)
			if(shootable_x [j] > shootable_x[i] )
		    {
				double tmp_x = shootable_x[i];
				shootable_x[i] = shootable_x[j];
				shootable_x[j] = tmp_x;
				int tmp_unum = shoot[i];
				shoot[i] = shoot[j];
				shoot[j] = tmp_unum;
		    }
    std::vector<int>::const_iterator end = shoot.end();
    for( std::vector<int>::const_iterator it = shoot.begin(); it!= end ; ++it)
    {
		//std::cout<<"player "<<*it <<"is shootable\n";
	        //clear pass
	    pass.clear();
	
		std::cout<<"find pass node"<<std::endl;
		if(findPassNode( agent , agent->world().self().unum() , (*it) , pass) )
			//std::cout<<"find pass node succeed"<<std::endl;
		    
		    if( findPassPos(agent , pass[pass.size()-1]) )
		    {
				std::cout<<"keep shoot chance succeed\n";
				agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
				return true;
		    }
		    
		//{ pass.push_back(/*tm_unum*/0);
		//execute pass :D
		//return true }
    }
    
    return false;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::isShootable( rcsc::PlayerAgent * agent , rcsc::Vector2D tm_pos )
{
	return false;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::isShootLineExist( rcsc::PlayerAgent * agent , rcsc::Vector2D & target_point )
{
	
    //std::cout<<"isShootLineExist1\n";
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D check_point(52.5,-5.5);//badan az server begirim
    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D opGoalie_pos(0,0);
    double safe_dif = 1.0;
    
    if( my_pos.x > 42)
		safe_dif = 0.8;
    else
	if( my_pos.x > 36 )
		safe_dif = 3.0;
    else
		return false;
    
    //set start point
    
    opGoalie_pos = (wm.getOpponentGoalie())->pos();

    int op_count;
    double step = 0.5;
    if( opGoalie_pos.absY() <= 1.5 )//start point-->tiri ke be agent nazdiktare
		if( my_pos.y > 0 )
		{
		    check_point.y = 5.5;
		    step = - 0.5;
		}
		else
		{
		    check_point.y = -5.5;
		    step = + 0.5;
		
		}
    else//start point-->tiri ke az opGoalie doortare
	if( opGoalie_pos.y < 0 )
	{
	    check_point.y = 5.5;
	    step = - 0.5;
	}
	else
	{
	    check_point.y = -5.5;
	    step = + 0.5;
	
	}
    
    while( (check_point.y < 5.5) && (check_point.y > -5.5) )
    {
		target_point.y = -6;
		target_point.x = 52.5;
		op_count = 0;
		
		rcsc::Line2D shoot_line(my_pos, check_point );
		
		const rcsc::PlayerPtrCont opps = wm.opponentsFromSelf();
		const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
		for ( rcsc::PlayerPtrCont::const_iterator itop = opps.begin(); itop != opps_end; ++itop )
		{
		    if(! *itop )
				continue;
		    if( (! (*itop)) || (*itop)->isGhost() || (*itop)->posCount() > 3  )
	            continue;
		    rcsc::Vector2D op_pos = (*itop)->pos();
		    if( op_pos.x < my_pos.x )
				continue;
		    if( shoot_line.dist(op_pos) <= safe_dif )
				if( ( my_pos.dist(op_pos) < 3 ) && ( ++op_count > 1 ) )
					break;
		    target_point = check_point;
		}
		if(target_point == check_point)
		{
		    //Body_KickOneStep( target_point,ServerParam::i().ballSpeedMax()).execute( agent );
		    Body_SmartKick( target_point , rcsc::ServerParam::i().ballSpeedMax() , rcsc::ServerParam::i().ballSpeedMax() * 0.96 , 3 ).execute( agent );
		    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		    std::cout<<"is shoot line exist succeed"<<std::endl;
		    return true;
		}
		check_point.y += step;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::isShootLineExist2( rcsc::PlayerAgent * agent , rcsc::Vector2D target_point , bool checkShootable )
{
	
    //std::cout<<agent->world().self().unum()<<" isShootLineExist2\n";
    const rcsc::WorldModel & wm = agent->world();
    
    //cout<<" line 3\n";
    double safe_ang = 0;
    rcsc::Vector2D check_point(52.5,-5.5);//badan az server begirim
    
    //cout<<"line 5\n";
    rcsc::Vector2D my_pos; 
    my_pos = (!checkShootable) ?(wm.self().pos()) : target_point;
	    
    //cout<<"line 6\n";
    //rcsc::Vector2D opGoalie_pos(52,0);
    //opGoalie_pos = (wm.getOpponentGoalie())->pos();
    const rcsc::PlayerObject *  op_goalie = wm.getOpponentGoalie();
    if(! op_goalie || op_goalie->isGhost() || op_goalie->posCount() > 3)
		return false;
    rcsc::Vector2D opGoalie_pos = op_goalie->pos();
    
    if(my_pos.x < 30)
		return false;
	
    if(my_pos.x < 35)
		safe_ang = 10;
 //   else
	//safe_ang = 15;
//	{
	    
//	}

    //cout<<"line 7\n";
    std::vector<rcsc::Vector2D>shoot_point;
    std::vector<rcsc::AngleDeg>shoot_ang;
    std::vector<double> value;
    
        
    //cout<<"before for\n";
    
    for(double step = 0.5 ;check_point. y < 5.5 ; check_point.y += step)
    {
		if(safe_ang != 10)
		{
		    safe_ang = fabs((std::atan(fabs((my_pos.y - (check_point.y - 1.5))) / (- my_pos.x + 52.5)) * 180 / 3.14) -
				(std::atan(fabs((my_pos.y - (check_point.y + 1.5))) / (- my_pos.x + 52.5)) * 180 / 3.14));
		    if(my_pos.absY() > 13)
				safe_ang *=(my_pos.absY()/5 );
		}
		//std::cout<<"safe_ang = "<<safe_ang<<'\n';
		if( ( wm.ball().pos().dist(check_point) / wm.ball().vel().r() ) >= 
		    ( op_goalie->playerTypePtr()->cyclesToReachDistance(opGoalie_pos.dist(check_point))))
		    continue;
	
		//if( wm.self().playerType().cyclesToReachDistance(my_pos.dist(check_point)) > 
		  //  op_goalie->playerTypePtr()->cyclesToReachDistance(opGoalie_pos.dist(check_point)))
		 /*if( check_point == wm.ball().inertiaPoint(op_goalie->playerTypePtr()->cyclesToReachDistance(opGoalie_pos.dist(check_point))))
		{
		    //cout<<"cycles to reach\n";
		    continue;
		}  */
		rcsc::Sector2D safe_area( my_pos , 0.5 , my_pos.dist( check_point ) , -safe_ang , safe_ang  );
		if( wm.existOpponentIn( safe_area , 11 , true ) )
		    continue;
		if(opGoalie_pos.dist(check_point) < rcsc::ServerParam::i().catchableArea() * 1.1 )
		    continue;
		
		value.push_back( rcsc::Line2D(my_pos,check_point).dist(opGoalie_pos) * opGoalie_pos.dist(check_point));
		//shoot_ang.push_baack(teta);
		shoot_point.push_back(check_point);
    }
    
    if(value.empty())
		return false;
    
    //find widest angle
    //cout<<"after is empty\n";
    for(unsigned int i = 0 ; i < value.size() - 1 ; i++)
		for(unsigned int j = i+1 ; j < value.size() ; j++)
		    if( value[i] < value[j] )
		    //if(shoot_ang[i] < shoot_ang[j])
		    {
				double tmp_value = value[i];
			    value[i] = value[j];
			    value[j] = tmp_value;
			    rcsc::Vector2D tmp_point = shoot_point[i];
			    shoot_point[i] = shoot_point[j];
			    shoot_point[j] = tmp_point;
			}
    //std::cout<<"target point in exist shoot line 2 = "<<"x = "<<shoot_point[0].x<<" y = "<<shoot_point[0].y<<'\n';
    if(! checkShootable)
    {
		target_point = shoot_point[0];//has widest angle or value
		//Body_KickOneStep( target_point,ServerParam::i().ballSpeedMax()).execute( agent );
		Body_SmartKick( target_point , rcsc::ServerParam::i().ballSpeedMax() , rcsc::ServerParam::i().ballSpeedMax() * 0.96 , 3 ).execute( agent );
		std::cout<<"is shoot line exist2 succeed"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
		return true;
    }
    
    return false;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::findPassNode( rcsc::PlayerAgent * agent , int src_unum , int dest_unum , std::vector<int> & pass )
{
	std::cout<<"find recursive"<<std::endl;
    const rcsc::WorldModel & wm = agent->world();
    int tm_unum, agent_unum = agent->world().self().unum();
    
    if(dest_unum == agent_unum)
		return true;
    if(exist(dest_unum , pass))
		return false;
    pass.push_back(dest_unum);

    const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
    for( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
    {
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
		    continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
		    continue;
		if( (*tm)->pos().x < 16 )
		    continue;
	    tm_unum = (*tm)->unum();
		if(tm_unum == dest_unum )
			continue;
		if( existPassLine(agent,dest_unum,tm_unum) )//?????
		{
		    std::cout<<agent->world().time().cycle()<<" exist pass line succeed!!!"<<dest_unum<<" <-- "<<tm_unum<<'\n';
		    if( findPassNode(agent , dest_unum , tm_unum , pass) )
				return true;
		    //else
		//	pass.erase(pass.end()-1);
		}
    }
    pass.erase(pass.end()-1);
    return false;
}
/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_OffensivePlanner::exist( int value , std::vector<int> & a )
{
	std::vector<int>::const_iterator end = a.end();
    for(std::vector<int>::const_iterator it = a.begin(); it != end ; it++)
    {
		if( !(*it) )
			continue;
		if(*it == value)
		    return true;
	}
    return false;
}

/*--------------------------------------------------------------*/

bool Bhv_OffensivePlanner::simplePass( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
	
    double offside = wm.offsideLineX();
    double ball_d = rcsc::ServerParam::i().ballDecay();
    double max_ball_s = rcsc::ServerParam::i().ballSpeedMax();
    double max_dp_dist = rcsc::inertia_final_distance( max_ball_s,ball_d );
    std::vector< rcsc::Vector2D > teammates(12);
    std::vector< double > scores(12);
    bool found = false;
    
    for(int i=0 ; i<12 ; i++)
		scores[i] = 0;
    rcsc::Vector2D my_pos = wm.self().pos();
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
		double dist = my_pos.dist( tm_pos );
		int unum =(*tm)->unum();
		if(unum > 11 || unum < 2)
		    continue;
		teammates[ unum ] = tm_pos;
		scores[unum] = 0;
		
		if( tm_pos.x < my_pos.x - 10.0 )
		    continue;
		if( dist > max_dp_dist )
		    continue;
		if( tm_pos.absY() > 34 )
		    continue;
		if( tm_pos.x > offside - 1.0 )
		    continue;
		
		rcsc::Circle2D round( tm_pos , 3.0 );
		if( wm.existOpponentIn( round , 10 , true ) )
		    continue;
		for(int i = 4 ; i < 10 ; i++)
		{
		    rcsc::Circle2D safeCircle(tm_pos , i);
		    if(! wm.existOpponentIn(safeCircle , 10 , true))
				scores[unum] += 100;
		}
		scores[unum] += tm_pos.x;
		
		if(my_pos.absY() > 17 && tm_pos.absY() < 17)
			scores[unum] += 100;
			
		rcsc::Sector2D front( tm_pos , 0.5 , 5.0 , -30 , 30 );
		if( ! wm.existOpponentIn( front , 10 , true ) )
		    scores[ unum ] += 100;
			
		found = true;

    }
    if(! found)
		return false;
	
	rcsc::Vector2D tmp;
	int tmp_score;
	for(int i = 0 ; i < 11 ; i++)
	    for(int j = i + 1 ; j < 12 ; j++)
		if( scores[ i ] < scores[ j ] )
		{
		    tmp = teammates[ i ];
		    teammates[ i ] = teammates[ j ];
		    teammates[ j ] = tmp;
		    tmp_score = scores[i];
		    scores[i] = scores[j];
		    scores[j] = tmp_score;
		}
	
    for( int i = 0 ; i < 12 ; i ++)
    {
		double dist = 1000.0;
		const rcsc::PlayerObject * tm = wm.getTeammateNearestTo( teammates[i] , 5 , &dist );
		if( (! tm) || tm->isGhost() || tm->posCount() > 3  )
		    continue;
		if( tm->goalie() || tm->unum() > 11 || tm->unum() < 2 )
		    continue;
			
		rcsc::Vector2D tm_pos = tm->pos();
		double end_speed = 1.5;
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
		first_speed1 *= rcsc::ServerParam::i().ballDecay();
			
		rcsc::AngleDeg deg = ( tm_pos - my_pos ).th();
		dist = my_pos.dist( tm_pos );
		rcsc::Sector2D pass( my_pos , 0.5 , dist , deg - 10 , deg + 10 );
		if(! wm.existOpponentIn( pass, 10 , false ) )
		{
			//std::cout<<"simple pass"<<std::endl;
		    return execute( agent , tm_pos , tm->unum() , first_speed1 );
		}
    }
	
    return false;

}
/*--------------------------------------------------------------*/

bool Bhv_OffensivePlanner::existPassLine(rcsc::PlayerAgent * agent , int src_unum , int dest_unum)
{
    
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D dest_pos,src_pos;
    bool find_src = false , find_dest = false;// is true when src_pos and dest_pos are fined
    double dist = src_pos.dist( dest_pos );
    double max_dp_dist = 30;
	
    const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
    for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
    {
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->unum() == src_unum )
		{
		    src_pos = (*tm)->pos();
		    find_src =true;
		}
		if( (*tm)->unum() == dest_unum )
		{
		    dest_pos = (*tm)->pos();
		    find_dest = true;
		}
    }	
    
    if(! find_src || ! find_dest)
		return false;
			
    if( dest_pos.x < 20 )
		return false;
    if( dist > max_dp_dist )
		return false;
    if( dest_pos.absY() > 34 )
		return false;
		
    rcsc::AngleDeg deg = ( src_pos - dest_pos ).th();
    rcsc::Sector2D pass( src_pos , 0.5 , dist , deg - 12.5 , deg + 12.5 );
    if( wm.existOpponentIn( pass , 10 , true ) )
		return false;

    rcsc::Circle2D round( dest_pos , 2.0 );
    if( wm.existOpponentIn( round , 10 , true ) )
		return false;//value[ unum ] -= 200;
		
    rcsc::Sector2D front( dest_pos , 0.5 , 5.0 , -30 , 30 );
    if( wm.existOpponentIn( front , 10 , true ) )
		return false;
    
    return true;
    //found = true;

}

/*--------------------------------------------------*/

bool Bhv_OffensivePlanner::findPassPos(rcsc::PlayerAgent * agent , int dest_unum)
{
	std::cout<<"findpasspos"<<std::endl;
    const rcsc::WorldModel & wm = agent->world();
    
	if ( ! wm.self().isKickable() )
        return false;
    
    rcsc::Vector2D dest_pos;
    rcsc::Vector2D my_pos = wm.self().pos();
    bool find_dest = false;// is true when src_pos and dest_pos are found
	
    const rcsc::PlayerPtrCont & team = wm.teammatesFromSelf();
    const rcsc::PlayerPtrCont::const_iterator team_end = team.end();
    for ( rcsc::PlayerPtrCont::const_iterator  tm = team.begin(); tm != team_end; tm++ )
    {
		if( (! (*tm)) || (*tm)->isGhost() || (*tm)->posCount() > 3  )
			continue;
		if( (*tm)->goalie() || (*tm)->unum() > 11 || (*tm)->unum() < 2 )
			continue;
		if( (*tm)->unum() == dest_unum )
		{
		    dest_pos = (*tm)->pos();
		    find_dest = true;
		}
    }	
    
    if(! find_dest)
		return false;

    rcsc::Vector2D target_point(50.0, 0.0);
    double first_speed = 0.0;
    int receiver = 0;

    double end_speed = 1.5;
    double first_speed1 = 100.0;
    do
    {
		first_speed1 = rcsc::calc_first_term_geom_series_last( end_speed, my_pos.dist( dest_pos ) , rcsc::ServerParam::i().ballDecay() );
		if ( first_speed1 < rcsc::ServerParam::i().ballSpeedMax() )
		{
		    break;
		}
		end_speed -= 0.1;
    }
    while ( end_speed > 0.8 );
	
    first_speed1 = std::min( rcsc::ServerParam::i().ballSpeedMax() , first_speed1 );
    first_speed1 *= rcsc::ServerParam::i().ballDecay();
	target_point = dest_pos;
	receiver = dest_unum;
	first_speed = first_speed1;
    return execute( agent , target_point , receiver , first_speed );

}

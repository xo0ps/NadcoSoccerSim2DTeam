// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "shoot_table.h"
#include "body_shoot.h"
#include "body_smart_kick.h"
#include "body_kick_one_step.h"

#include <rcsc/common/server_param.h>
#include <rcsc/player/player_agent.h>

#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_turn_to_angle.h>

ShootTable Body_Shoot::S_shoot_table;
/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Shoot::execute( rcsc::PlayerAgent * agent )
{
    if ( ! agent->world().self().isKickable() )
    {
        return false;
    }

#if 0

    const ShootGenerator::Container & cont = ShootGenerator::instance().courses( agent->world() );

    // update
    if ( cont.empty() )
    {
        return false;
    }
	ShootGenerator::Container::const_iterator best_shoot
        = std::min_element( cont.begin(), cont.end(),ShootGenerator::ScoreCmp() );

    if ( best_shoot == cont.end() )
    {
        return false;
    }
    // it is necessary to evaluate shoot courses

	/*
	static rcsc::GameTime lastTime = agent->world().time();
	if( lastTime.cycle() >= agent->world().time().cycle() - 1 )
	{
		rcsc::AngleDeg deg = ( agent->world().ball().pos().y > 0 ) ? -120 : 120;
		rcsc::Body_TurnToAngle( deg ).execute( agent );
	}
	else
		lastTime = agent->world().time();
	*/
	
    rcsc::Vector2D target_point = best_shoot->target_point_;
    double first_speed = best_shoot->first_ball_speed_;

    rcsc::Vector2D one_step_vel
        = rcsc::KickTable::calc_max_velocity( ( target_point - agent->world().ball().pos() ).th(),
                                        agent->world().self().kickRate(),
                                        agent->world().ball().vel() );
    double one_step_speed = one_step_vel.r();

    if ( one_step_speed > first_speed * 0.99 )
    {
        Body_SmartKick( target_point,
                        one_step_speed,
                        one_step_speed * 0.99 - 0.0001,
                        1 ).execute( agent );
		std::cout<<"shoot"<<std::endl;
        //agent->setNeckAction( new rcsc::Neck_TurnToGoalieOrScan( -1 ) );
        agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
        return true;
    }

    if( Body_SmartKick( target_point, one_step_speed, one_step_speed * 0.99 , 3 ).execute( agent ) )
    {
		std::cout<<"shoot"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
        return true;
	}
	/*
    if ( agent->world().existKickableOpponent() )
    {
        if ( Body_KickOneStep( target_point , rcsc::ServerParam::i().ballSpeedMax() ).execute( agent ) )
        {
			std::cout<<"shoot"<<std::endl;
			agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
	        return true;
        }
    }
    */

#else

    const ShootTable::ShotCont & shots = S_shoot_table.getShots( agent );

    // update
    if ( shots.empty() )
    {
        return false;
    }

    ShootTable::ShotCont::const_iterator shot
        = std::min_element( shots.begin(), shots.end(),ShootTable::ScoreCmp() );

    if ( shot == shots.end() )
    {
        return false;
    }

    // it is necessary to evaluate shoot courses

    rcsc::Vector2D target_point = shot->point_;
    double first_speed = shot->speed_;
    
    rcsc::Vector2D one_step_vel
        = rcsc::KickTable::calc_max_velocity( ( target_point - agent->world().ball().pos() ).th(),
                                        agent->world().self().kickRate(),
                                        agent->world().ball().vel() );
    double one_step_speed = one_step_vel.r();

    if ( one_step_speed > first_speed * 0.99 )
    {
        Body_SmartKick( target_point,
                        one_step_speed,
                        one_step_speed * 0.99 - 0.0001,
                        1 ).execute( agent );
        std::cout<<"shoot"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
        return true;
    }

    if( Body_SmartKick( target_point, first_speed , first_speed * 0.99 , 3 ).execute( agent ) )
    {
		std::cout<<"shoot"<<std::endl;
		agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
        return true;
	}

    if ( agent->world().existKickableOpponent() )
    {
        if ( Body_KickOneStep( target_point , rcsc::ServerParam::i().ballSpeedMax() ).execute( agent ) )
        {
			std::cout<<"shoot"<<std::endl;
			agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
	        return true;
        }
    }

#endif
    
    agent->setNeckAction( new rcsc::Neck_TurnToPoint( target_point ) );
    return true;
}

// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
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

#include "role_center_back.h"

#include "strategy.h"

#include "bhv_basic_offensive_kick.h"
#include "bhv_basic_move.h"
#include "bhv_offmid_defmid_kick.h"
#include "bhv_offmid_defmid_move.h"
#include "bhv_dribble_kick.h"
#include "bhv_dribble_move.h"
#include "bhv_cross_kick.h"
#include "bhv_cross_move.h"
#include "bhv_danger_kick.h"
#include "bhv_danger_move.h"
#include "bhv_shootchance_kick.h"
#include "bhv_shootchance_move.h"

#include <rcsc/formation/formation.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/server_param.h>


const std::string RoleCenterBack::NAME( "CenterBack" );

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg( &RoleCenterBack::create,
                                                       RoleCenterBack::NAME );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleCenterBack::execute( rcsc::PlayerAgent * agent )
{
    bool kickable = agent->world().self().isKickable();
    if ( agent->world().existKickableTeammate()
         && agent->world().teammatesFromBall().front()->distFromBall()
         < agent->world().ball().distFromSelf() )
    {
        kickable = false;
    }

    if ( kickable )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleCenterBack::doKick( rcsc::PlayerAgent * agent )
{

    switch ( Strategy::get_ball_area( agent->world().ball().pos() ) ) {
	case Strategy::BA_CrossBlock:
    case Strategy::BA_Cross:
		Bhv_CrossKick().execute( agent );
		break;
    case Strategy::BA_Stopper:
    case Strategy::BA_Danger:
		Bhv_DangerKick().execute( agent );
		break;
    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DribbleAttack:
		Bhv_DribbleKick().execute( agent );
		break;
    case Strategy::BA_DefMidField:
    case Strategy::BA_OffMidField:
		Bhv_OffmidDefmidKick().execute( agent );
		break;
    case Strategy::BA_ShootChance:
		Bhv_ShootChanceKick().execute( agent );
		break;
    default:
		Bhv_BasicOffensiveKick().execute( agent );
        break;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
RoleCenterBack::doMove( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D home_pos = Strategy::i().getPosition( agent->world().self().unum() );
    if ( rcsc::ServerParam::i().useOffside() )
    {
        home_pos.x = std::min( home_pos.x, wm.offsideLineX() - 1.0 );
    }

	if ( ! home_pos.isValid() ) home_pos = wm.self().pos();
        
    switch ( Strategy::get_ball_area( agent->world() ) ) {
    case Strategy::BA_CrossBlock:
    case Strategy::BA_Cross:
		Bhv_CrossMove( home_pos ).execute( agent );
		break;
    case Strategy::BA_Stopper:
    case Strategy::BA_Danger:
		Bhv_DangerMove( home_pos ).execute( agent );
		break;
    case Strategy::BA_DribbleBlock:
    case Strategy::BA_DribbleAttack:
		Bhv_DribbleMove( home_pos ).execute( agent );
		break;
    case Strategy::BA_DefMidField:
    case Strategy::BA_OffMidField:
		Bhv_OffmidDefmidMove( home_pos ).execute( agent );
		break;
    case Strategy::BA_ShootChance:
		Bhv_ShootChanceMove( home_pos ).execute( agent );
		break;
    default:
		Bhv_BasicMove( home_pos ).execute( agent );
        break;
    }
}

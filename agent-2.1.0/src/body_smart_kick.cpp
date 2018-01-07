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

#include "body_smart_kick.h"

#include <rcsc/action/kick_table.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>

#include <algorithm>

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_SmartKick::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();

    if ( ! wm.self().isKickable() )
    {
        return false;
    }

    if ( ! wm.ball().velValid() )
    {
	   return rcsc::Body_StopBall().execute( agent );
    }

    double first_speed = std::min( M_first_speed, sp.ballSpeedMax() );
    double first_speed_thr = std::max( 0.0, M_first_speed_thr );
    int max_step = std::max( 1, M_max_step );

    rcsc::KickTable::instance().createTables();
    if ( rcsc::KickTable::instance().simulate( wm,
                                               M_target_point,
                                               first_speed,
                                               first_speed_thr,
                                               max_step,
                                               M_sequence )
         || M_sequence.speed_ >= first_speed_thr )
    {
        rcsc::Vector2D vel = M_sequence.pos_list_.front() - wm.ball().pos();
        rcsc::Vector2D kick_accel = vel - wm.ball().vel();
        agent->doKick( kick_accel.r() / wm.self().kickRate(),
                       kick_accel.th() - wm.self().body() );
        return true;
    }


    //
    // TODO: force mode
    //

	
    // failed to search the kick sequence

    rcsc::Body_HoldBall2008( false, M_target_point, M_target_point ).execute( agent );
    return false;
}

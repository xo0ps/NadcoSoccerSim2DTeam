// -*-c++-*-

/*!
  \file neck_scan_players.cpp
  \brief scan players only by turn_neck
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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

#include "neck_scan_players.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/view_mode.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/logger.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/angle_deg.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>

#include <algorithm>
#include <limits>
#include <cstdio>

// #define DEBUG_PRINT

namespace rcsc {

//! invalid angle value
const double Neck_ScanPlayers::INVALID_ANGLE = -360.0;


/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_ScanPlayers::execute( PlayerAgent * agent )
{
    static GameTime s_last_calc_time( 0, 0 );
    static ViewWidth s_last_calc_view_width = ViewWidth::NORMAL;
    static double s_last_calc_min_neck_angle = 0.0;
    static double s_last_calc_max_neck_angle = 0.0;
    static double s_cached_target_angle = 0.0;

    if ( s_last_calc_time != agent->world().time()
         || s_last_calc_view_width != agent->effector().queuedNextViewWidth()
         || std::fabs( s_last_calc_min_neck_angle - M_min_neck_angle ) > 1.0e-3
         || std::fabs( s_last_calc_max_neck_angle - M_max_neck_angle ) > 1.0e-3 )
    {
        s_last_calc_time = agent->world().time();
        s_last_calc_view_width = agent->effector().queuedNextViewWidth();
        s_last_calc_min_neck_angle = M_min_neck_angle;
        s_last_calc_max_neck_angle = M_max_neck_angle;

        s_cached_target_angle = get_best_angle( agent,
                                                M_min_neck_angle,
                                                M_max_neck_angle );
    }

    if ( s_cached_target_angle == INVALID_ANGLE )
    {
        return Neck_ScanField().execute( agent );
    }

    AngleDeg target_angle = s_cached_target_angle;

    agent->debugClient().addMessage( "NeckScanPl" );

    agent->doTurnNeck( target_angle
                       - agent->effector().queuedNextSelfBody()
                       - agent->world().self().neck() );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
double
Neck_ScanPlayers::get_best_angle( const PlayerAgent * agent,
                                  const double & min_neck_angle,
                                  const double & max_neck_angle )
{
    const WorldModel & wm = agent->world();

    if ( wm.allPlayers().size() < 22 )
    {
        return INVALID_ANGLE;
    }

    const ServerParam & SP = ServerParam::i();

    const Vector2D next_self_pos = agent->effector().queuedNextSelfPos();
    const AngleDeg next_self_body = agent->effector().queuedNextSelfBody();
    const double view_width = agent->effector().queuedNextViewWidth().width();
    const double view_half_width = view_width * 0.5;
    const double neck_min = ( min_neck_angle == INVALID_ANGLE
                              ? SP.minNeckAngle()
                              : std::max( SP.minNeckAngle(), min_neck_angle ) );
    const double neck_max = ( max_neck_angle == INVALID_ANGLE
                              ? SP.maxNeckAngle()
                              : std::min( SP.maxNeckAngle(), max_neck_angle ) );
    const double neck_step = std::max( 1.0, ( neck_max - neck_min ) / 36.0 );


    double best_dir = INVALID_ANGLE;
    double best_score = -std::numeric_limits< double >::max();

    const AbstractPlayerCont::const_iterator end = wm.allPlayers().end();
    for ( double dir = neck_min; dir < neck_max + 0.5; dir += neck_step )
    {
        const AngleDeg left_angle = next_self_body + ( dir - ( view_half_width - 0.01 ) );
        const AngleDeg right_angle = next_self_body + ( dir + ( view_half_width - 0.01 ) );

        double score = calculate_score( wm, next_self_pos, left_angle, right_angle );

        if ( score > best_score )
        {
            best_dir = dir;
            best_score = score;
        }

    }

    if ( best_dir == INVALID_ANGLE )
    {
        return INVALID_ANGLE;
    }


    AngleDeg angle = next_self_body + best_dir;
    return angle.degree();
}


/*-------------------------------------------------------------------*/
/*!

*/
double
Neck_ScanPlayers::calculate_score( const WorldModel & wm,
                                   const Vector2D & next_self_pos,
                                   const AngleDeg & left_angle,
                                   const AngleDeg & right_angle )
{
    double score = 0.0;
    double view_buffer = 90.0;

    const int our_min = std::min( wm.interceptTable()->selfReachCycle(),
                                  wm.interceptTable()->teammateReachCycle() );
    const int opp_min = wm.interceptTable()->opponentReachCycle();
    const bool our_ball = ( our_min <= opp_min );

    const AbstractPlayerCont::const_iterator end = wm.allPlayers().end();
    for ( AbstractPlayerCont::const_iterator p = wm.allPlayers().begin();
          p != end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->isSelf() ) continue;

        Vector2D pos = (*p)->pos() + (*p)->vel();
        AngleDeg angle = ( pos - next_self_pos ).th();

        if ( ! angle.isRightOf( left_angle )
             || ! angle.isLeftOf( right_angle ) )
        {
            continue;
        }

        double pos_count = (*p)->seenPosCount();
        if ( (*p)->isGhost()
             && (*p)->ghostCount() % 2 == 1 )
        {
            pos_count = std::min( 2.0, pos_count );
        }
        pos_count += 1.0;

        if ( our_ball )
        {
            if ( (*p)->side() == wm.ourSide()
                 && ( (*p)->pos().x > wm.ball().pos().x - 10.0
                      || (*p)->pos().x > 30.0 ) )
            {
                pos_count *= 2.0;
            }
        }

        double base_val = std::pow( pos_count, 2 );
        double rate = std::exp( - std::pow( (*p)->distFromSelf(), 2 )
                                / ( 2.0 * std::pow( 20.0, 2 ) ) ); // Magic Number
        score += base_val * rate;

        double buf = std::min( ( angle - left_angle ).abs(),
                               ( angle - right_angle ).abs() );

        if ( buf < view_buffer )
        {
            view_buffer = buf;
        }
    }

    // The bigger view buffer, the bigger rate
    // range: [1.0:2.0]
    // double rate = 2.0 - std::exp( - std::pow( view_buffer, 2 )
    //                               / ( 2.0 * std::pow( 180.0, 2 ) ) ); // Magic Number
    double rate = 1.0 + view_buffer / 90.0;

    score *= rate;

    return score;

}

}

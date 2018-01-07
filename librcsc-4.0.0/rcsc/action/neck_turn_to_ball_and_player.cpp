// -*-c++-*-

/*!
  \file neck_turn_to_ball_and_player.cpp
  \brief check ball and player
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

#include "neck_turn_to_ball_and_player.h"

#include "basic_actions.h"
#include "neck_scan_field.h"
#include "neck_turn_to_ball.h"
#include "neck_turn_to_ball_or_scan.h"
#include "neck_turn_to_player_or_scan.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
bool
Neck_TurnToBallAndPlayer::execute( PlayerAgent * agent )
{
    if ( ! M_target_player )
    {
        return Neck_TurnToBall().execute( agent );
    }
    if ( M_target_player->posCount() < M_count_thr )
    {
        return Neck_TurnToBall().execute( agent );
    }

    if ( M_target_player->isSelf()
         || M_target_player->isGhost() )
    {
        return Neck_TurnToBall().execute( agent );
    }

    const Vector2D ball_next = agent->effector().queuedNextBallPos();
    const Vector2D my_next = agent->effector().queuedNextSelfPos();
    const AngleDeg my_next_body = agent->effector().queuedNextSelfBody();

    const Vector2D player_next = M_target_player->pos() + M_target_player->vel();

    const double next_view_width = agent->effector().queuedNextViewWidth().width();

    const AngleDeg ball_angle = ( ball_next - my_next ).th();
    const AngleDeg player_angle = ( player_next - my_next ).th();
    bool can_face_to_ball = false;
    bool can_face_to_player = false;

    if ( ( ball_angle - my_next_body ).abs()
         < ServerParam::i().maxNeckAngle() + next_view_width * 0.5 - 2.0 )
    {
        can_face_to_ball = true;
    }

    if ( ( player_angle - my_next_body ).abs()
         < ServerParam::i().maxNeckAngle() + next_view_width * 0.5 - 2.0 )
    {
        can_face_to_player = true;
    }

    if ( ! can_face_to_ball
         && ! can_face_to_player )
    {
        return Neck_ScanField().execute( agent );
    }

    if ( can_face_to_ball
         && ! can_face_to_player )
    {

        return Neck_TurnToBall().execute( agent );
    }

    if ( ! can_face_to_ball
         && can_face_to_player )
    {

        return Neck_TurnToPlayerOrScan( M_target_player, 0 ).execute( agent );
    }

    if ( ( ball_angle - player_angle ).abs() > next_view_width - 5.0 )
    {
        return Neck_TurnToBall().execute( agent );
    }

    AngleDeg target_angle = ( ball_angle.isLeftOf( player_angle )
                              ? AngleDeg::bisect( ball_angle, player_angle )
                              : AngleDeg::bisect( player_angle, ball_angle ) );

    target_angle -= my_next_body;
    target_angle -= agent->world().self().neck();
    return agent->doTurnNeck( target_angle );
}

}

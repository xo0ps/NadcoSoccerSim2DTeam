// -*-c++-*-

/*!
  \file basic_actions.cpp
  \brief basic player actions Source File
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

#include "basic_actions.h"

#include "bhv_scan_field.h"
#include "neck_scan_field.h"
#include "neck_scan_players.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/math_util.h>

//#define DEBUG
//#define DEBUG_NECK_TURN_TO_BALL

namespace rcsc {


/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_TurnToAngle::execute( PlayerAgent * agent )
{

    const SelfObject & self = agent->world().self();

    if ( ! self.faceValid() )
    {
        agent->doTurn( 0.0 );
        return false;
    }

    agent->doTurn( M_angle - self.body() );
    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_TurnToPoint::execute( PlayerAgent * agent )
{
    const SelfObject & self = agent->world().self();

    if ( ! self.posValid() )
    {
        return agent->doTurn( 60.0 );
    }

    // relative angle from my predicted pos & body angle to point
    Vector2D my_point = self.inertiaPoint( M_cycle );
    AngleDeg target_rel_angle
        = ( M_point - my_point ).th() - self.body();


    // not consider about max effective turn (inertia moment)
    // try to face to point greedy

    agent->doTurn( target_rel_angle );

    if ( target_rel_angle.abs() < 1.0 )
    {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_TurnToBall::execute( PlayerAgent * agent )
{

    if ( ! agent->world().ball().posValid() )
    {
        return false;
    }

    Vector2D ball_point = agent->world().ball().inertiaPoint( M_cycle );

    return Body_TurnToPoint( ball_point, M_cycle ).execute( agent );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_TackleToPoint::execute( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();
    const ServerParam & sp = ServerParam::i();

    if ( wm.self().tackleProbability() < M_min_prob )
    {
        return false;
    }

    AngleDeg target_angle = ( M_point - wm.ball().pos() ).th();
    AngleDeg target_rel_angle = target_angle - wm.self().body();

    if ( agent->config().version() < 12.0 )
    {
        if ( target_rel_angle.abs() < 90.0 )
        {
            return agent->doTackle( sp.maxTacklePower() );
        }
        else if ( sp.maxBackTacklePower() > 0.0 )
        {
            // backward case
            return agent->doTackle( - sp.maxBackTacklePower() );
        }

        return false;
    }

    AngleDeg ball_rel_angle = wm.ball().rpos().th() - wm.self().body();

    double eff_power
        = sp.maxBackTacklePower()
        + ( ( sp.maxTacklePower() - sp.maxBackTacklePower() )
            * ( 1.0 - target_rel_angle.abs() / 180.0 ) );
    eff_power *= sp.tacklePowerRate();
    eff_power *= 1.0 - 0.5*( ball_rel_angle.abs() / 180.0 );

    Vector2D vel = wm.ball().vel()
        + Vector2D::polar2vector( eff_power, target_angle );


    if ( ( vel.th() - target_angle ).abs() > 90.0 // never accelerate to the target direction
         || vel.r() < M_min_speed ) // too small speed
    {
        return false;
    }

    return agent->doTackle( target_rel_angle.degree() );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Neck_TurnToRelative::execute( PlayerAgent * agent )
{

    return agent->doTurnNeck( M_angle_rel_to_body
                              - agent->world().self().neck() );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Neck_TurnToPoint::execute( PlayerAgent * agent )
{
    const Vector2D next_pos = agent->effector().queuedNextSelfPos();
    const AngleDeg next_body = agent->effector().queuedNextSelfBody();
    const double next_view_width = agent->effector().queuedNextViewWidth().width() * 0.5;

    for ( std::vector< Vector2D >::const_iterator p = M_points.begin();
          p != M_points.end();
          ++p )
    {
        //if( !(*p) ) continue;
        Vector2D rel_pos = *p - next_pos;
        AngleDeg rel_angle = rel_pos.th() - next_body;

        if ( rel_angle.abs() < ServerParam::i().maxNeckAngle() + next_view_width - 5.0 )
        {
            return agent->doTurnNeck( rel_angle - agent->world().self().neck() );
        }
    }
    Neck_ScanField().execute( agent );
    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Neck_TurnToBall::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.ball().posValid() )
    {
        Neck_ScanField().execute( agent );
        return true;
    }

    const Vector2D my_next = agent->effector().queuedNextSelfPos();
    const AngleDeg my_body_next = agent->effector().queuedNextSelfBody();

    const Vector2D ball_next = agent->effector().queuedNextBallPos();
    const AngleDeg ball_angle_next = ( ball_next - my_next ).th();
    const AngleDeg ball_rel_angle_next = ball_angle_next - my_body_next;

    const double next_view_width = agent->effector().queuedNextViewWidth().width();

    //
    // never look the ball
    //
    if ( ball_rel_angle_next.abs()
         > ServerParam::i().maxNeckAngle() + next_view_width * 0.5 )
    {
        Neck_ScanField().execute( agent );
        return true;
    }

    double angle_buf = std::max( 0.0, next_view_width * 0.5 - 10.0 ); //[200803] 15.0

    const PlayerObject * opp = wm.getOpponentNearestToBall( 10 );
    const PlayerObject * mate = wm.getTeammateNearestToBall( 10 );

    const double ball_dist = my_next.dist( ball_next );

    if ( ServerParam::i().visibleDistance() * 0.7 < ball_dist // 2008-07-11
         && ball_dist < 15.0 // 200803
         && ( wm.existKickableTeammate() // 2008-07-11
              || wm.existKickableOpponent()
              || ( opp && opp->distFromBall() < ( opp->playerTypePtr()->kickableArea() + 0.3 ) )
              || ( mate && mate->distFromBall() < ( mate->playerTypePtr()->kickableArea() + 0.3 ) )
              )
         )
    {
        angle_buf = std::max( 0.0, next_view_width * 0.5 - 20.0 ); // 2008-07-11
    }

    //
    // if possible,
    // face to the angle that contans the largest number of players.
    //
    if ( wm.opponentsFromSelf().size() >= 11
         && ( wm.ball().pos().x > 0.0
              || wm.ball().pos().absY() > ServerParam::i().pitchHalfWidth() - 8.0
              || ! opp
              || opp->distFromBall() > 3.0 ) )
    {
        double best_angle = Neck_ScanPlayers::INVALID_ANGLE;

        if ( ball_dist > ServerParam::i().visibleDistance() - 0.3 )
        {
            double min_neck_angle = ball_rel_angle_next.degree() - angle_buf;
            double max_neck_angle = ball_rel_angle_next.degree() + angle_buf;
            best_angle = Neck_ScanPlayers::get_best_angle( agent,
                                                           min_neck_angle,
                                                           max_neck_angle );
        }
        else
        {
            best_angle = Neck_ScanPlayers::get_best_angle( agent );
        }

        if ( best_angle != Neck_ScanPlayers::INVALID_ANGLE )
        {
            AngleDeg target_angle = best_angle;
            AngleDeg neck_moment = target_angle - my_body_next - wm.self().neck();
            agent->doTurnNeck( neck_moment );
            return true;
        }

    }

    //
    // find the lowest accuracy angle
    //

    AngleDeg left_rel_angle = ball_rel_angle_next - angle_buf;
    AngleDeg right_rel_angle = ball_rel_angle_next + angle_buf;


    if ( left_rel_angle.degree() < ServerParam::i().minNeckAngle() )
    {
        left_rel_angle = ServerParam::i().minNeckAngle();
    }

    if ( left_rel_angle.degree() > ServerParam::i().maxNeckAngle() )
    {
        left_rel_angle = ServerParam::i().maxNeckAngle();
    }

    if ( right_rel_angle.degree() < ServerParam::i().minNeckAngle() )
    {
        right_rel_angle = ServerParam::i().minNeckAngle();
    }

    if ( right_rel_angle.degree() > ServerParam::i().maxNeckAngle() )
    {
        right_rel_angle = ServerParam::i().maxNeckAngle();
    }

    int left_sum_count = 0;
    int right_sum_count = 0;

    wm.dirRangeCount( my_body_next + left_rel_angle,
                      next_view_width,
                      NULL, &left_sum_count, NULL );
    wm.dirRangeCount( my_body_next + right_rel_angle,
                      next_view_width,
                      NULL, &right_sum_count, NULL );


    if ( left_sum_count > right_sum_count )
    {
        agent->doTurnNeck( left_rel_angle - wm.self().neck() );
    }
    else
    {
        agent->doTurnNeck( right_rel_angle - wm.self().neck() );
    }

    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BodyNeckToPoint::execute( PlayerAgent * agent )
{

    if ( ! agent->world().self().posValid() )
    {
        return Bhv_ScanField().execute( agent );
    }

    // relative angle from my predicted pos & body angle to point
    Vector2D mynext
        = agent->world().self().pos()
        + agent->world().self().vel();

    AngleDeg target_rel_angle
        = ( M_point - mynext ).th()
        - agent->world().self().body();

    // body & neck is already face to point.
    //if ( (agent->world().self().neck() - target_rel_angle).abs() < 1.0
    //     && target_rel_angle.abs() < 1.0 )
    //{
    //    return false;
    //}

    // calc the max turn angle with current my speed.
    double max_turn
        = effective_turn( ServerParam::i().maxMoment(),
                          agent->world().self().vel().r(),
                          agent->world().self().playerType().inertiaMoment());

    // can face only turn
    if ( target_rel_angle.abs() < max_turn )
    {
        Body_TurnToPoint( M_point ).execute( agent );
        agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
        return true;
    }
    // cannot face only turn
    // turn_neck is required.
    agent->doTurn( target_rel_angle );

    if ( target_rel_angle.degree() > 0.0 )
    {
        target_rel_angle -= max_turn;
    }
    else
    {
        target_rel_angle += max_turn;
    }

    if ( target_rel_angle.degree() < ServerParam::i().minNeckAngle() )
    {
        target_rel_angle = ServerParam::i().minNeckAngle();
    }
    else if ( target_rel_angle.degree() > ServerParam::i().maxNeckAngle() )
    {
        target_rel_angle = ServerParam::i().maxNeckAngle();
    }

    agent->setNeckAction( new Neck_TurnToRelative( target_rel_angle ) );
    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BodyNeckToBall::execute( PlayerAgent * agent )
{

    if ( agent->world().ball().posValid() )
    {
        Vector2D ballnext
            = agent->world().ball().pos()
            + agent->world().ball().vel();
        return Bhv_BodyNeckToPoint( ballnext ).execute( agent );
    }
    else
    {
        return Bhv_ScanField().execute( agent );
    }
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_NeckBodyToPoint::execute( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();

    const double angle_buf = bound( 0.0, M_angle_buf, 180.0 );

    // relative angle from my predicted pos & body angle to point
    Vector2D my_next = wm.self().pos() + wm.self().vel();

    AngleDeg target_rel_angle = ( M_point - my_next ).th() - wm.self().body();

    //if ( ( agent->world().self().neck() - target_rel_angle ).abs() < 1.0 )
    //{
    //    dlog.addText( Logger::ACTION,
    //                  "%s:%d: alerady facing"
    //                  ,__FILE__, __LINE__ );
    //    return false;
    //}

    // can face to point only by turn_neck.
    if ( ServerParam::i().minNeckAngle() + angle_buf < target_rel_angle.degree()
         && target_rel_angle.degree() < ServerParam::i().maxNeckAngle() - angle_buf )
    {
        agent->doTurn( 0.0 );
        agent->setNeckAction( new Neck_TurnToRelative( target_rel_angle ) );
        return true;
    }

    // max turn angle with current my speed
    double max_turn
        = wm.self().playerType().effectiveTurn( ServerParam::i().maxMoment(),
                                                wm.self().vel().r() );

    // can face only by turn
    if ( target_rel_angle.abs() < max_turn )
    {
        agent->doTurn( target_rel_angle );
        agent->setNeckAction( new Neck_TurnToRelative( 0.0 ) );
        return true;
    }

    // cannot face only turn
    // turn_neck is required.
    agent->doTurn( target_rel_angle );

    if ( target_rel_angle.degree() > 0.0 )
    {
        target_rel_angle -= max_turn;
    }
    else
    {
        target_rel_angle += max_turn;
    }

    // moment is justified automatically.
    agent->setNeckAction( new Neck_TurnToRelative( target_rel_angle ) );
    return true;
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_NeckBodyToBall::execute( PlayerAgent * agent )
{

    if ( agent->world().ball().posValid() )
    {
        Vector2D ball_next
            = agent->world().ball().pos()
            + agent->world().ball().vel();

        return Bhv_NeckBodyToPoint( ball_next, M_angle_buf ).execute( agent );
    }
    else
    {
        return Bhv_ScanField().execute( agent );
    }
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_Wide::execute( PlayerAgent * agent )
{
    return agent->doChangeView( ViewWidth::WIDE );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_Normal::execute( PlayerAgent * agent )
{
    return agent->doChangeView( ViewWidth::NORMAL );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
View_ChangeWidth::execute( PlayerAgent * agent )
{
    return agent->doChangeView( M_width );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Arm_PointToPoint::execute( PlayerAgent * agent )
{
    if ( agent->world().self().armMovable() > 0 )
    {
        return false;
    }

    return agent->doPointto( M_point.x, M_point.y );
}

/////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
bool
Arm_Off::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::ACTION,
                  __FILE__": Arm_Off" );

    if ( agent->world().self().armMovable() > 0 )
    {
        dlog.addText( Logger::ACTION,
                      "Arm_Off. arm is not movable." );
        return false;
    }

    return agent->doPointtoOff();
}

/////////////////////////////////////////////////////////////////////

}

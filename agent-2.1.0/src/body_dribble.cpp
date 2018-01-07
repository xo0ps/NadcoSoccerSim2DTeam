// -*-c++-*-

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

#include "body_dribble.h"
#include "body_intercept.h"

#include <rcsc/action/intention_dribble2008.h>
#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_kick_to_relative.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/player/audio_sensor.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/common/server_param.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>
#include <rcsc/timer.h>
#include <rcsc/geom/sector_2d.h>

#define USE_CHANGE_VIEW

struct KeepDribbleCmp
    : public std::binary_function< Body_Dribble::KeepDribbleInfo,
                                   Body_Dribble::KeepDribbleInfo,
                                   bool > {
    result_type operator()( const first_argument_type & lhs,
                            const second_argument_type & rhs ) const
      {
          if ( lhs.dash_count_ > rhs.dash_count_ )
          {
              return true;
          }

          if ( lhs.dash_count_ == rhs.dash_count_ )
          {
              if ( lhs.min_opp_dist_ > 5.0
                   && rhs.min_opp_dist_ > 5.0 )
              {
                  return lhs.ball_forward_travel_ > rhs.ball_forward_travel_;
                  //return lhs.ball_forward_travel_ < rhs.ball_forward_travel_;
                  ///return lhs.last_ball_rel_.absX() < rhs.last_ball_rel_.absX();
              }

              return lhs.min_opp_dist_ > rhs.min_opp_dist_;
          }

          return false;
      }
};

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Body_Dribble::execute( rcsc::PlayerAgent * agent )
{
    if ( ! agent->world().self().isKickable() )
    {
        return Body_Intercept( false ).execute( agent );
    }

    if ( ! agent->world().ball().velValid() )
    {
        return Body_StopBall().execute( agent );
    }

    M_dash_power = agent->world().self().getSafetyDashPower( M_dash_power );

    doAction( agent,
              M_target_point,
              M_dash_power,
              M_dash_count,
              M_dodge_mode );  // dodge mode;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Body_Dribble::say( rcsc::PlayerAgent * agent,
                       const rcsc::Vector2D & target_point,
                       const int queue_count )
{
    if ( agent->config().useCommunication() )
    {
        agent->addSayMessage( new DribbleMessage( target_point, queue_count ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doAction( rcsc::PlayerAgent * agent,
                            const rcsc::Vector2D & target_point,
                            const double & dash_power,
                            const int dash_count,
                            const bool dodge,
                            const bool dodge_mode )
{
    // try to create the action queue.
    // kick -> dash -> dash -> ...
    // the number of dash is specified by M_dash_count

    /*--------------------------------------------------------*/
    // do dodge dribble
    if ( dodge
         && isDodgeSituation( agent, target_point ) )
    {
        return doDodge( agent, target_point );
    }

    /*--------------------------------------------------------*/
    // normal dribble

    const rcsc::WorldModel & wm = agent->world();
    const rcsc::Vector2D my_final_pos= wm.self().inertiaFinalPoint();
    const rcsc::Vector2D target_rel = target_point - my_final_pos;
    const double target_dist = target_rel.r();

    // already reach the target point
    if ( target_dist < M_dist_thr )
    {
        return Body_HoldBall().execute( agent );
    }


    /*--------------------------------------------------------*/
    // decide dribble angle & dist

    if ( doTurn( agent, target_point, dash_power, dash_count, dodge ) )
    {
        return true;
    }

    /*--------------------------------------------------------*/
    // after one dash, ball will kickable
    double used_dash_power = dash_power;
    if ( canKickAfterDash( agent, &used_dash_power ) )
    {
        return agent->doDash( used_dash_power );
    }

    /*--------------------------------------------------------*/
    // do kick first

    if ( ( ! dodge_mode || dash_count >= 2 ) &&
         doKickDashesWithBall( agent, target_point, dash_power, dash_count,
                               dodge_mode ) )
    {
        return true;
    }

    return doKickDashes( agent, target_point, dash_power, dash_count );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doTurn( rcsc::PlayerAgent * agent,
                          const rcsc::Vector2D & target_point,
                          const double & dash_power,
                          const int /*dash_count*/,
                          const bool /*dodge*/ )
{
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D my_final_pos = wm.self().inertiaFinalPoint();
    const rcsc::Vector2D target_rel = target_point - my_final_pos;
    const rcsc::AngleDeg target_angle = target_rel.th();
    const double dir_diff
        = ( dash_power > 0.0
            ? ( target_angle - wm.self().body() ).degree()
            : ( target_angle - wm.self().body() - 180.0 ).degree() );
    const double dir_margin_abs
        = std::max( 15.0,
                    std::fabs( AngleDeg::atan2_deg( M_dist_thr, target_rel.r() ) ) );

    /*--------------------------------------------------------*/
    // already facing to the target
    if ( std::fabs( dir_diff ) < dir_margin_abs )
    {
        return false;
    }

    if ( doTurnOnly( agent, target_point, dash_power,
                     dir_diff ) )
    {
        return true;
    }

    if ( doKickTurnsDash( agent, target_point, dash_power,
                          dir_diff, dir_margin_abs ) )
    {
        return true;
    }

    // just stop the ball

    double kick_power = wm.ball().vel().r() / wm.self().kickRate();
    rcsc::AngleDeg kick_dir = ( wm.ball().vel().th() - 180.0 ) - wm.self().body();

    agent->doKick( kick_power, kick_dir );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doTurnOnly( rcsc::PlayerAgent * agent,
                              const rcsc::Vector2D & /*target_point*/,
                              const double & /*dash_power*/,
                              const double & dir_diff )
{
    const rcsc::WorldModel & wm = agent->world();

    //---------------------------------------------------------//
    // check opponent
    if ( wm.interceptTable()->opponentReachCycle() <= 1 )
    {
        return false;
    }

    //---------------------------------------------------------//
    // check next ball dist after turn
    rcsc::Vector2D my_next = wm.self().pos() + wm.self().vel();
    rcsc::Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
    const double ball_next_dist = my_next.dist( ball_next );

    // not kickable at next cycle, if do turn at current cycle.
    if ( ball_next_dist
         > ( wm.self().playerType().kickableArea()
             - wm.ball().vel().r() * ServerParam::i().ballRand()
             - wm.self().vel().r() * ServerParam::i().playerRand()
             - 0.15 ) )
    {
        return false;
    }


    //---------------------------------------------------------//
    // check required turn step.
    //if ( ! wm.self().canTurn( dir_diff ) )
    double my_speed = wm.self().vel().r();
    if( dir_diff <= wm.self().playerType().effectiveTurn( rcsc::ServerParam::i().maxMoment(), my_speed ) )
    {
        // it is necessary to turn more than one step.
        rcsc::Vector2D my_next2 = wm.self().inertiaPoint( 2 );
        rcsc::Vector2D ball_next2 = wm.ball().inertiaPoint( 2 );
        double ball_dist_next2 = my_next2.dist( ball_next2 );
        if ( ball_dist_next2
             > ( wm.self().playerType().kickableArea()
                 - wm.ball().vel().r() * ServerParam::i().ballRand()
                 - wm.self().vel().r() * ServerParam::i().playerRand()
                 - 0.15 ) )
        {
            return false;
        }
    }

    //
    // check opponent
    //
    const rcsc::PlayerObject * nearest_opp = wm.getOpponentNearestToBall( 5 );
    if ( nearest_opp )
    {
        rcsc::Vector2D opp_next = nearest_opp->pos() + nearest_opp->vel();
        rcsc::AngleDeg opp_angle = ( nearest_opp->bodyCount() == 0
                               ? nearest_opp->body()
                               : nearest_opp->vel().th() );
        rcsc::Line2D opp_line( opp_next, opp_angle );
        if ( opp_line.dist( ball_next ) < 1.1
             && nearest_opp->pos().dist( ball_next ) < 2.0 )
        {
            return false;
        }
    }

    //---------------------------------------------------------//
    // turn only
    agent->doTurn( dir_diff );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doCollideWithBall( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    rcsc::Vector2D required_accel = wm.self().vel(); // target relative pos
    required_accel -= wm.ball().rpos(); // required vel
    required_accel -= wm.ball().vel(); // ball next rpos

    double required_power = required_accel.r()/ wm.self().kickRate();
    if ( required_power > ServerParam::i().maxPower() * 1.1 )
    {
        return false;
    }

    agent->doKick( std::min( required_power, ServerParam::i().maxPower() ),
                   required_accel.th() - wm.self().body() );
    return true;

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doCollideForTurn( rcsc::PlayerAgent * agent,
                                    const double & dir_diff_abs,
                                    const bool kick_first )
{
    const rcsc::WorldModel & wm = agent->world();
    double my_speed = wm.self().vel().r();

    if ( kick_first )
    {
        my_speed *= wm.self().playerType().playerDecay();
    }

    const double max_turn_moment
        = wm.self().playerType().effectiveTurn( ServerParam::i().maxMoment(),
                                                my_speed );

    if ( max_turn_moment > dir_diff_abs * 0.9 )
    {
        return false;
    }

    if ( doCollideWithBall( agent ) )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!
  if back dash mode, dash_power is negative value.
*/
bool
Body_Dribble::doKickTurnsDash( rcsc::PlayerAgent * agent,
                                   const rcsc::Vector2D & target_point,
                                   const double & dash_power,
                                   const double & dir_diff,
                                   const double & /*dir_margin_abs*/ )
{
    // try to create these action queue
    // kick -> turn -> turn -> ... -> one dash -> normal dribble kick

    // assume that ball kickable and require to turn to target.

    const rcsc::WorldModel & wm = agent->world();

    const rcsc::Vector2D my_final_pos = wm.self().inertiaFinalPoint();
    const rcsc::Vector2D target_rel = target_point - my_final_pos;
    const rcsc::AngleDeg target_angle = target_rel.th();

    // simulate kick - turn - dash

    // first step is kick
    double my_speed
        = wm.self().vel().r()
        * wm.self().playerType().playerDecay();
    int n_turn = 0;
    double dir_diff_abs = std::fabs( dir_diff );

    while ( dir_diff_abs > 0.0 )
    {
        // !!! it is necessary to consider about self inertia moment

        double moment_abs = effective_turn( ServerParam::i().maxMoment(),
                                            my_speed,
                                            wm.self().playerType().inertiaMoment() );
        moment_abs = std::min( dir_diff_abs, moment_abs );
        dir_diff_abs -= moment_abs;
        my_speed *= wm.self().playerType().playerDecay();
        ++n_turn;
    }

    if ( n_turn <= 2
         //&& wm.dirCount( target_angle ) <= 5
         && wm.ball().pos().x > 0.0
         && doKickTurnsDashes( agent, target_point, dash_power, n_turn ) )
    {
        return true;
    }

    rcsc::AngleDeg keep_global_angle;
    bool exist_opp = existCloseOpponent( agent, &keep_global_angle );

    if ( ! exist_opp
         && doCollideForTurn( agent, std::fabs( dir_diff ), true ) )
    {
        // several turns are required after kick.
        // try to collide with ball.
        return true;
    }

    const rcsc::Vector2D my_pos
        = inertia_n_step_point
        ( rcsc::Vector2D( 0.0, 0.0 ),
          wm.self().vel(),
          1 + n_turn, // kick + turns
          wm.self().playerType().playerDecay() );

    const double control_dist = wm.self().playerType().kickableArea() * 0.7;

    const rcsc::PlayerObject * nearest_opp = wm.getOpponentNearestToBall( 5 );
    if ( nearest_opp
         && nearest_opp->distFromBall() < 5.0 )
    {
        double best_angle = 0.0;
        double min_dist2 = 100000.0;
        for ( double angle = -90.0; angle <= 91.0; angle += 10.0 )
        {
            rcsc::Vector2D keep_pos = my_pos + rcsc::Vector2D::from_polar( control_dist, target_angle + angle );
            double d2 = nearest_opp->pos().dist2( keep_pos );
            if ( d2 < min_dist2 )
            {
                best_angle = angle;
                min_dist2 = d2;
            }
        }
        keep_global_angle = target_angle + best_angle;
    }
    else if ( ! exist_opp )
    {
        if ( target_angle.isLeftOf( wm.ball().angleFromSelf() ) )
        {
            keep_global_angle = target_angle + 35.0;
        }
        else
        {
            keep_global_angle = target_angle - 35.0;
        }
    }

    // relative to current my pos, angle is global
    rcsc::Vector2D required_ball_rel_pos
        = my_pos
        + rcsc::Vector2D::polar2vector( control_dist, keep_global_angle );


    // travel = firstvel * (1 + dec + dec^2 + ...)
    // firstvel = travel / (1 + dec + dec^2 + ...)
    const double term
        = ( 1.0 - std::pow( ServerParam::i().ballDecay(), n_turn + 2 ) )
        / ( 1.0 - ServerParam::i().ballDecay() );
    const rcsc::Vector2D required_first_vel
        = ( required_ball_rel_pos - wm.ball().rpos() ) / term;
    const rcsc::Vector2D required_accel
        = required_first_vel
        - wm.ball().vel();

    // check power overflow
    const double required_kick_power
        = required_accel.r() / wm.self().kickRate();

    // cannot get the required accel using only one kick
    if ( required_kick_power > ServerParam::i().maxPower()
         || required_first_vel.r() > ServerParam::i().ballSpeedMax() )
    {
        rcsc::Vector2D ball_next
            = wm.self().pos() + wm.self().vel()
            + rcsc::Vector2D::polar2vector( control_dist, keep_global_angle );
        if ( ball_next.absX() > ServerParam::i().pitchHalfLength() - 0.5
             || ball_next.absY() > ServerParam::i().pitchHalfLength() - 0.5 )
        {
            return false;
        }

        return Body_KickToRelative( control_dist,
                                    keep_global_angle - wm.self().body(),
                                    false // not need to stop
                                    ).execute( agent );
    }

    // can archieve required vel

    //////////////////////////////////////////////////////////
    // register intention
    agent->setIntention
        ( new IntentionDribble2008( target_point,
                                    M_dist_thr,
                                    n_turn,
                                    1, // one dash
                                    std::fabs( dash_power ),
                                    ( dash_power < 0.0 ), // back_dash
                                    wm.time() ) );
    say( agent, target_point, n_turn + 1 );
#ifdef USE_CHANGE_VIEW
    if ( wm.gameMode().type() == GameMode::PlayOn
         && n_turn + 1 >= 3 )
    {
        agent->setViewAction( new View_Normal() );
    }
#endif

    // execute first kick
    return agent->doKick( required_kick_power,
                          required_accel.th() - wm.self().body() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doKickTurnsDashes( rcsc::PlayerAgent * agent,
                                     const rcsc::Vector2D & target_point,
                                     const double & dash_power,
                                     const int n_turn )
{
    static std::vector< Vector2D > self_cache;

    const int max_dash = 5;

    const rcsc::WorldModel & wm = agent->world();

    createSelfCache( agent,
                     target_point, dash_power,
                     n_turn, max_dash, self_cache );

    const double max_moment = ServerParam::i().maxMoment();
    const rcsc::AngleDeg accel_angle = ( target_point - self_cache[n_turn] ).th();

    const rcsc::Vector2D trap_rel
        = rcsc::Vector2D::polar2vector( ( wm.self().playerType().playerSize()
                                    + wm.self().playerType().kickableMargin() * 0.2
                                    + ServerParam::i().ballSize() ),
                                  accel_angle );

    for ( int n_dash = max_dash; n_dash >= 2; --n_dash )
    {
        const rcsc::Vector2D ball_trap_pos = self_cache[n_turn + n_dash] + trap_rel;

        if ( ball_trap_pos.absX() > ServerParam::i().pitchHalfLength() - 0.5
             || ball_trap_pos.absY() > ServerParam::i().pitchHalfWidth() - 0.5 )
        {
            continue;
        }

        const double term
            = ( 1.0 - std::pow( ServerParam::i().ballDecay(), 1 + n_turn + n_dash ) )
            / ( 1.0 - ServerParam::i().ballDecay() );
        const rcsc::Vector2D first_vel = ( ball_trap_pos - wm.ball().pos() ) / term;
        const rcsc::Vector2D kick_accel = first_vel - wm.ball().vel();
        const double kick_power = kick_accel.r() / wm.self().kickRate();

        if ( kick_power > ServerParam::i().maxPower()
             || kick_accel.r() > ServerParam::i().ballAccelMax()
             || first_vel.r() > ServerParam::i().ballSpeedMax() )
        {
            continue;
        }

        if ( ( wm.ball().pos() + first_vel ).dist( self_cache[0] )
             < wm.self().playerType().playerSize() + ServerParam::i().ballSize() + 0.1 )
        {
            continue;
        }

        bool failed = false;

        const int dribble_step = 1 + n_turn + n_dash;

        const rcsc::PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
        for ( rcsc::PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
              o != o_end;
              ++o )
        {
			if( !(*o) ) continue;
            if ( (*o)->distFromSelf() > 30.0 ) break;

            bool goalie = false;
            double control_area = (*o)->playerTypePtr()->kickableArea();
            if ( (*o)->goalie()
                 && ball_trap_pos.x > ServerParam::i().theirPenaltyAreaLineX()
                 && ball_trap_pos.absY() < ServerParam::i().penaltyAreaHalfWidth() )
            {
                goalie = true;
                control_area = rcsc::ServerParam::i().catchableArea();
            }

            const rcsc::Vector2D & opos = ( (*o)->seenPosCount() <= (*o)->posCount()
                                      ? (*o)->seenPos()
                                      : (*o)->pos() );
            const int vel_count = std::min( (*o)->seenVelCount(), (*o)->velCount() );
            const rcsc::Vector2D & ovel = ( (*o)->seenVelCount() <= (*o)->velCount()
                                      ? (*o)->seenVel()
                                      : (*o)->vel() );

            rcsc::Vector2D opp_pos = ( (*o)->velCount() <= 1
                                 ? inertia_n_step_point( opos, ovel, dribble_step,
                                                         (*o)->playerTypePtr()->playerDecay() )
                                 : opos + ovel );
            rcsc::Vector2D opp_to_pos = ball_trap_pos - opp_pos;

            double opp_dist = opp_to_pos.r();
            int opp_turn_step = 0;

            if ( (*o)->bodyCount() <= 5
                 || vel_count <= 5 )
            {
                double angle_diff = ( (*o)->bodyCount() <= 1
                                      ? ( opp_to_pos.th() - (*o)->body() ).abs()
                                      : ( opp_to_pos.th() - ovel.th() ).abs() );

                double turn_margin = 180.0;
                if ( control_area < opp_dist )
                {
                    turn_margin = rcsc::AngleDeg::asin_deg( control_area / opp_dist );
                }
                turn_margin = std::max( turn_margin, 15.0 );

                double opp_speed = ovel.r();
                while ( angle_diff > turn_margin )
                {
                    double max_turn = (*o)->playerTypePtr()->effectiveTurn( max_moment, opp_speed );
                    angle_diff -= max_turn;
                    opp_speed *= (*o)->playerTypePtr()->playerDecay();
                    ++opp_turn_step;
                }
            }

            opp_dist -= control_area;
            opp_dist -= 0.2;
            //opp_dist -= (*o)->distFromSelf() * 0.05;

            if ( opp_dist < 0.0 )
            {
                failed = true;
                break;
            }

            int opp_reach_step = (*o)->playerTypePtr()->cyclesToReachDistance( opp_dist );
            opp_reach_step += opp_turn_step;
            opp_reach_step -= bound( 0, (*o)->posCount(), 10 );

            if ( opp_reach_step <= dribble_step )
            {
                failed = true;
                break;
            }
        }

        if ( failed ) continue;

        agent->doKick( kick_power, kick_accel.th() - wm.self().body() );

        agent->setIntention
            ( new IntentionDribble2008( target_point,
                                        M_dist_thr,
                                        n_turn,
                                        n_dash,
                                        std::fabs( dash_power ),
                                        ( dash_power < 0.0 ), // back_dash
                                        wm.time() ) );
        say( agent, target_point, n_dash + n_turn );
#ifdef USE_CHANGE_VIEW
        if ( n_turn + n_dash >= 3 )
        {
            agent->setViewAction( new View_Normal() );
        }
#endif
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doKickDashes( rcsc::PlayerAgent * agent,
                                const Vector2D & target_point,
                                const double & dash_power,
                                const int dash_count )
{
    static std::vector< Vector2D > self_cache;

    // do dribble kick. simulate next action queue.
    // kick -> dash -> dash -> ...

    const rcsc::WorldModel & wm = agent->world();

    ////////////////////////////////////////////////////////
    // simulate my pos after one kick & dashes
    createSelfCache( agent,
                     target_point, dash_power,
                     0, dash_count, // no turn
                     self_cache );

    // my moved position after 1 kick and n dashes
    const rcsc::Vector2D my_pos = self_cache.back() - wm.self().pos();
    const double my_move_dist = my_pos.r();
    // my move direction
    const rcsc::AngleDeg my_move_dir = my_pos.th();

    const rcsc::AngleDeg accel_angle = ( dash_power > 0.0
                                   ? wm.self().body()
                                   : wm.self().body() - 180.0 );

    ////////////////////////////////////////////////////////
    // estimate required kick param

    // decide next ball control point

    rcsc::AngleDeg keep_global_angle;
    bool exist_close_opp = existCloseOpponent( agent, &keep_global_angle );

    double control_dist;
    double add_angle_abs;
    {
        double y_dist
            = wm.self().playerType().playerSize()
            + ServerParam::i().ballSize()
            + 0.2;
        Vector2D cur_ball_rel = wm.ball().rpos().rotatedVector( - my_move_dir );
        if ( cur_ball_rel.absY() < y_dist )
        {
            //y_dist = ( y_dist + cur_ball_rel.absY() ) * 0.5;
            y_dist += 0.1;
            y_dist = std::min( y_dist, cur_ball_rel.absY() );
        }
        double x_dist
            = std::sqrt( std::pow( wm.self().playerType().kickableArea(), 2 )
                         - std::pow( y_dist, 2 ) )
            - 0.2 - std::min( 0.6, my_move_dist * 0.05 );
        control_dist = std::sqrt( std::pow( x_dist, 2 ) + std::pow( y_dist, 2 ) );
        add_angle_abs = std::fabs( AngleDeg::atan2_deg( y_dist, x_dist ) );
    }

    if ( exist_close_opp )
    {
        //         if ( my_move_dir.isLeftOf( keep_global_angle ) )
        //         {
        //             keep_global_angle = my_move_dir + add_angle_abs;
        //             dlog.addText( Logger::DRIBBLE,
        //                           __FILE__": doKickDashes() avoid. keep right" );
        //         }
        //         else
        //         {
        //             keep_global_angle = my_move_dir - add_angle_abs;
        //             dlog.addText( Logger::DRIBBLE,
        //                           __FILE__": doKickDashes() avoid. keep left" );
        //         }
    }
    else
    {
        if ( my_move_dir.isLeftOf( wm.ball().angleFromSelf() ) )
        {
            keep_global_angle = my_move_dir + add_angle_abs;
         
        }
        else
        {
            keep_global_angle = my_move_dir - add_angle_abs;
         
        }
    }

    const rcsc::Vector2D next_ball_rel
        = rcsc::Vector2D::polar2vector( control_dist, keep_global_angle );
    rcsc::Vector2D next_ctrl_ball_pos = wm.self().pos() + my_pos + next_ball_rel;

    // calculate required kick param

    // relative to current my pos
    const rcsc::Vector2D required_ball_pos = my_pos + next_ball_rel;
    const double term
        = ( 1.0 - std::pow( ServerParam::i().ballDecay(), dash_count + 1 ) )
        / ( 1.0 - ServerParam::i().ballDecay() );
    const rcsc::Vector2D required_first_vel
        = (required_ball_pos - wm.ball().rpos()) / term;
    const rcsc::Vector2D required_accel
        = required_first_vel
        - wm.ball().vel();
    const double required_kick_power
        = required_accel.r() / wm.self().kickRate();

    ////////////////////////////////////////////////////////
    // never kickable
    if ( required_kick_power > ServerParam::i().maxPower()
         || required_first_vel.r() > ServerParam::i().ballSpeedMax() )
    {
        return Body_KickToRelative( wm.self().playerType().kickableArea() * 0.7,
                                    keep_global_angle - wm.self().body(),
                                    false  // not need to stop
                                    ).execute( agent );
    }


    ////////////////////////////////////////////////////////
    // check next collision

    const double collide_dist2
        = std::pow( wm.self().playerType().playerSize()
                    + ServerParam::i().ballSize()
                    + 0.15,
                    2 );
    if ( ( wm.ball().rpos()
           + required_first_vel - wm.self().vel() ).r2() // next rel pos
         < collide_dist2 )
    {
        AngleDeg rotate_global_angle = keep_global_angle;
        if ( ( wm.ball().angleFromSelf() - my_move_dir ).abs() > 90.0 )
        {
            if ( keep_global_angle.isLeftOf( my_move_dir ) )
            {
                rotate_global_angle = my_move_dir + 90.0;
            }
            else
            {
                rotate_global_angle = my_move_dir + 90.0;
            }
        }
        rcsc::AngleDeg rotate_rel_angle = rotate_global_angle - wm.self().body();
        return Body_KickToRelative( wm.self().playerType().kickableArea() * 0.7,
                                    rotate_rel_angle,
                                    false  // not need to stop
                                    ).execute( agent );
    }

    //////////////////////////////////////////////////////////
    // register intention
    agent->setIntention
        ( new IntentionDribble2008( target_point,
                                    M_dist_thr,
                                    0, // zero turn
                                    dash_count,
                                    std::fabs( dash_power ),
                                    ( dash_power < 0.0 ), // back_dash
                                    wm.time() ) );
    say( agent, target_point, dash_count );
#ifdef USE_CHANGE_VIEW
    if ( dash_count >= 2 )
    {
        agent->setViewAction( new View_Normal() );
    }
#endif

    // execute first kick
    return agent->doKick( required_kick_power,
                          required_accel.th() - wm.self().body() );
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doKickDashesWithBall( rcsc::PlayerAgent * agent,
                                        const Vector2D & target_point,
                                        const double & dash_power,
                                        const int dash_count,
                                        const bool dodge_mode )
{
    static std::vector< Vector2D > my_state;
    static std::vector< KeepDribbleInfo > dribble_info;

    my_state.clear();
    dribble_info.clear();

    // do dribble kick. simulate next action queue.
    // kick -> dash -> dash -> ...
    const rcsc::WorldModel & wm = agent->world();

    MSecTimer timer;

    // estimate my move positions
    createSelfCache( agent,
                     target_point, dash_power,
                     0, // no turn
                     std::max( 12, dash_count ),
                     my_state );

    const rcsc::AngleDeg accel_angle = ( dash_power > 0.0
                                   ? wm.self().body()
                                   : wm.self().body() - 180.0 );

    const int DIST_DIVS = 10;

    const double max_dist = wm.self().playerType().kickableArea() + 0.2;
    double first_ball_dist = ( wm.self().playerType().playerSize()
                               + ServerParam::i().ballSize()
                               + 0.15 );
    const double dist_step = ( max_dist - first_ball_dist ) / ( DIST_DIVS - 1 );

    const double angle_range = 240.0;
    const double angle_range_forward = 160.0;
    const double arc_dist_step = 0.1;

    int total_loop_count = 0;

    for ( int dist_loop = 0;
          dist_loop < DIST_DIVS;
          ++dist_loop, first_ball_dist += dist_step )
    {
        const double angle_step
            = ( arc_dist_step * 360.0 )
            / ( 2.0 * first_ball_dist * M_PI );
        const int ANGLE_DIVS
            = ( first_ball_dist < wm.self().playerType().kickableArea() - 0.1
                ? static_cast< int >( std::ceil( angle_range / angle_step ) ) + 1
                : static_cast< int >( std::ceil( angle_range_forward / angle_step ) ) + 1 );

        rcsc::AngleDeg first_ball_angle = accel_angle - angle_step * ( ANGLE_DIVS/2 );

        // angle loop
        for ( int angle_loop = 0;
              angle_loop < ANGLE_DIVS;
              ++angle_loop, first_ball_angle += angle_step )
        {
            ++total_loop_count;

            const rcsc::Vector2D first_ball_pos
                = my_state.front()
                + rcsc::Vector2D::polar2vector( first_ball_dist, first_ball_angle );
            const rcsc::Vector2D first_ball_vel = first_ball_pos - wm.ball().pos();

            KeepDribbleInfo info;
            if ( simulateKickDashes( agent ,
                                     my_state,
                                     dash_count,
                                     accel_angle,
                                     first_ball_pos,
                                     first_ball_vel,
                                     &info ) )
            {
                dribble_info.push_back( info );
            }
        }
    }

    if ( dribble_info.empty() )
    {
        return false;
    }

    std::vector< KeepDribbleInfo >::const_iterator dribble
        = std::min_element( dribble_info.begin(),
                            dribble_info.end(),
                            KeepDribbleCmp() );

    if ( dodge_mode
         && dash_count > dribble->dash_count_ )
    {
        return false;
    }


#if 1
    {
        rcsc::Vector2D ball_pos = wm.ball().pos();
        rcsc::Vector2D ball_vel = dribble->first_ball_vel_;
        const int DASH = dribble->dash_count_ + 1;
        for ( int i = 0; i < DASH; ++i )
        {
            ball_pos += ball_vel;
            ball_vel *= ServerParam::i().ballDecay();
        }
    }
#endif

    rcsc::Vector2D kick_accel = dribble->first_ball_vel_ - wm.ball().vel();

    // execute first kick
    agent->doKick( kick_accel.r() / wm.self().kickRate(),
                   kick_accel.th() - wm.self().body() );

    //
    // register intention
    //
    agent->setIntention
        ( new IntentionDribble2008( target_point,
                                    M_dist_thr,
                                    0, // zero turn
                                    std::min( dribble->dash_count_, dash_count ),
                                    std::fabs( dash_power ),
                                    ( dash_power < 0.0 ), // back_dash
                                    wm.time() ) );
    say( agent, target_point, std::min( dribble->dash_count_, dash_count ) );
#ifdef USE_CHANGE_VIEW
    if ( std::min( dribble->dash_count_, dash_count ) >= 2 )
    {
        agent->setViewAction( new View_Normal() );
    }
#endif

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Body_Dribble::createSelfCache( rcsc::PlayerAgent * agent,
                                   const Vector2D & target_point,
                                   const double & dash_power,
                                   const int turn_count,
                                   const int dash_count,
                                   std::vector< Vector2D > & self_cache )
{
    const rcsc::WorldModel & wm = agent->world();

    self_cache.clear();
    self_cache.reserve( turn_count + dash_count + 1 );

    double my_stamina = wm.self().stamina();
    double my_effort = wm.self().effort();
    double my_recovery = wm.self().recovery();

    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D my_vel = wm.self().vel();

    my_pos += my_vel;
    my_vel *= wm.self().playerType().playerDecay();

    self_cache.push_back( my_pos ); // first element is next cycle just after kick

    for ( int i = 0; i < turn_count; ++i )
    {
        my_pos += my_vel;
        my_vel *= wm.self().playerType().playerDecay();
        self_cache.push_back( my_pos );
    }

    wm.self().playerType().predictStaminaAfterWait( ServerParam::i(),
                                                    1 + turn_count,
                                                    &my_stamina,
                                                    &my_effort,
                                                    my_recovery );
    rcsc::AngleDeg accel_angle;
    if ( turn_count == 0 )
    {
        accel_angle = ( dash_power > 0.0
                        ? wm.self().body()
                        : wm.self().body() - 180.0 );
    }
    else
    {
        accel_angle = ( target_point - wm.self().inertiaFinalPoint() ).th();
    }

    for ( int i = 0; i < dash_count; ++i )
    {
        double available_stamina
            =  std::max( 0.0,
                         my_stamina
                         - ServerParam::i().recoverDecThrValue()
                         - 300.0 );
        double consumed_stamina = ( dash_power > 0.0
                                    ? dash_power
                                    : dash_power * -2.0 );
        consumed_stamina = std::min( available_stamina,
                                     consumed_stamina );
        double used_power = ( dash_power > 0.0
                              ? consumed_stamina
                              : consumed_stamina * -0.5 );
        double max_accel_mag = ( std::fabs( used_power )
                                 * wm.self().playerType().dashPowerRate()
                                 * my_effort );
        double accel_mag = max_accel_mag;
        if ( wm.self().playerType().normalizeAccel( my_vel,
                                                    accel_angle,
                                                    &accel_mag ) )
        {
            used_power *= accel_mag / max_accel_mag;
        }

        rcsc::Vector2D dash_accel
            = rcsc::Vector2D::polar2vector( std::fabs( used_power )
                                      * my_effort
                                      * wm.self().playerType().dashPowerRate(),
                                      accel_angle );
        my_vel += dash_accel;
        my_pos += my_vel;

        self_cache.push_back( my_pos );

        my_vel *= wm.self().playerType().playerDecay();

        wm.self().playerType().predictStaminaAfterOneDash( ServerParam::i(),
                                                           used_power,
                                                           &my_stamina,
                                                           &my_effort,
                                                           &my_recovery );
    }
}

bool
Body_Dribble::simulateKickDashes( rcsc::PlayerAgent * agent,
                                      const std::vector< Vector2D > & self_cache,
                                      const int dash_count,
                                      const rcsc::AngleDeg & accel_angle,
                                      const rcsc::Vector2D & first_ball_pos,
                                      const rcsc::Vector2D & first_ball_vel,
                                      KeepDribbleInfo * dribble_info )
{
	
	
    const rcsc::WorldModel & wm = agent->world();
    static const rcsc::Rect2D pitch_rect( Vector2D( - ServerParam::i().pitchHalfLength() + 0.2,
                                              - ServerParam::i().pitchHalfWidth() + 0.2 ),
                                    Size2D( ServerParam::i().pitchLength() - 0.4,
                                            ServerParam::i().pitchWidth() - 0.4 ) );

    if ( ! pitch_rect.contains( first_ball_pos ) )
    {
        return false;
    }

    const ServerParam & param = ServerParam::i();
    const double collide_dist = ( wm.self().playerType().playerSize()
                                  + param.ballSize() );
    const double kickable_area = wm.self().playerType().kickableArea();

    const rcsc::Vector2D first_ball_accel = first_ball_vel - wm.ball().vel();
    const double first_ball_accel_r = first_ball_accel.r();

    if ( first_ball_vel.r() > param.ballSpeedMax()
         || first_ball_accel_r > param.ballAccelMax()
         || ( first_ball_accel_r > wm.self().kickRate() * param.maxPower() )
         )
    {
        // cannot acccelerate to the desired speed
        return false;
    }

    double min_opp_dist = 1000.0;

    if ( existKickableOpponent( agent , first_ball_pos, &min_opp_dist ) )
    {
        return false;
    }

    rcsc::Vector2D ball_pos = first_ball_pos;
    rcsc::Vector2D ball_vel = first_ball_vel;
    ball_vel *= param.ballDecay();

    int tmp_dash_count = 0;
    rcsc::Vector2D total_ball_move( 0.0, 0.0 );
    rcsc::Vector2D last_ball_rel( 0.0, 0.0 );

    // future state loop
    const std::vector< Vector2D >::const_iterator my_end = self_cache.end();
    for ( std::vector< Vector2D >::const_iterator my_pos = self_cache.begin() + 1;
          my_pos != my_end;
          ++my_pos )
    {
        ball_pos += ball_vel;

        // out of pitch
        if ( ! pitch_rect.contains( ball_pos ) ) break;

        const rcsc::Vector2D ball_rel = ( ball_pos - *my_pos ).rotatedVector( - accel_angle );
        const double new_ball_dist = ball_rel.r();

        const double ball_travel = ball_pos.dist( wm.ball().pos() );
        const double my_travel = my_pos->dist( wm.self().pos() );

        // check collision
        //double dist_buf = std::min( 0.01 * ball_travel + 0.02 * my_travel,
        //                            0.3 );
        //if ( new_ball_dist < collide_dist - dist_buf + 0.1 ) break;
        //double dist_buf = std::min( 0.02 * ball_travel + 0.03 * my_travel,
        //                            0.3 );
        //if ( new_ball_dist < collide_dist - dist_buf + 0.15 ) break;
        //double dist_buf = std::min( 0.02 * ball_travel + 0.03 * my_travel,
        //                            0.1 );
        //if ( new_ball_dist < collide_dist - dist_buf + 0.15 ) break;
        double dist_buf = std::min( 0.02 * ball_travel + 0.03 * my_travel,
                                    0.1 );
        if ( new_ball_dist < collide_dist - dist_buf + 0.2 ) break;

        // check kickable

        if ( tmp_dash_count == dash_count - 1
             && ball_rel.x > 0.0
             && new_ball_dist > kickable_area - 0.25 ) break;
        //         if ( tmp_dash_count >= dash_count
        //              && ball_rel.x > 0.0
        //              && new_ball_dist > kickable_area - 0.25 ) break;

        //dist_buf = std::min( 0.025 * ball_travel + 0.05 * my_travel,
        //                     0.2 );
        //if ( new_ball_dist > kickable_area + dist_buf ) break;
        //if ( new_ball_dist > kickable_area - 0.2 * std::pow( 0.8, tmp_dash_count ) ) break;
        if ( new_ball_dist > kickable_area - 0.2 ) break;

        // front x buffer
        dist_buf = std::min( 0.02 * ball_travel + 0.04 * my_travel,
                             0.2 );
        if ( ball_rel.x > kickable_area - dist_buf - 0.2 ) break;

        // side y buffer
        //dist_buf = std::min( 0.02 * ball_travel + 0.04 + my_travel,
        //                     0.2 );
        //if ( ball_rel.absY() > kickable_area - dist_buf - 0.25 ) break;
        dist_buf = std::min( 0.02 * ball_travel + 0.055 + my_travel,
                             0.35 );
        if ( ball_rel.absY() > kickable_area - dist_buf - 0.15 ) break;

        // check opponent kickable possibility
        if ( existKickableOpponent( agent , ball_pos, &min_opp_dist ) )
        {
            break;
        }

        total_ball_move = ball_pos - wm.ball().pos();
        ++tmp_dash_count;
        last_ball_rel = ball_rel;
        ball_vel *= param.ballDecay();
    }

    if ( tmp_dash_count > 0 )
    {
        dribble_info->first_ball_vel_ = first_ball_vel;
        dribble_info->last_ball_rel_ = last_ball_rel;
        dribble_info->ball_forward_travel_ = total_ball_move.rotate( - accel_angle ).x;
        dribble_info->dash_count_ = tmp_dash_count;
        dribble_info->min_opp_dist_ = min_opp_dist;
    }

    return ( tmp_dash_count > 0 );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::existKickableOpponent( rcsc::PlayerAgent * agent,
                                         const rcsc::Vector2D & ball_pos,
                                         double * min_opp_dist ) const
{
	
    const rcsc::WorldModel & wm = agent->world();

    static const double kickable_area
        = ServerParam::i().defaultKickableArea() + 0.2;

    const PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin();
          it != end;
          ++it )
    {
		if( !(*it) ) continue;
        if ( (*it)->posCount() > 5 )
        {
            continue;
        }

        if ( (*it)->distFromSelf() > 30.0 )
        {
            break;
        }

        // goalie's catchable check
        if ( (*it)->goalie() )
        {
            if ( ball_pos.x > ServerParam::i().theirPenaltyAreaLineX()
                 && ball_pos.absY() < ServerParam::i().penaltyAreaHalfWidth() )
            {
                double d = (*it)->pos().dist( ball_pos );
                if ( d < rcsc::ServerParam::i().catchableArea() )
                {
                    return true;
                }

                d -= rcsc::ServerParam::i().catchableArea();
                if ( *min_opp_dist > d )
                {
                    *min_opp_dist = d;
                }
            }
        }

        // normal kickable check
        double d = (*it)->pos().dist( ball_pos );
        if ( d < kickable_area )
        {
            return true;
        }

        if ( *min_opp_dist > d )
        {
            *min_opp_dist = d;
        }
    }

    return false;
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doDodge( rcsc::PlayerAgent * agent,
                           const rcsc::Vector2D & target_point )
{
    const rcsc::WorldModel & wm = agent->world();

    const double new_target_dist = 6.0;

    rcsc::AngleDeg avoid_angle
        = getAvoidAngle( agent,
                         ( target_point - wm.self().pos() ).th() );

    const rcsc::Vector2D new_target_rel
        = rcsc::Vector2D::polar2vector( new_target_dist, avoid_angle );
    rcsc::Vector2D new_target
        = wm.self().pos()
        + new_target_rel;

    const rcsc::PlayerPtrCont & opponents = wm.opponentsFromSelf();
    {
        const rcsc::PlayerPtrCont::const_iterator end = opponents.end();
        for ( rcsc::PlayerPtrCont::const_iterator it = opponents.begin();
              it != end;
              ++it )
        {
			if( !(*it) ) continue;
            if ( (*it)->posCount() >= 5 ) break;
            if ( (*it)->isGhost() ) break;
            if ( (*it)->distFromSelf() > 3.0 ) break;

            if ( ( (*it)->angleFromSelf() - avoid_angle ).abs() > 90.0 )
            {
                continue;
            }

            if ( (*it)->distFromSelf()
                 < ServerParam::i().defaultKickableArea() + 0.3 )
            {
                return doAvoidKick( agent, avoid_angle );
            }
        }
    }

    double min_opp_dist = ( opponents.empty()
                            ? 100.0
                            : opponents.front()->distFromSelf() );

    double dir_diff_abs = ( avoid_angle - wm.self().body() ).abs();
    double avoid_dash_power;
    if ( min_opp_dist > 3.0
         || dir_diff_abs < 120.0
         || agent->world().self().stamina() < ServerParam::i().staminaMax() * 0.5
         )
    {
        avoid_dash_power
            = agent->world().self().getSafetyDashPower( ServerParam::i().maxDashPower() );
    }
    else
    {
        // backward
        avoid_dash_power
            = agent->world().self().getSafetyDashPower( ServerParam::i().minDashPower() );

    }

    const double pitch_buffer = 1.0;
    if ( new_target.absX() > ServerParam::i().pitchHalfLength() - pitch_buffer )
    {
        double diff
            = new_target.absX()
            - (ServerParam::i().pitchHalfLength() - pitch_buffer);
        double rate = 1.0 - diff / new_target_rel.absX();
        new_target
            = wm.self().pos()
            + rcsc::Vector2D::polar2vector( new_target_dist * rate,
                                      avoid_angle );
    }
    if ( new_target.absY() > ServerParam::i().pitchHalfWidth() - pitch_buffer )
    {
        double diff
            = new_target.absY()
            - (ServerParam::i().pitchHalfWidth() - pitch_buffer);
        double rate = 1.0 - diff / new_target_rel.absY();
        new_target
            = wm.self().pos()
            + rcsc::Vector2D::polar2vector( new_target_dist * rate,
                                      avoid_angle );
    }

    int n_dash = 2;

    if ( avoid_dash_power > 0.0
         && wm.self().pos().x > -20.0
         && new_target.absY() > 15.0 )
    {
        double dist_to_target = wm.self().pos().dist( new_target );
        n_dash = wm.self().playerType().cyclesToReachDistance( dist_to_target );
        n_dash = std::min( 3, n_dash );

    }

    return doAction( agent, new_target, avoid_dash_power,
                     n_dash, false, true ); // no dodge & dodge_mode flag
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::doAvoidKick( rcsc::PlayerAgent * agent,
                               const rcsc::AngleDeg & avoid_angle )
{
    const rcsc::WorldModel & wm = agent->world();

    const double ball_move_radius = 2.0;

    const rcsc::Vector2D target_rel_point
        = rcsc::Vector2D::polar2vector( ball_move_radius, avoid_angle );

    // my max turnable moment with current speed
    const double next_turnable
        = ServerParam::i().maxMoment()
        / ( 1.0
            + wm.self().playerType().inertiaMoment()
            * (wm.self().vel().r()
               * wm.self().playerType().playerDecay()) );
    // my inernia move vector
    const rcsc::Vector2D my_final_rel_pos
        = wm.self().vel()
        / ( 1.0 - wm.self().playerType().playerDecay() );

    rcsc::AngleDeg target_angle = (target_rel_point - my_final_rel_pos).th();
    double dir_diff_abs = (target_angle - wm.self().body()).abs();
    double dir_margin_abs
        = std::max( 12.0,
                    std::fabs( AngleDeg::atan2_deg( wm.self().playerType().kickableArea() * 0.8,
                                                    ball_move_radius ) ) );

    double ball_first_speed;
    // kick -> dash -> dash -> dash -> ...
    if ( dir_diff_abs < dir_margin_abs
         || dir_diff_abs > 180.0 - dir_margin_abs ) // backward dash
    {
        ball_first_speed = 0.7;
    }
    // kick -> turn -> dash -> dash -> ...
    else if ( dir_diff_abs < next_turnable
              || dir_diff_abs > 180.0 - next_turnable )
    {
        ball_first_speed = 0.5;
    }
    // kick -> turn -> turn -> dash -> ...
    else
    {
        ball_first_speed = 0.3;
    }

    rcsc::Vector2D required_first_vel
        = rcsc::Vector2D::polar2vector( ball_first_speed,
                                  ( target_rel_point - wm.ball().rpos() ).th() );
    rcsc::Vector2D required_accel = required_first_vel - wm.ball().vel();
    double required_kick_power = required_accel.r() / wm.self().kickRate();

    // over max power
    if ( required_kick_power > ServerParam::i().maxPower() )
    {
        rcsc::Vector2D face_point
            = wm.self().pos()
            + rcsc::Vector2D::polar2vector( 20.0, target_angle );
        return Body_HoldBall( true, face_point ).execute( agent );
    }

    // check collision
    if ( ( wm.ball().rpos() + required_first_vel ).dist( wm.self().vel() )
         < wm.self().playerType().playerSize() + ServerParam::i().ballSize() )
    {
        rcsc::Vector2D face_point
            = wm.self().pos()
            + rcsc::Vector2D::polar2vector(20.0, target_angle);
        return Body_HoldBall( true, face_point ).execute( agent );
    }

    return agent->doKick( required_kick_power,
                          required_accel.th() - wm.self().body() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::isDodgeSituation( const rcsc::PlayerAgent * agent,
                                    const rcsc::Vector2D & target_point )
{
    const rcsc::WorldModel & wm = agent->world();

    //////////////////////////////////////////////////////

    const rcsc::AngleDeg target_angle = (target_point - wm.self().pos()).th();
    // check if opponent on target dir
    const rcsc::Sector2D sector( wm.self().pos(),
                           0.6,
                           std::min( 4, M_dash_count )
                           * wm.self().playerType().realSpeedMax(),
                           target_angle - 20.0, target_angle + 20.0 );
    const double base_safety_dir_diff = 60.0;
    double dodge_consider_dist
        = ( ServerParam::i().defaultPlayerSpeedMax() * M_dash_count * 1.5 );
    //     double dodge_consider_dist
    //         = ( ServerParam::i().defaultPlayerSpeedMax() * M_dash_count * 2.0 )
    //         + 4.0;

    if ( dodge_consider_dist > 10.0 ) dodge_consider_dist = 10.0;


    const rcsc::PlayerPtrCont::const_iterator end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator it = wm.opponentsFromSelf().begin();
          it != end;
          ++it )
    {
		if( !(*it) ) continue;
        if ( (*it)->posCount() >= 10 ) continue;
        if ( (*it)->isGhost() ) continue;

        if ( sector.contains( (*it)->pos() ) )
        {
            return true;
        }

        const double dir_diff = ( (*it)->angleFromSelf() - target_angle ).abs();
        double add_buf = 0.0;
        if ( (*it)->distFromSelf() < dodge_consider_dist
             && (*it)->distFromSelf() > 3.0 )
        {
            add_buf = 30.0 / (*it)->distFromSelf();
        }

        if ( (*it)->distFromSelf() < 1.0
             || ( (*it)->distFromSelf() < 1.5 && dir_diff < 120.0 )
             || ( (*it)->distFromSelf() < dodge_consider_dist
                  && dir_diff < base_safety_dir_diff + add_buf ) )
        {
            return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::canKickAfterDash( const rcsc::PlayerAgent * agent,
                                    double * dash_power )
{
    static const rcsc::Rect2D penalty_area( rcsc::Vector2D( ServerParam::i().theirPenaltyAreaLineX(),
                                                - ServerParam::i().penaltyAreaHalfWidth() ),
                                      Size2D( ServerParam::i().penaltyAreaLength(),
                                              ServerParam::i().penaltyAreaWidth() ) );


    const rcsc::WorldModel & wm = agent->world();

    if ( wm.interceptTable()->opponentReachCycle() <= 1 )
    {
        return false;
    }

    const rcsc::Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
    if ( ball_next.absX() > ServerParam::i().pitchHalfLength() - 0.2
         || ball_next.absY() > ServerParam::i().pitchHalfWidth() - 0.2 )
    {
        return false;
    }

    // check dash possibility
    {
        rcsc::AngleDeg accel_angle = ( *dash_power < 0.0
                                 ? wm.self().body() - 180.0
                                 : wm.self().body() );
        rcsc::Vector2D my_pos = wm.self().pos();
        rcsc::Vector2D my_vel = wm.self().vel();

        const double max_accel_mag = ( std::fabs( *dash_power )
                                       * wm.self().playerType().dashPowerRate()
                                       * wm.self().effort() );
        double accel_mag = max_accel_mag;
        if ( wm.self().playerType().normalizeAccel( wm.self().vel(),
                                                    accel_angle,
                                                    &accel_mag ) )
        {
            *dash_power *= accel_mag / max_accel_mag;
        }

        rcsc::Vector2D dash_accel = rcsc::Vector2D::polar2vector( accel_mag, accel_angle );

        my_vel += dash_accel;
        my_pos += my_vel;

        double ball_dist = my_pos.dist( ball_next );
        double noise_buf
            = my_vel.r() * ServerParam::i().playerRand() * 0.5
            + wm.ball().vel().r() * ServerParam::i().ballRand() * 0.5;

        if ( ( ( ball_next - my_pos ).th() - accel_angle ).abs() < 150.0
             && ball_dist < wm.self().playerType().kickableArea() - noise_buf - 0.2
             && ( ball_dist - noise_buf > ( wm.self().playerType().playerSize()
                                            + ServerParam::i().ballSize() ) ) )
        {

        }
        else
        {
            return false;
        }
    }

#if 1
    {
        const rcsc::PlayerPtrCont::const_iterator o_end = wm.opponentsFromBall().end();
        for ( rcsc::PlayerPtrCont::const_iterator o = wm.opponentsFromBall().begin();
              o != o_end;
              ++o )
        {
			if( !(*o) ) continue;
            if ( (*o)->distFromSelf() > 8.0 ) break;
            if ( (*o)->posCount() > 5 ) continue;
            if ( (*o)->isGhost() ) continue;
            if ( (*o)->isTackling() ) continue;

            const rcsc::PlayerType * player_type = (*o)->playerTypePtr();
            const double control_area = ( ( (*o)->goalie()
                                            && penalty_area.contains( (*o)->pos() )
                                            && penalty_area.contains( ball_next ) )
                                          ? rcsc::ServerParam::i().catchableArea()
                                          : player_type->kickableArea() );
            const rcsc::Vector2D opp_next = (*o)->pos() + (*o)->vel();
            const rcsc::AngleDeg opp_body =  ( (*o)->bodyCount() <= 1
                                         ? (*o)->body()
                                         : ( ball_next - opp_next ).th() );
            const rcsc::Vector2D opp_2_ball = ( ball_next - opp_next ).rotatedVector( - opp_body );

            double tackle_dist = ( opp_2_ball.x > 0.0
                                   ? ServerParam::i().tackleDist()
                                   : ServerParam::i().tackleBackDist() );
            if ( tackle_dist > 1.0e-5 )
            {
                double tackle_prob = ( std::pow( opp_2_ball.absX() / tackle_dist,
                                                 ServerParam::i().tackleExponent() )
                                       + std::pow( opp_2_ball.absY() / ServerParam::i().tackleWidth(),
                                                   ServerParam::i().tackleExponent() ) );
                if ( tackle_prob < 1.0
                     && 1.0 - tackle_prob > 0.6 ) // success probability
                {
                    return false;
                }
            }

            const double max_accel = ( ServerParam::i().maxDashPower()
                                       * player_type->dashPowerRate()
                                       * player_type->effortMax() );

            if ( opp_2_ball.absY() < control_area + 0.1
                 && ( opp_2_ball.absX() < max_accel
                      || ( opp_2_ball + Vector2D( max_accel, 0.0 ) ).r() < control_area + 0.1
                      || ( opp_2_ball - Vector2D( max_accel, 0.0 ) ).r() < control_area + 0.1 )
                 )
            {
                return false;
            }
            else if ( opp_2_ball.absY() < ServerParam::i().tackleWidth()
                      && opp_2_ball.x > 0.0
                      && opp_2_ball.x - max_accel < ServerParam::i().tackleDist() + 0.1 )
            {
                return false;
            }

        }
#else
        const rcsc::PlayerObject * opp = wm.getOpponentNearestToSelf( 5 );
        if ( opp
             && ( opp->pos().dist( ball_next )
                  < ( ServerParam::i().defaultKickableArea()
                      + ServerParam::i().defaultPlayerSpeedMax()
                      + 0.3 ) )
             )
        {
            return false;
        }
#endif
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_Dribble::existCloseOpponent( const rcsc::PlayerAgent * agent,
                                      rcsc::AngleDeg * keep_angle )
{
    const rcsc::WorldModel & wm = agent->world();

    const rcsc::PlayerObject * opp = wm.getOpponentNearestToBall( 5 );
    if ( ! opp )
    {
        return false;
    }

    if ( opp->distFromBall()
         > ( ServerParam::i().defaultPlayerSpeedMax()
             + ServerParam::i().tackleDist() )
         )
    {
        return false;
    }


    // opp is in dangerous range

    rcsc::Vector2D my_next = wm.self().pos() + wm.self().vel();
    rcsc::Vector2D opp_next = opp->pos() + opp->vel();

    if ( opp->bodyCount() == 0
         || ( opp->velCount() <= 1 && opp->vel().r() > 0.2 ) )
    {
        rcsc::Line2D opp_line( opp_next,
                         ( opp->bodyCount() == 0
                           ? opp->body()
                           : opp->vel().th() ) );
        rcsc::Vector2D proj_pos = opp_line.projection( my_next );

        *keep_angle = ( my_next - proj_pos ).th();

        return true;
    }

    *keep_angle = ( my_next - opp_next ).th();

    return true;
}

/*-------------------------------------------------------------------*/
/*!
  TODO: angles which agent can turn by 1 step should have the priority
*/
AngleDeg
Body_Dribble::getAvoidAngle( const rcsc::PlayerAgent * agent,
                                 const rcsc::AngleDeg & target_angle )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.opponentsFromSelf().empty() )
    {
        return target_angle;
    }

    const double avoid_radius = 4.4; //5.0; 2008-04-30
    const double safety_opp_dist = 5.0;
    const double safety_space_body_ang_radius2 = 3.0 * 3.0;


    const rcsc::PlayerPtrCont & opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();

    // at first, check my body dir and opposite dir of my body
    if ( ! opps.empty()
         && opps.front()->distFromSelf() < 3.0 )
    {
        rcsc::AngleDeg new_target_angle = wm.self().body();
        for ( int i = 0; i < 2; i++ )
        {
            const rcsc::Vector2D sub_target
                = wm.self().pos()
                + rcsc::Vector2D::polar2vector( avoid_radius,
                                          new_target_angle );

            if ( sub_target.absX() > ServerParam::i().pitchHalfLength() - 1.8
                 || sub_target.absY() > ServerParam::i().pitchHalfWidth() - 1.8 )
            {
                // out of pitch
                continue;
            }

            bool success = true;
            for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin();
                  it != opps_end;
                  ++it )
            {
				if( !(*it) ) continue;
                if ( (*it)->posCount() >= 10 ) continue;

                if ( (*it)->distFromSelf() > 20.0 )
                {
                    break;
                }

                if ( (*it)->distFromSelf() < safety_opp_dist
                     && ((*it)->angleFromSelf() - new_target_angle).abs() < 30.0 )
                {
                    success = false;
                    break;
                }

                if ( sub_target.dist2( (*it)->pos() )
                     < safety_space_body_ang_radius2 )
                {
                    success = false;
                    break;
                }
            }

            if ( success )
            {
                return new_target_angle;
            }

            new_target_angle -= 180.0;
        }
    }

    // search divisions

    const int search_divs = 10;
    const double div_dir = 360.0 / static_cast< double >( search_divs );
    const double safety_angle = 60.0;
    const double safety_space_radius2 = avoid_radius * avoid_radius;

    double angle_sign = 1.0;
    if ( agent->world().self().pos().y < 0.0 ) angle_sign = -1.0;

    for ( int i = 1; i < search_divs; ++i, angle_sign *= -1.0 )
    {
        const rcsc::AngleDeg new_target_angle
            = target_angle
            + ( angle_sign * div_dir * ( (i+1)/2 ) );

        const rcsc::Vector2D sub_target
            = wm.self().pos()
            + rcsc::Vector2D::polar2vector( avoid_radius,
                                      new_target_angle );

        if ( sub_target.absX()
             > ServerParam::i().pitchHalfLength() - wm.self().playerType().kickableArea() - 0.2
             || sub_target.absY()
             > ServerParam::i().pitchHalfWidth() - wm.self().playerType().kickableArea() - 0.2 )
        {
            // out of pitch
            continue;
        }

        if ( sub_target.x < 30.0
             && sub_target.x < wm.self().pos().x - 2.0 )
        {
            continue;
        }

        bool success = true;
        for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin();
              it != opps_end;
              ++it )
        {
			if( !(*it) ) continue;
            if ( (*it)->posCount() >= 10 ) continue;

            if ( (*it)->distFromSelf() > 20.0 ) break;

            double add_dir = 5.8 / (*it)->distFromSelf();
            add_dir = std::min( 180.0 - safety_angle, add_dir );
            if ( (*it)->distFromSelf() < safety_opp_dist
                 && ( ( (*it)->angleFromSelf() - new_target_angle ).abs()
                      < safety_angle + add_dir ) )
            {
                success = false;
                break;
            }

            if ( sub_target.dist2( (*it)->pos() ) < safety_space_radius2 )
            {
                success = false;
                break;
            }
        }

        if ( success )
        {
            return new_target_angle;
        }

    }


    // Best angle is not found.
    // go to the least congestion point

    rcsc::Rect2D target_rect( Vector2D( wm.self().pos().x - 4.0,
                                  wm.self().pos().y - 4.0 ),
                        Size2D( 8.0, 8.0 ) );

    double best_score = 10000.0;
    rcsc::Vector2D best_target = wm.self().pos();

    double x_i = 30.0 - wm.self().pos().x;
    if ( x_i > 0.0 ) x_i = 0.0;
    if ( x_i < -8.0 ) x_i = -8.0;

    for ( ; x_i < 8.5; x_i += 1.0 )
    {
        for ( double y_i = - 8.0; y_i < 8.5; y_i += 1.0 )
        {
            rcsc::Vector2D candidate = wm.self().pos();
            candidate.add( x_i, y_i );

            if ( candidate.absX() > ServerParam::i().pitchHalfLength() - 2.0
                 || candidate.absY() > ServerParam::i().pitchHalfWidth() - 2.0 )
            {
                continue;
            }

            double tmp_score = 0.0;
            for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin();
                  it != opps_end;
                  ++it )
            {
				if( !(*it) ) continue;
                double d2 = (*it)->pos().dist2( candidate );

                if ( d2 > 15.0 * 15.0 ) continue;

                tmp_score += 1.0 / d2;
            }

            if ( tmp_score < best_score )
            {
                best_target = candidate;
                best_score = tmp_score;
            }
        }
    }

    return ( best_target - wm.self().pos() ).th();
}

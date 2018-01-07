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

#include "body_intercept.h"
#include "body_go_to_point.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/common/server_param.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>
#include <rcsc/player/say_message_builder.h>

#define USE_GOALIE_MODE

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Intercept::execute( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    
    //agent->addSayMessage( new rcsc::InterceptMessage( true , wm.self().unum() , wm.time().cycle() ) );

    /////////////////////////////////////////////
    if ( doKickableOpponentCheck( agent ) )
    {
		//agent->addSayMessage( new rcsc::InterceptMessage( true , wm.self().unum() , wm.time().cycle() ) );
        return true;
    }

    const rcsc::InterceptTable * table = wm.interceptTable();

    /////////////////////////////////////////////
    if ( table->selfReachCycle() > 100 )
    {
        rcsc::Vector2D final_point = wm.ball().inertiaFinalPoint();
        Body_GoToPoint( final_point,
                        2.0,
                        sp.maxDashPower()
                        ).execute( agent );
		//agent->addSayMessage( new rcsc::InterceptMessage( true , wm.self().unum() , wm.time().cycle() ) );
        return true;
    }

    /////////////////////////////////////////////
    rcsc::InterceptInfo best_intercept = getBestIntercept( wm, table );
    //InterceptInfo best_intercept_test = getBestIntercept( wm, table );

    rcsc::Vector2D target_point = wm.ball().inertiaPoint( best_intercept.reachCycle() );
    
    if ( best_intercept.dashCycle() == 0 )
    {
        rcsc::Vector2D face_point = M_face_point;
        if ( ! face_point.isValid() )
        {
            face_point.assign( 50.5, wm.self().pos().y * 0.75 );
        }

        rcsc::Body_TurnToPoint( face_point,
                          best_intercept.reachCycle() ).execute( agent );
        //agent->addSayMessage( new rcsc::InterceptMessage( true , wm.self().unum() , wm.time().cycle() ) );
        return true;
    }

    /////////////////////////////////////////////
    if ( best_intercept.turnCycle() > 0 )
    {
        rcsc::Vector2D my_inertia = wm.self().inertiaPoint( best_intercept.reachCycle() );
        rcsc::AngleDeg target_angle = ( target_point - my_inertia ).th();
        if ( best_intercept.dashPower() < 0.0 )
        {
            // back dash
            target_angle -= 180.0;
        }

        return agent->doTurn( target_angle - wm.self().body() );
    }

    /////////////////////////////////////////////

    if ( doWaitTurn( agent, target_point, best_intercept ) )
    {
        return true;
    }

    if ( M_save_recovery
         && ! wm.self().staminaModel().capacityIsEmpty() )
    {
        double consumed_stamina = best_intercept.dashPower();
        if ( best_intercept.dashPower() < 0.0 ) consumed_stamina *= -2.0;

        if ( wm.self().stamina() - consumed_stamina
             < sp.recoverDecThrValue() + 1.0 )
        {
            agent->doTurn( 0.0 );
            return false;
        }

    }

    return doInertiaDash( agent,
                          target_point,
                          best_intercept );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Intercept::doKickableOpponentCheck( rcsc::PlayerAgent * agent )
{
	
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::ServerParam & sp = rcsc::ServerParam::i();
    
    if ( wm.ball().distFromSelf() < 2.0
         && wm.existKickableOpponent() )
    {
        const rcsc::PlayerObject * opp = wm.opponentsFromBall().front();
        if ( opp )
        {
            rcsc::Vector2D goal_pos( -sp.pitchHalfLength(), 0.0 );
            rcsc::Vector2D my_next = wm.self().pos() + wm.self().vel();
            rcsc::Vector2D attack_pos = opp->pos() + opp->vel();

            if ( attack_pos.dist2( goal_pos ) > my_next.dist2( goal_pos ) )
            {
                Body_GoToPoint( attack_pos,
                                0.1,
                                sp.maxDashPower(),
                                -1.0, // dash speed
                                1, // cycle
                                true, // save recovery
                                15.0  // dir thr
                                ).execute( agent );
                return true;
            }
        }
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::InterceptInfo
Body_Intercept::getBestIntercept( const rcsc::WorldModel & wm,
                                      const rcsc::InterceptTable * table ) const
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const std::vector< rcsc::InterceptInfo > & cache = table->selfCache();

    if ( cache.empty() )
    {
        return rcsc::InterceptInfo();
    }

    const rcsc::Vector2D goal_pos( 65.0, 0.0 );
    const rcsc::Vector2D our_goal_pos( -SP.pitchHalfLength(), 0.0 );
    const double max_pitch_x = ( SP.keepawayMode()
                                 ? SP.keepawayLength() * 0.5 - 1.0
                                 : SP.pitchHalfLength() - 1.0 );
    const double max_pitch_y = ( SP.keepawayMode()
                                 ? SP.keepawayWidth() * 0.5 - 1.0
                                 : SP.pitchHalfWidth() - 1.0 );
    const double penalty_x = SP.ourPenaltyAreaLineX();
    const double penalty_y = SP.penaltyAreaHalfWidth();
    const double speed_max = wm.self().playerType().realSpeedMax() * 0.9;
    const int opp_min = table->opponentReachCycle();
    const int mate_min = table->teammateReachCycle();
    //const PlayerObject * fastest_opponent = table->fastestOpponent();

    const rcsc::InterceptInfo * attacker_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double attacker_score = 0.0;

    const rcsc::InterceptInfo * forward_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double forward_score = 0.0;

    const rcsc::InterceptInfo * noturn_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double noturn_score = 10000.0;

    const rcsc::InterceptInfo * nearest_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double nearest_score = 10000.0;

#ifdef USE_GOALIE_MODE
    const rcsc::InterceptInfo * goalie_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double goalie_score = -10000.0;

    const rcsc::InterceptInfo * goalie_aggressive_best = static_cast< rcsc::InterceptInfo * >( 0 );
    double goalie_aggressive_score = -10000.0;
#endif

    const std::size_t MAX = cache.size();
    for ( std::size_t i = 0; i < MAX; ++i )
    {
        if ( M_save_recovery
             && cache[i].mode() != rcsc::InterceptInfo::NORMAL )
        {
            continue;
        }

        const int cycle = cache[i].reachCycle();
        const rcsc::Vector2D self_pos = wm.self().inertiaPoint( cycle );
        const rcsc::Vector2D ball_pos = wm.ball().inertiaPoint( cycle );
        const rcsc::Vector2D ball_vel = wm.ball().vel() * std::pow( SP.ballDecay(), cycle );

        if ( ball_pos.absX() > max_pitch_x
             || ball_pos.absY() > max_pitch_y )
        {
            continue;
        }

#ifdef USE_GOALIE_MODE
        if ( wm.self().goalie()
             && wm.lastKickerSide() != wm.ourSide()
             && ball_pos.x < penalty_x - 1.0
             && ball_pos.absY() < penalty_y - 1.0
             && cycle < opp_min - 1 )
        {
            if ( ( cache[i].turnCycle() == 0
                   && cache[i].ballDist() < SP.catchableArea() * 0.5 )
                 || cache[i].ballDist() < 0.01 )
            {
                double d = ball_pos.dist2( our_goal_pos );
                if ( d> goalie_score )
                {
                    goalie_score = d;
                    goalie_best = &cache[i];
                }
            }
        }

        if ( wm.self().goalie()
             && wm.lastKickerSide() != wm.ourSide()
             && cycle < mate_min - 3
             && cycle < opp_min - 5
             && ( ball_pos.x > penalty_x - 1.0
                  || ball_pos.absY() > penalty_y - 1.0 ) )
        {
            if ( ( cache[i].turnCycle() == 0
                   && cache[i].ballDist() < wm.self().playerType().kickableArea() * 0.5 )
                 || cache[i].ballDist() < 0.01 )
            {
                if ( ball_pos.x > goalie_aggressive_score )
                {
                    goalie_aggressive_score = ball_pos.x;
                    goalie_aggressive_best = &cache[i];
                }
            }
        }
#endif

        bool attacker = false;
        if ( ball_vel.x > 0.5
             && ball_vel.r2() > std::pow( speed_max, 2 )
             && cache[i].dashPower() >= 0.0
             && ball_pos.x < 47.0
             //&& std::fabs( ball_pos.y - wm.self().pos().y ) < 10.0
             && ( ball_pos.x > 35.0
                  || ball_pos.x > wm.offsideLineX() )
             )
        {
            attacker = true;
        }

        const double opp_rate = ( attacker ? 0.95 : 0.7 );
#if 0
        if ( attacker
             && opp_min <= cycle - 5
             && ball_vel.r2() > std::pow( 1.2, 2 ) )
        {
        }
        else
#endif
            if ( cycle >= opp_min * opp_rate )
        {
            continue;
        }

        // attacker type

        if ( attacker )
        {
            double goal_dist = 100.0 - std::min( 100.0, ball_pos.dist( goal_pos ) );
            double x_diff = 47.0 - ball_pos.x;

            double score
                = ( goal_dist / 100.0 )
                * std::exp( - ( x_diff * x_diff ) / ( 2.0 * 100.0 ) );
            if ( score > attacker_score )
            {
                attacker_best = &cache[i];
                attacker_score = score;
            }

            continue;
        }

        // no turn type

        if ( cache[i].turnCycle() == 0 )
        {
            //double score = ball_pos.x;
            //double score = wm.self().pos().dist2( ball_pos );
            double score = cycle;
            //if ( ball_vel.x > 0.0 )
            //{
            //    score *= std::exp( - std::pow( ball_vel.r() - 1.0, 2.0 )
            //                       / ( 2.0 * 1.0 ) );
            //}
            if ( score < noturn_score )
            {
                noturn_best = &cache[i];
                noturn_score = score;
            }

            continue;
        }

        // forward type

//         if ( ball_vel.x > 0.5
//              && ball_pos.x > wm.offsideLineX() - 15.0
//              && ball_vel.r() > speed_max * 0.98
//              && cycle <= opp_min - 5 )
        if ( ball_vel.x > 0.1
             && cycle <= opp_min - 5
             && ball_vel.r2() > std::pow( 0.6, 2 ) )
        {
            double score
                = ( 100.0 * 100.0 )
                - std::min( 100.0 * 100.0, ball_pos.dist2( goal_pos ) );
            if ( score > forward_score )
            {
                forward_best = &cache[i];
                forward_score = score;
            }

            continue;
        }

        // other: select nearest one

        {
            //double d = wm.self().pos().dist2( ball_pos );
            double d = self_pos.dist2( ball_pos );
            if ( d < nearest_score )
            {
                nearest_best = &cache[i];
                nearest_score = d;
            }
        }

    }

#ifdef USE_GOALIE_MODE
    if ( goalie_aggressive_best )
    {
        return *goalie_aggressive_best;
    }

    if ( goalie_best )
    {
        return *goalie_best;
    }
#endif

    if ( attacker_best )
    {
        return *attacker_best;
    }

    if ( noturn_best && forward_best )
    {
        //const Vector2D forward_ball_pos = wm.ball().inertiaPoint( forward_best->reachCycle() );
        //const Vector2D forward_ball_vel
        //    = wm.ball().vel()
        //    * std::pow( SP.ballDecay(), forward_best->reachCycle() );

        if ( forward_best->reachCycle() >= 5 )
        {
        }

        const rcsc::Vector2D noturn_ball_vel
            = wm.ball().vel()
            * std::pow( SP.ballDecay(), noturn_best->reachCycle() );
        const double noturn_ball_speed = noturn_ball_vel.r();
        if ( noturn_ball_vel.x > 0.1
             && ( noturn_ball_speed > speed_max
                  || noturn_best->reachCycle() <= forward_best->reachCycle() + 2 )
             )
        {
            return *noturn_best;
        }
    }

    if ( forward_best )
    {
        return *forward_best;
    }

    const rcsc::Vector2D fastest_pos = wm.ball().inertiaPoint( cache[0].reachCycle() );
    const rcsc::Vector2D fastest_vel = wm.ball().vel() * std::pow( SP.ballDecay(),
                                                             cache[0].reachCycle() );
    if ( ( fastest_pos.x > -33.0
           || fastest_pos.absY() > 20.0 )
         && ( cache[0].reachCycle() >= 10
             //|| wm.ball().vel().r() < 1.5 ) )
             || fastest_vel.r() < 1.2 ) )
    {
        return cache[0];
    }

    if ( noturn_best && nearest_best )
    {
        const rcsc::Vector2D noturn_self_pos = wm.self().inertiaPoint( noturn_best->reachCycle() );
        const rcsc::Vector2D noturn_ball_pos = wm.ball().inertiaPoint( noturn_best->reachCycle() );
        const rcsc::Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
        const rcsc::Vector2D nearest_ball_pos = wm.ball().inertiaPoint( nearest_best->reachCycle() );

//         if ( wm.self().pos().dist2( noturn_ball_pos )
//              < wm.self().pos().dist2( nearest_ball_pos ) )
        if ( noturn_self_pos.dist2( noturn_ball_pos )
             < nearest_self_pos.dist2( nearest_ball_pos ) )
        {
            return *noturn_best;
        }

        if ( nearest_best->reachCycle() <= noturn_best->reachCycle() + 2 )
        {
            const rcsc::Vector2D nearest_ball_vel
                = wm.ball().vel()
                * std::pow( SP.ballDecay(), nearest_best->reachCycle() );
            const double nearest_ball_speed = nearest_ball_vel.r();
            if ( nearest_ball_speed < 0.7 )
            {
                return *nearest_best;
            }

            const rcsc::Vector2D noturn_ball_vel
                = wm.ball().vel()
                * std::pow( SP.ballDecay(), noturn_best->reachCycle() );

            if ( nearest_best->ballDist() < wm.self().playerType().kickableArea() - 0.4
                 && nearest_best->ballDist() < noturn_best->ballDist()
                 && noturn_ball_vel.x < 0.5
                 && noturn_ball_vel.r2() > std::pow( 1.0, 2 )
                 && noturn_ball_pos.x > nearest_ball_pos.x )
            {
                return *nearest_best;
            }

            rcsc::Vector2D nearest_self_pos = wm.self().inertiaPoint( nearest_best->reachCycle() );
            if ( nearest_ball_speed > 0.7
                //&& wm.self().pos().dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
                 && nearest_self_pos.dist( nearest_ball_pos ) < wm.self().playerType().kickableArea() )
            {
                return *nearest_best;
            }
        }
        return *noturn_best;
    }

    if ( noturn_best )
    {
        return *noturn_best;
    }

    if ( nearest_best )
    {
        return *nearest_best;
    }

    if ( wm.self().pos().x > 40.0
         && wm.ball().vel().r() > 1.8
         && wm.ball().vel().th().abs() < 100.0
         && cache[0].reachCycle() > 1 )
    {
        const rcsc::InterceptInfo * chance_best = static_cast< rcsc::InterceptInfo * >( 0 );
        for ( std::size_t i = 0; i < MAX; ++i )
        {
            if ( cache[i].reachCycle() <= cache[0].reachCycle() + 3
                 && cache[i].reachCycle() <= opp_min - 2 )
            {
                chance_best = &cache[i];
            }
        }

        if ( chance_best )
        {
            return *chance_best;
        }
    }

    return cache[0];

}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::InterceptInfo
Body_Intercept::getBestIntercept_Test( const rcsc::WorldModel & /*wm*/,
                                           const rcsc::InterceptTable * /*table*/ ) const
{
#if 0
    const std::vector< rcsc::InterceptInfo > & cache = table->selfCache();

    if ( cache.empty() )
    {
        return InterceptInfo();
    }

    const rcscServerParam & SP = rcsc::ServerParam::i();
    const int our_min = table->teammateReachCycle();
    const int opp_min = table->opponentReachCycle();
    const rcsc::PlayerObject * opponent = table->fastestOpponent();

    const std::size_t MAX = cache.size();

    for ( std::size_t i = 0; i < MAX; ++i )
    {
        const rcsc::InterceptInfo & info = cache[i];

        if ( M_save_recovery
             && info.mode() != rcsc::InterceptInfo::NORMAL )
        {
            continue;
        }

        const int reach_cycle = info.reachCycle();
        const rcsc::Vector2D ball_pos = wm.ball().inertiaPoint( reach_cycle );
        const rcsc::Vector2D ball_vel = wm.ball().vel() * std::pow( SP.ballDecay(), reach_cycle );

    }


#if 0
#endif
#endif
    return rcsc::InterceptInfo();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Intercept::doWaitTurn( rcsc::PlayerAgent * agent,
                                const rcsc::Vector2D & target_point,
                                const rcsc::InterceptInfo & info )
{
    const rcsc::WorldModel & wm = agent->world();

    {
        const rcsc::PlayerObject * opp = wm.getOpponentNearestToSelf( 5 );
        if ( opp && opp->distFromSelf() < 3.0 )
        {
            return false;
        }

        int opp_min = wm.interceptTable()->opponentReachCycle();
        if ( info.reachCycle() > opp_min - 5 )
        {
            return false;
        }
    }

    const rcsc::Vector2D my_inertia = wm.self().inertiaPoint( info.reachCycle() );
    const rcsc::Vector2D target_rel = ( target_point - my_inertia ).rotatedVector( - wm.self().body() );
    const double target_dist = target_rel.r();

    const double ball_travel
        = rcsc::inertia_n_step_distance( wm.ball().vel().r(),
                                   info.reachCycle(),
                                   rcsc::ServerParam::i().ballDecay() );
    const double ball_noise = ball_travel * rcsc::ServerParam::i().ballRand();

    if ( info.reachCycle() == 1
         && info.turnCycle() == 1 )
    {
        rcsc::Vector2D face_point = M_face_point;
        if ( ! face_point.isValid() )
        {
            face_point.assign( 50.5, wm.self().pos().y * 0.9 );
        }
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        
        return true;
    }

    double extra_buf = 0.1 * rcsc::bound( 0, info.reachCycle() - 1, 4 );
    {
        double angle_diff = ( wm.ball().vel().th() - wm.self().body() ).abs();
        if ( angle_diff < 10.0
             || 170.0 < angle_diff )
        {
            extra_buf = 0.0;
        }
    }

    double dist_buf = wm.self().playerType().kickableArea() - 0.3 + extra_buf;
    dist_buf -= 0.1 * wm.ball().seenPosCount();

    if ( target_dist > dist_buf )
    {
        return false;
    }

    rcsc::Vector2D face_point = M_face_point;
    if ( info.reachCycle() > 2 )
    {
        face_point = my_inertia
            + ( wm.ball().pos() - my_inertia ).rotatedVector( 90.0 );
        if ( face_point.x < my_inertia.x )
        {
            face_point = my_inertia
                + ( wm.ball().pos() - my_inertia ).rotatedVector( -90.0 );
        }
    }

    if ( ! face_point.isValid() )
    {
        face_point.assign( 50.5, wm.self().pos().y * 0.9 );
    }

    rcsc::Vector2D face_rel = face_point - my_inertia;
    rcsc::AngleDeg face_angle = face_rel.th();

    rcsc::Vector2D faced_rel = target_point - my_inertia;
    faced_rel.rotate( face_angle );
    if ( faced_rel.absY() > wm.self().playerType().kickableArea() - ball_noise - 0.2 )
    {
        return false;
    }

    rcsc::Body_TurnToPoint( face_point ).execute( agent );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Body_Intercept::doInertiaDash( rcsc::PlayerAgent * agent,
                                   const rcsc::Vector2D & target_point,
                                   const rcsc::InterceptInfo & info )
{
    const rcsc::WorldModel & wm = agent->world();
    const rcsc::PlayerType & ptype = wm.self().playerType();

    if ( info.reachCycle() == 1 )
    {
        agent->doDash( info.dashPower(), info.dashAngle() );
        return true;
    }

    rcsc::Vector2D target_rel = target_point - wm.self().pos();
    target_rel.rotate( - wm.self().body() );

    rcsc::AngleDeg accel_angle = wm.self().body();
    if ( info.dashPower() < 0.0 ) accel_angle += 180.0;

    rcsc::Vector2D ball_vel = wm.ball().vel() * std::pow( rcsc::ServerParam::i().ballDecay(),
                                                    info.reachCycle() );

    if ( ( ! wm.self().goalie()
           || wm.lastKickerSide() == wm.ourSide() )
         && wm.self().body().abs() < 50.0 )
    {
        double buf = 0.3;
        if ( info.reachCycle() >= 8 )
        {
            buf = 0.0;
        }
        else if ( target_rel.absY() > wm.self().playerType().kickableArea() - 0.25 )
        {
            buf = 0.0;
        }
        else if ( target_rel.x < 0.0 )
        {
            if ( info.reachCycle() >= 3 ) buf = 0.5;
        }
        else if ( target_rel.x < 0.3 )
        {
            if ( info.reachCycle() >= 3 ) buf = 0.5;
        }
        else if ( target_rel.absY() < 0.5 )
        {
            if ( info.reachCycle() >= 3 ) buf = 0.5;
            if ( info.reachCycle() == 2 ) buf = std::min( target_rel.x, 0.5 );
        }
        else if ( ball_vel.r() < 1.6 )
        {
            buf = 0.4;
        }
        else
        {
            if ( info.reachCycle() >= 4 ) buf = 0.3;
            else if ( info.reachCycle() == 3 ) buf = 0.3;
            else if ( info.reachCycle() == 2 ) buf = std::min( target_rel.x, 0.3 );
        }

        target_rel.x -= buf;
    }

    double used_power = info.dashPower();

    if ( wm.ball().seenPosCount() <= 2
         && wm.ball().vel().r() * std::pow( rcsc::ServerParam::i().ballDecay(), info.reachCycle() ) < ptype.kickableArea() * 1.5
         && info.dashAngle().abs() < 5.0
         && target_rel.absX() < ( ptype.kickableArea()
                                  + ptype.dashRate( wm.self().effort() )
                                  * rcsc::ServerParam::i().maxDashPower()
                                  * 0.8 ) )
    {
        double first_speed
            = rcsc::calc_first_term_geom_series( target_rel.x,
                                           wm.self().playerType().playerDecay(),
                                           info.reachCycle() );

        first_speed = rcsc::min_max( - wm.self().playerType().playerSpeedMax(),
                               first_speed,
                               wm.self().playerType().playerSpeedMax() );
        rcsc::Vector2D rel_vel = wm.self().vel().rotatedVector( - wm.self().body() );
        double required_accel = first_speed - rel_vel.x;
        used_power = required_accel / wm.self().dashRate();
        used_power /= rcsc::ServerParam::i().dashDirRate( info.dashAngle().degree() );

        //if ( info.dashPower() < 0.0 ) used_power = -used_power;

        used_power = rcsc::ServerParam::i().normalizeDashPower( used_power );
        if ( M_save_recovery )
        {
            used_power = wm.self().getSafetyDashPower( used_power );
        }

    }
    else
    {
    }


    if ( info.reachCycle() >= 4
         && ( target_rel.absX() < 0.5
              || std::fabs( used_power ) < 5.0 )
         )
    {

        rcsc::Vector2D my_inertia = wm.self().inertiaPoint( info.reachCycle() );
        rcsc::Vector2D face_point = M_face_point;
        if ( ! M_face_point.isValid() )
        {
            face_point.assign( 50.5, wm.self().pos().y * 0.75 );
        }
        rcsc::AngleDeg face_angle = ( face_point - my_inertia ).th();

        rcsc::Vector2D ball_next = wm.ball().pos() + wm.ball().vel();
        rcsc::AngleDeg ball_angle = ( ball_next - my_inertia ).th();
        double normal_half_width = rcsc::ViewWidth::width( rcsc::ViewWidth::NORMAL );

        if ( ( ball_angle - face_angle ).abs()
             > ( rcsc::ServerParam::i().maxNeckAngle()
                 + normal_half_width
                 - 10.0 )
             )
        {
            face_point.x = my_inertia.x;
            if ( ball_next.y > my_inertia.y + 1.0 ) face_point.y = 50.0;
            else if ( ball_next.y < my_inertia.y - 1.0 ) face_point.y = -50.0;
            else  face_point = ball_next;
            
        }
        else
        {
        }
        rcsc::Body_TurnToPoint( face_point ).execute( agent );
        return true;
    }

    agent->doDash( used_power, info.dashAngle() );
    return true;
}

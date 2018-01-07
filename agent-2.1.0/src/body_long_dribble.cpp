// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

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

#include "body_long_dribble.h"
#include "bhv_predictor.h"
#include "body_pass.h"
#include "body_smart_kick.h"
#include "strategy.h"

#include <rcsc/action/kick_table.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/player/world_model.h>
#include <rcsc/common/server_param.h>
#include <rcsc/timer.h>

#include <limits>
#include <cmath>

/*!

 */
bool
Body_LongDribble::generate( const rcsc::WorldModel & wm )
{
    if ( M_update_time == wm.time() )
    {
        return false;
    }
    M_update_time = wm.time();

    if ( wm.gameMode().type() != rcsc::GameMode::PlayOn
         && ! wm.gameMode().isPenaltyKickMode() )
    {
        return false;
    }

    if ( ! wm.self().isKickable()
         || wm.self().isFrozen() )
    {
        return false;
    }

    createCourses( wm );

}

/*-------------------------------------------------------------------*/
/*!

 */
void
Body_LongDribble::createCourses( const rcsc::WorldModel & wm )
{
    static const int ANGLE_DIVS = 60;
    static const double ANGLE_STEP = 360.0 / ANGLE_DIVS;

    static std::vector< rcsc::Vector2D > self_cache( 24 );

    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    const rcsc::Vector2D ball_pos = wm.ball().pos();
    const rcsc::AngleDeg body_angle = wm.self().body();

    const int min_dash = 5;
    const int max_dash = ( ball_pos.x < -20.0 ? 6
                           : ball_pos.x < 0.0 ? 7
                           : ball_pos.x < 10.0 ? 13
                           : ball_pos.x < 20.0 ? 15
                           : 20 );

    const rcsc::PlayerType & ptype = wm.self().playerType();

    const double max_effective_turn
        = ptype.effectiveTurn( SP.maxMoment(),
                               wm.self().vel().r() * ptype.playerDecay() );

    const rcsc::Vector2D our_goal = rcsc::ServerParam::i().ourTeamGoalPos();
    const double goal_dist_thr2 = std::pow( 18.0, 2 ); // Magic Number

    for ( int a = 0; a < ANGLE_DIVS; ++a )
    {
        const double add_angle = ANGLE_STEP * a;

        int n_turn = 0;
        if ( a != 0 )
        {
            if ( rcsc::AngleDeg( add_angle ).abs() > max_effective_turn )
            {
                continue;
            }

            n_turn = 1;
        }

        const rcsc::AngleDeg dash_angle = body_angle + add_angle;

        if ( ball_pos.x < SP.theirPenaltyAreaLineX() + 5.0
             && dash_angle.abs() > 85.0 ) // Magic Number
        {
            continue;
        }

        createSelfCache( wm, dash_angle,
                         n_turn, max_dash,
                         self_cache );

        int n_dash = self_cache.size() - n_turn;

        if ( n_dash < min_dash )
        {
            continue;
        }

        int count = 0;
        int dash_dec = 2;
        for ( ; n_dash >= min_dash; n_dash -= dash_dec )
        {
            ++M_total_count;

            if ( n_dash <= 10 )
            {
                dash_dec = 1;
            }

            const rcsc::Vector2D receive_pos = self_cache[n_turn + n_dash - 1];

            if ( receive_pos.dist2( our_goal ) < goal_dist_thr2 )
            {
                continue;
            }

            if ( ! canKick( wm, n_turn, n_dash, receive_pos ) )
            {
                continue;
            }

            if ( ! checkOpponent( wm, n_turn, n_dash, ball_pos, receive_pos ) )
            {
                continue;
            }

            double first_speed = SP.firstBallSpeed( ball_pos.dist( receive_pos ),
                                                    1 + n_turn + n_dash );

            //M_courses.push_back( ptr );

            ++count;
            if ( count >= 10 )
            {
                break;
            }
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
Body_LongDribble::createSelfCache( const rcsc::WorldModel & wm,
                                    const rcsc::AngleDeg & dash_angle,
                                    const int n_turn,
                                    const int n_dash,
                                    std::vector< rcsc::Vector2D > & self_cache )
{
    self_cache.clear();

    const rcsc::PlayerType & ptype = wm.self().playerType();

    const rcsc::Vector2D first_ball_pos = wm.ball().pos();

    const double dash_power = rcsc::ServerParam::i().maxDashPower();
    const double stamina_thr = ( wm.self().staminaModel().capacityIsEmpty()
                                 ? -ptype.extraStamina() // minus value to set available stamina
                                 : rcsc::ServerParam::i().recoverDecThrValue() + 350.0 );

    rcsc::StaminaModel stamina_model = wm.self().staminaModel();

    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D my_vel = wm.self().vel();

    //
    // 1 kick
    //
    my_pos += my_vel;
    my_vel *= ptype.playerDecay();
    stamina_model.simulateWait( ptype );

    //
    // turns
    //
    for ( int i = 0; i < n_turn; ++i )
    {
        my_pos += my_vel;
        my_vel *= ptype.playerDecay();
        stamina_model.simulateWait( ptype );

        self_cache.push_back( my_pos );
    }

    //
    // simulate dashes
    //
    for ( int i = 0; i < n_dash; ++i )
    {
        if ( stamina_model.stamina() < stamina_thr )
        {
            break;
        }

        double available_stamina =  std::max( 0.0, stamina_model.stamina() - stamina_thr );
        double actual_dash_power = std::min( dash_power, available_stamina );
        double accel_mag = actual_dash_power * ptype.dashPowerRate() * stamina_model.effort();
        rcsc::Vector2D dash_accel = rcsc::Vector2D::polar2vector( accel_mag, dash_angle );

        // TODO: check playerSpeedMax & update actual_dash_power if necessary
        // if ( ptype.normalizeAccel( my_vel, &dash_accel ) ) actual_dash_power = ...

        my_vel += dash_accel;
        my_pos += my_vel;

        if ( my_pos.x > rcsc::ServerParam::i().pitchHalfLength() - 2.5 )
        {
            break;
        }

        if ( my_pos.absY() > rcsc::ServerParam::i().pitchHalfWidth() - 3.0
             && ( ( my_pos.y > 0.0 && dash_angle.degree() > 0.0 )
                  || ( my_pos.y < 0.0 && dash_angle.degree() < 0.0 ) )
             )
        {
            break;
        }

        my_vel *= ptype.playerDecay();
        stamina_model.simulateDash( ptype, actual_dash_power );

        self_cache.push_back( my_pos );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_LongDribble::canKick( const rcsc::WorldModel & wm,
                            const int n_turn,
                            const int n_dash,
                            const rcsc::Vector2D & receive_pos )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    const rcsc::Vector2D ball_pos = wm.ball().pos();
    const rcsc::Vector2D ball_vel = wm.ball().vel();
    const rcsc::AngleDeg target_angle = ( receive_pos - ball_pos ).th();

    //
    // check kick possibility
    //
    double first_speed = rcsc::calc_first_term_geom_series( ball_pos.dist( receive_pos ),
                                                      SP.ballDecay(),
                                                      1 + n_turn + n_dash );
    rcsc::Vector2D max_vel = rcsc::KickTable::calc_max_velocity( target_angle,
                                                     wm.self().kickRate(),
                                                     ball_vel );
    if ( max_vel.r2() < std::pow( first_speed, 2 ) )
    {
        return false;
    }

    //
    // check collision
    //
    const rcsc::Vector2D my_next = wm.self().pos() + wm.self().vel();
    const rcsc::Vector2D ball_next
        = ball_pos
        + ( receive_pos - ball_pos ).setLengthVector( first_speed );

    if ( my_next.dist2( ball_next ) < std::pow( wm.self().playerType().playerSize()
                                                + SP.ballSize()
                                                + 0.1,
                                                2 ) )
    {
        return false;
    }

    //
    // check opponent kickable area
    //

    const rcsc::PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
          o != o_end;
          ++o )
    {
        const rcsc::PlayerType * ptype = (*o)->playerTypePtr();
        rcsc::Vector2D o_next = (*o)->pos() + (*o)->vel();

        const double control_area = ( ( (*o)->goalie()
                                        && ball_next.x > SP.theirPenaltyAreaLineX()
                                        && ball_next.absY() < SP.penaltyAreaHalfWidth() )
                                      ? SP.catchableArea()
                                      : ptype->kickableArea() );

        if ( ball_next.dist2( o_next ) < std::pow( control_area + 0.1, 2 ) )
        {
            return false;
        }

        if ( (*o)->bodyCount() <= 1 )
        {
            o_next += rcsc::Vector2D::from_polar( SP.maxDashPower() * ptype->dashPowerRate() * ptype->effortMax(),
                                            (*o)->body() );
        }
        else
        {
            o_next += (*o)->vel().setLengthVector( SP.maxDashPower()
                                                   * ptype->dashPowerRate()
                                                   * ptype->effortMax() );
        }

        if ( ball_next.dist2( o_next ) < std::pow( control_area, 2 ) )
        {
            return false;
        }

    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Body_LongDribble::checkOpponent( const rcsc::WorldModel & wm,
                                  const int n_turn,
                                  const int n_dash,
                                  const rcsc::Vector2D & ball_pos,
                                  const rcsc::Vector2D & receive_pos )
{
    const rcsc::ServerParam & SP = rcsc::ServerParam::i();
    const int self_step = 1 + n_turn + n_dash;
    const rcsc::AngleDeg target_angle = ( receive_pos - ball_pos ).th();

    const bool in_penalty_area = ( receive_pos.x > SP.theirPenaltyAreaLineX()
                                   && receive_pos.absY() < SP.penaltyAreaHalfWidth() );

    int min_step = 1000;

    const rcsc::PlayerPtrCont::const_iterator o_end = wm.opponentsFromSelf().end();
    for ( rcsc::PlayerPtrCont::const_iterator o = wm.opponentsFromSelf().begin();
          o != o_end;
          ++o )
    {
        const rcsc::Vector2D & opos = ( (*o)->seenPosCount() <= (*o)->posCount()
                                  ? (*o)->seenPos()
                                  : (*o)->pos() );
        const rcsc::Vector2D ball_to_opp_rel = ( opos - ball_pos ).rotatedVector( -target_angle );

        if ( ball_to_opp_rel.x < -4.0 )
        {
            continue;
        }

        const rcsc::Vector2D & ovel = ( (*o)->seenVelCount() <= (*o)->velCount()
                                  ? (*o)->seenVel()
                                  : (*o)->vel() );

        const rcsc::PlayerType * ptype = (*o)->playerTypePtr();

        const bool goalie = ( (*o)->goalie() && in_penalty_area );
        const double control_area ( goalie
                                    ? SP.catchableArea()
                                    : ptype->kickableArea() );

        rcsc::Vector2D opp_pos = ptype->inertiaPoint( opos, ovel, self_step );
        double target_dist = opp_pos.dist( receive_pos );

        if ( target_dist
             > ptype->realSpeedMax() * ( self_step + (*o)->posCount() ) + control_area )
        {
            continue;
        }


        if ( target_dist - control_area < 0.001 )
        {
            return false;
        }

        double dash_dist = target_dist;
        dash_dist -= control_area;
        dash_dist -= 0.2;
        dash_dist -= (*o)->distFromSelf() * 0.01;

        int opp_n_dash = ptype->cyclesToReachDistance( dash_dist );

        int opp_n_turn = ( (*o)->bodyCount() > 1
                           ? 0
                           : Bhv_Predictor::predict_player_turn_cycle( ptype,
                                                                       (*o)->body(),
                                                                       ovel.r(),
                                                                       target_dist,
                                                                       ( receive_pos - opp_pos ).th(),
                                                                       control_area,
                                                                       false ) );

        int opp_n_step = ( opp_n_turn == 0
                           ? opp_n_turn + opp_n_dash
                           : opp_n_turn + opp_n_dash + 1 );
        int bonus_step = 0;

        if ( receive_pos.x < 27.0 )
        {
            bonus_step += 1;
        }

        if ( (*o)->isTackling() )
        {
            bonus_step = -5;
        }

        if ( ball_to_opp_rel.x > 0.8 )
        {
            bonus_step += 1;
            bonus_step += rcsc::bound( 0, (*o)->posCount() - 1, 8 );
        }
        else
        {
            int penalty_step = ( ( receive_pos.x > wm.offsideLineX()
                                   || receive_pos.x > 35.0 )
                                 ? 1
                                 : 0 );
            bonus_step =  rcsc::bound( 0, (*o)->posCount() - penalty_step, 3 );
        }

        if ( opp_n_step - bonus_step <= self_step )
        {
            return false;
        }

        if ( min_step > opp_n_step )
        {
            min_step = opp_n_step;
        }
    }

    return true;
}


/*--------------------------------------------------*/
bool
Body_LongDribble::execute( const rcsc::PlayerAgent * agent )
{
	if ( ! Strategy::i().long_dribble() )
    {
		return false;
    }

	const rcsc::WorldModel & wm = agent->world();
	if( wm.self().pos().x < -20 )
		return false;
	
	if( wm.self().pos().x > 40 && wm.self().pos().absY() < 17 )
		return false;
		
	if( wm.self().stamina() < rcsc::ServerParam::i().staminaMax() * 0.45 )
		return false;
	

	if( ! generate( wm ) )
		return false;
	
	
	rcsc::Vector2D target_point = wm.ball().pos() + rcsc::Vector2D::polar2vector( 5.0 , 0.0 );

    if( target_point.absX() > 40 || target_point.absY() > 32 )
		return false;

	double end_speed = 0.8;
    double first_speed = 2.0;
    do
    {
        first_speed = rcsc::calc_first_term_geom_series_last( end_speed, wm.ball().pos().dist( target_point ) , rcsc::ServerParam::i().ballDecay() );
        if ( first_speed < rcsc::ServerParam::i().ballSpeedMax() )
        {
            break;
        }
        end_speed -= 0.1;
    }
    while ( end_speed > 0.1 );

	first_speed = std::min( first_speed , rcsc::ServerParam::i().ballSpeedMax() ); 
	first_speed *= rcsc::ServerParam::i().ballDecay();
    
    if( agent->config().useCommunication() )
    {
		//Body_Pass::say_pass( agent , wm.self().unum() , target_point );
    }
    
	//agent->setNeckAction( new rcsc::Neck_TurnToBall() );
    return true;
}

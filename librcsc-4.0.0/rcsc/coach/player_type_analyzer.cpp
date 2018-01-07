// -*-c++-*-

/*!
  \file player_type_analyzer.cpp
  \brief player type analyzer class Header File
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
#include "config.h"
#endif

#include "player_type_analyzer.h"

#include "global_world_model.h"
#include "global_object.h"

#include <rcsc/common/player_param.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/common/logger.h>
#include <rcsc/game_mode.h>

//#define DEBUG_PRINT

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
PlayerTypeAnalyzer::Data::Data()
    : turned_( false )
    , kicked_( false )
    , tackling_( false )
    , maybe_referee_( false )
    , maybe_collide_( false )
    , maybe_kick_( false )
    , pos_( Vector2D::INVALIDATED )
    , vel_( 0.0, 0.0 )
    , body_( -360 )
    , invalid_flags_( PlayerParam::i().playerTypes(), 0 )
    , type_( Hetero_Default )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::Data::setDefaultType()
{
    invalid_flags_.assign( PlayerParam::i().playerTypes(), 0 );

    type_ = Hetero_Default;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::Data::setUnknownType()
{
    invalid_flags_.assign( PlayerParam::i().playerTypes(), 0 );

    type_ = Hetero_Unknown;
}

/*-------------------------------------------------------------------*/
/*!

*/
PlayerTypeAnalyzer::PlayerTypeAnalyzer( const GlobalWorldModel & world )
    : M_world( world )
    , M_updated_time( -1, 0 )
    , M_playmode( PM_BeforeKickOff )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::update()
{
    if ( M_updated_time == M_world.time() )
    {
        return;
    }

    const std::size_t max_types = static_cast< std::size_t >( PlayerParam::i().playerTypes() );
    for ( int i = 0; i < 11; ++i )
    {
        if ( M_teammate_data[i].invalid_flags_.size() != max_types )
        {
            M_teammate_data[i].invalid_flags_.resize( max_types, 0 );
        }

        if ( M_opponent_data[i].invalid_flags_.size() != max_types )
        {
            M_opponent_data[i].invalid_flags_.resize( max_types, 0 );
        }

#if 0
        // heterogeneous goalie is available in v14 or later
        if ( ! PlayerParam::i().allowMultDefaultType()
             && PlayerParam::i().ptMax() == 1
             && max_types > 0 )
        {
            // goalie is always default type.
            M_teammate_data[i].invalid_flags_[0] = 1;
            M_opponent_data[i].invalid_flags_[0] = 1;
        }
#endif
    }

    if ( M_opponent_type_used_count.size() != max_types )
    {
        M_opponent_type_used_count.resize( max_types, 0 );
#if 0
        // heterogeneous goalie is available in v14 or later
        if ( max_types > 0 )
        {
            M_opponent_type_used_count[0] = 1; // goalie is always default type.
        }
#endif
    }

    if ( M_updated_time.cycle() != M_world.time().cycle() - 1
         && M_updated_time.stopped() != M_world.time().stopped() - 1 )
    {
        // missed cycles??
//         if ( M_world.time().stopped() != 1 )
//         {
//             std::cerr << __FILE__ << ' ' << __LINE__
//                       << " missed cycles? last updated time = " << M_updated_time
//                       << " current = " << M_world.time()
//                       << std::endl;
//         }

        M_updated_time = M_world.time();
        updateLastData();
        return;
    }

    M_updated_time = M_world.time();

    const PlayMode pm = M_world.gameMode().getServerPlayMode();

    // just after the playmode change
    if ( M_playmode != pm )
    {
        M_playmode = pm;
        updateLastData();
        return;
    }

    switch ( M_world.gameMode().type() ) {
    case GameMode::PlayOn:
    case GameMode::KickIn_:
    case GameMode::FreeKick_:
    case GameMode::CornerKick_:
    case GameMode::GoalKick_:
        break;
    default:
        updateLastData();
        return; // not analyzed in other playmode
        break;
    }

    // analyzer player container

    analyze();

    updateLastData();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::reset( const int unum )
{
    if ( unum < 1 || 11 < unum )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << " Illegal unum = " << unum
                  << std::endl;
        return;
    }

    //if ( M_opponent_data[unum - 1].type_ != Hetero_Unknown )
    //{
    //    std::cout << M_world.time()
    //              << ' ' << M_world.ourTeamName()
    //              << " Coach: opponent " << unum << " changed."
    //              << std::endl;
    //}

    M_opponent_data[unum - 1].setUnknownType();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::updateLastData()
{
    M_prev_ball = M_world.ball();

    const std::vector< const GlobalPlayerObject * >::const_iterator t_end = M_world.teammates().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.teammates().begin();
          p != t_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_teammate_data[(*p)->unum() - 1];

        data.pos_ = (*p)->pos();
        data.vel_ = (*p)->vel();
        data.body_ = (*p)->body().degree();
    }


    const std::vector< const GlobalPlayerObject * >::const_iterator o_end = M_world.opponents().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        data.pos_ = (*p)->pos();
        data.vel_ = (*p)->vel();
        data.body_ = (*p)->body().degree();
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::analyze()
{
    checkTurn();
    checkTackle();
    checkReferee();
    checkCollisions();
    checkKick();
    checkPlayerDecay();
    checkPlayerSpeedMax();
    checkTurnMoment();

    const int max_types = PlayerParam::i().playerTypes();

    const std::vector< const GlobalPlayerObject * > & players = M_world.opponents();

    const std::vector< const GlobalPlayerObject * >::const_iterator end = players.end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = players.begin();
          p != end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() )
        {
            std::cerr << __FILE__ << ' ' << __LINE__
                      << " Illegal uniform number " << (*p)->unum()
                      << std::endl;
            continue;
        }

        Data & data = M_opponent_data[(*p)->unum() - 1];

#if 0
        // heterogeneous goalie is available in v14 or later
        // goalie is always the default type in v13 or previous.
        if ( (*p)->goalie() )
        {
            data.type_ = Hetero_Default;
            continue;
        }
#endif

        // if player type is not changed, not need to analyze
        if ( data.type_ == Hetero_Default ) continue;
        // if player type has already been determined, not need to analyze
        if ( data.type_ != Hetero_Unknown ) continue;
        // if player might be moved by referee, we must not analyze
        if ( data.maybe_referee_ ) continue;

        int invalid_count = 0;
        for ( int t = 0; t < max_types; ++t )
        {
            if ( data.invalid_flags_[t] != 0 )
            {
                ++invalid_count;
            }
        }

        if ( invalid_count == max_types )
        {
            // no candidate
            std::cout <<  M_world.time()
                      << ' ' << M_world.ourTeamName()
                      << " Coach: no player type for opponent " << (*p)->unum()
                      << ". restart analysis."
                      << std::endl;
            data.setUnknownType();
        }
        else if ( invalid_count == max_types - 1 )
        {
            // success! only 1 candidate.
            for ( int t = 0; t < max_types; ++t )
            {
                if ( data.invalid_flags_[t] == 0 )
                {
                    std::cout << M_world.time()
                              << ' ' << M_world.ourTeamName()
                              << " Coach: determined opponent " << (*p)->unum()
                              << " type = " << t
                              << std::endl;
                    data.type_ = t;

                    M_opponent_type_used_count[t] += 1;
                    if ( M_opponent_type_used_count[t] >= PlayerParam::i().ptMax() )
                    {
                        for ( int i = 0; i < 11; ++i )
                        {
                            if ( M_opponent_data[i].type_ == Hetero_Unknown )
                            {
                                M_opponent_data[i].invalid_flags_[t] = 1;
                            }
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            // several candidates
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkTurn()
{
    for ( int i = 0; i < 11 ; ++i )
    {
        M_teammate_data[i].turned_ = false;
        M_opponent_data[i].turned_ = false;
    }

    const std::vector< const GlobalPlayerObject * >::const_iterator t_end = M_world.teammates().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.teammates().begin();
          p != t_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_teammate_data[(*p)->unum() - 1];

        if ( data.body_ != -360.0 )
        {
            if ( std::fabs( data.body_ - (*p)->body().degree() ) > 0.5 )
            {
                data.turned_ = true;
            }
        }
    }

    const std::vector< const GlobalPlayerObject * >::const_iterator o_end = M_world.opponents().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( data.body_ != -360.0 )
        {
            if ( std::fabs( data.body_ - (*p)->body().degree() ) > 0.5 )
            {
                data.turned_ = true;
            }
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkTackle()
{
    for ( int i = 0; i < 11 ; ++i )
    {
        M_teammate_data[i].tackling_ = false;
        M_opponent_data[i].tackling_ = false;
    }

    const std::vector< const GlobalPlayerObject * >::const_iterator t_end = M_world.teammates().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.teammates().begin();
          p != t_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        M_teammate_data[(*p)->unum() - 1].tackling_ = (*p)->isTackling();
    }

    const std::vector< const GlobalPlayerObject * >::const_iterator o_end = M_world.opponents().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        M_opponent_data[(*p)->unum() - 1].tackling_ = (*p)->isTackling();
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkReferee()
{
    for ( int i = 0; i < 11; ++i )
    {
        M_opponent_data[i].maybe_referee_ = false;
    }

    const double penalty_x
        = ServerParam::i().pitchHalfLength()
        - ServerParam::i().penaltyAreaLength()
        - 2.0;
    const double penalty_y
        = ServerParam::i().penaltyAreaWidth() * 0.5
        + 2.0;

    const bool our_set_play = M_world.gameMode().isOurSetPlay( M_world.ourSide() );

    const std::vector< const GlobalPlayerObject * > & players = M_world.opponents();

    const std::vector< const GlobalPlayerObject * >::const_iterator end = players.end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = players.begin();
          p != end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        // player may be moved by referee
        if ( our_set_play )
        {
            if ( (*p)->pos().dist2( M_world.ball().pos() ) < 12.0 * 12.0
                 || ( M_world.gameMode().type() == GameMode::GoalKick_
                      && (*p)->pos().absX() > penalty_x
                      && (*p)->pos().absY() < penalty_y )
                 )
            {
                data.maybe_referee_ = true;
            }
        }

        // player may be moved by simulator
        if ( (*p)->pos().absX() > ServerParam::i().pitchHalfLength() + 3.0
             || (*p)->pos().absY() > ServerParam::i().pitchHalfWidth() + 3.0 )
        {
            data.maybe_referee_ = true;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!
  \todo strict player size check
*/
void
PlayerTypeAnalyzer::checkCollisions()
{
    for ( int i = 0; i < 11; ++i )
    {
        M_opponent_data[i].maybe_collide_ = false;
    }

    const double ball_collide_dist2
        = std::pow( ServerParam::i().defaultPlayerSize()
                    + ServerParam::i().ballSize()
                    + 0.02,
                    2 );
    const double player_collide_dist2
        = std::pow( ServerParam::i().defaultPlayerSize() * 2.0 + 0.02, 2 );
    const Vector2D pole_pos( ServerParam::i().pitchHalfLength()
                             - ServerParam::i().goalPostRadius(),
                             ServerParam::i().goalHalfWidth()
                             + ServerParam::i().goalPostRadius() );
    const double pole_collide_dist2
        = std::pow( ServerParam::i().defaultPlayerSize()
                    + ServerParam::i().goalPostRadius()
                    + 2.0,
                    2 );


    const std::list< GlobalPlayerObject >::const_iterator all_end = M_world.players().end();

    const std::vector< const GlobalPlayerObject * > & opponents = M_world.opponents();
    const std::vector< const GlobalPlayerObject * >::const_iterator o_end = opponents.end();

    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = opponents.begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        // check ball
        if ( (*p)->pos().dist2( M_world.ball().pos() ) < ball_collide_dist2 )
        {
            data.maybe_collide_ = true;
        }
    }

    // check other opponent players
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = opponents.begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        for ( std::vector< const GlobalPlayerObject * >::const_iterator pp = p + 1;
              pp != o_end;
              ++pp )
        {
			if(!(*pp)) continue;
            if ( (*pp)->unum() == (*p)->unum() ) continue;

            if ( (*pp)->pos().dist2( (*p)->pos() ) < player_collide_dist2 )
            {
                data.maybe_collide_ = true;
                if ( 1 <= (*pp)->unum()
                     && (*pp)->unum() <= 11 )
                {
                    M_opponent_data[(*pp)->unum() - 1].maybe_collide_ = true;
                }
            }
        }
    }

    // check teammate players
    const std::vector< const GlobalPlayerObject * > & teammates = M_world.teammates();
    const std::vector< const GlobalPlayerObject * >::const_iterator t_end = teammates.end();

    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = opponents.begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( data.maybe_collide_ ) continue;

        for ( std::vector< const GlobalPlayerObject * >::const_iterator pp = teammates.begin();
              pp != t_end;
              ++pp )
        {
            if ( (*pp)->pos().dist2( (*p)->pos() ) < player_collide_dist2 )
            {
                data.maybe_collide_ = true;
                break;
            }
        }
    }

    // check goal post
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = opponents.begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( data.maybe_collide_ ) continue;

        Vector2D abs_pos( (*p)->pos().absX(), (*p)->pos().absY() );
        if ( abs_pos.dist2( pole_pos ) < pole_collide_dist2 )
        {
            data.maybe_collide_ = true;
        }
    }

}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkKick()
{
    static double S_max_kickable_area2 = -1.0;

    for ( int i = 0; i < 11; ++i )
    {
        M_opponent_data[i].kicked_ = false;
        M_teammate_data[i].kicked_ = false;
        M_opponent_data[i].maybe_kick_ = false;
        M_teammate_data[i].maybe_kick_ = false;
    }

    const int max_types = PlayerParam::i().playerTypes();

    if ( S_max_kickable_area2 < 0.0 )
    {
        for ( int t = 0; t < max_types; ++t )
        {
            const PlayerType * player_type = PlayerTypeSet::i().get( t );
            if ( ! player_type ) continue;

            double k2 = std::pow( player_type->kickableArea(), 2 );
            if ( k2 > S_max_kickable_area2 )
            {
                S_max_kickable_area2 = k2;
            }
        }
    }

    bool ball_kicked = false;

    const Vector2D new_ball_pos = M_prev_ball.pos() + M_prev_ball.vel();
    const Vector2D new_ball_vel = M_prev_ball.vel() * ServerParam::i().ballDecay();
    const double rand_max = M_prev_ball.vel().r() * ServerParam::i().ballRand();

    if ( std::fabs( M_world.ball().pos().x - new_ball_pos.x ) > rand_max
         || std::fabs( M_world.ball().pos().x - new_ball_pos.x ) > rand_max )
    {
        ball_kicked = true;
    }

    if ( std::fabs( M_world.ball().vel().x - new_ball_vel.x )
         > ServerParam::i().ballDecay() * rand_max
         || ( std::fabs( M_world.ball().vel().y - new_ball_vel.y )
              > ServerParam::i().ballDecay() * rand_max )
         )
    {
        ball_kicked = true;
    }

    int count = 0;
    int kicker_idx = -1;

    if ( M_world.clientVersion() >= 13 )
    {
        const std::vector< const GlobalPlayerObject * >::const_iterator t_end = M_world.teammates().end();
        for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.teammates().begin();
              p != t_end;
              ++p )
        {
			if(!(*p)) continue;
            if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

            Data & data = M_teammate_data[(*p)->unum() - 1];

            data.kicked_ = data.maybe_kick_ = (*p)->kicked();
            if ( (*p)->kicked() )
            {
                ++count;
            }
        }

        const std::vector< const GlobalPlayerObject * >::const_iterator o_end = M_world.opponents().end();
        for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
              p != o_end;
              ++p )
        {
			if(!(*p)) continue;
            if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

            Data & data = M_opponent_data[(*p)->unum() - 1];

            data.kicked_ = data.maybe_kick_ = (*p)->kicked();
            if ( (*p)->kicked() )
            {
                ++count;
                kicker_idx = (*p)->unum() - 1;
            }
        }
    }
    else
    {
        // v12 or older client cannot receive any kicking state information.

        for ( int i = 0; i < 11; ++i )
        {
            if ( ! M_teammate_data[i].turned_
                 && ! M_teammate_data[i].tackling_
                 && M_teammate_data[i].pos_.isValid() )
            {
                if ( M_prev_ball.pos().dist2( M_teammate_data[i].pos_ )
                     < S_max_kickable_area2 )
                {
                    M_teammate_data[i].maybe_kick_ = true;
                    ++count;
                }
            }

            if ( ! M_opponent_data[i].turned_
                 && ! M_opponent_data[i].tackling_
                 && M_opponent_data[i].pos_.isValid() )
            {
                if ( M_prev_ball.pos().dist2( M_opponent_data[i].pos_ )
                     < S_max_kickable_area2 )
                {
                    M_opponent_data[i].maybe_kick_ = true;
                    ++count;
                    kicker_idx = i;
                }
            }
        }
    }

    if ( ! ball_kicked )
    {
        // do nothing
        return;
    }

    if ( count == 0 )
    {
        // ball may be tackled
    }
    else if ( count == 1 && kicker_idx != -1 )
    {
        Data & data = M_opponent_data[kicker_idx];

        if ( data.maybe_collide_ )
        {
            // cannot determine kick or collide.
        }
        else
        {
            const double ball_dist = M_prev_ball.pos().dist( data.pos_ );

            for ( int t = 0; t < max_types; ++t )
            {
                if ( data.invalid_flags_[t] != 0 ) continue;

                const PlayerType * player_type = PlayerTypeSet::i().get( t );
                if ( ! player_type ) continue;


                if ( ball_dist > player_type->kickableArea() + 0.001 )
                {
                    data.invalid_flags_[t] = 1;
                }
            }
        }
    }
    else
    {
        // several kickers
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkPlayerDecay()
{
    const int max_types = PlayerParam::i().playerTypes();

    const std::vector< const GlobalPlayerObject * > & players
        = ( M_world.ourSide() == LEFT
            ? M_world.playersRight()
            : M_world.playersLeft() );

    const std::vector< const GlobalPlayerObject * >::const_iterator end = players.end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = players.begin();
          p != end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( data.maybe_collide_ ) continue;
        if ( data.maybe_referee_ ) continue;
        if ( ! data.turned_
             && ! data.kicked_
             //&& ! data.maybe_kick_
             && ! data.tackling_ )
        {
            continue;
        }
        if ( ! data.pos_.isValid() ) continue;
        if ( data.pos_.dist2( (*p)->pos() ) < 0.0001 ) continue;

        double rand_max = data.vel_.r() * ServerParam::i().playerRand();
        if ( rand_max < 0.00001 ) continue;

        for ( int t = 0; t < max_types; ++t )
        {
            if ( data.invalid_flags_[t] != 0 ) continue;

            const PlayerType * player_type = PlayerTypeSet::i().get( t );
            if ( ! player_type ) continue;

#if 0
            // old noise model
            double rand_x
                = std::fabs( ( (*p)->vel().x
                               - data.vel_.x * player_type->playerDecay() )
                             / player_type->playerDecay() );

            double rand_y
                = std::fabs( ( (*p)->vel().y
                               - data.vel_.y * player_type->playerDecay() )
                             / player_type->playerDecay() );

            if ( rand_x > rand_max + 0.0000001
                 || rand_y > rand_max + 0.0000001 )
            {
                data.invalid_flags_[t] = 1;
                //std::cout << M_world.time()
                //          << ' ' << M_world.ourTeamName()
                //          << " Coach: opponent " << (*p)->unum()
                //          << "  detect invalid decay. type = "
                //          << t
                //          << std::endl;
            }
#else
            // rcssserver-13 or lator
            Vector2D rand_vec
                = ( (*p)->vel() - data.vel_ * player_type->playerDecay() )
                / player_type->playerDecay();
            double rand_r = rand_vec.r();
            if ( rand_r > rand_max + 0.0000001 )
            {
                data.invalid_flags_[t] = 1;
            }
#endif
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkPlayerSpeedMax()
{
    const int max_types = PlayerParam::i().playerTypes();

    const std::vector< const GlobalPlayerObject * >::const_iterator end = M_world.opponents().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
          p != end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( data.turned_ ) continue;
        if ( data.kicked_ ) continue;
        if ( data.maybe_referee_ ) continue;
        if ( data.tackling_ ) continue;
        if ( data.maybe_collide_ ) continue;
        if ( ! data.pos_.isValid() ) continue;

        const Vector2D last_move( (*p)->pos().x - data.pos_.x,
                                  (*p)->pos().y - data.pos_.y );
        const double last_move_dist = last_move.r();
        const Vector2D last_accel = last_move - data.vel_;
        const double last_accel_r = last_accel.r();
        const double current_speed = (*p)->vel().r();

        for ( int t = 0; t < max_types; ++t )
        {
            if ( data.invalid_flags_[t] != 0 ) continue;

            const PlayerType * ptype = PlayerTypeSet::i().get( t );
            if ( ! ptype ) continue;

            //
            // accel range check
            //
            const double max_accel = ServerParam::i().maxDashPower() * ptype->dashRate( ptype->effortMax() );
            const double last_max_noise = ( current_speed / ptype->playerDecay()
                                            * ServerParam::i().playerRand()
                                            / ( 1.0 + ServerParam::i().playerRand() ) );

            if ( last_accel_r > max_accel + last_max_noise + 0.0001 )
            {
                data.invalid_flags_[t] = 1;
                continue;
            }

            //
            // speed range check
            //

            // XXX
            double max_move = ptype->realSpeedMax() * ( 1.0 + ServerParam::i().playerRand() );
            max_move *= ptype->playerDecay();
            max_move += ServerParam::i().maxDashPower() * ptype->dashRate( ptype->effortMax() );
            max_move *= ( 1.0 + ServerParam::i().playerRand() );

//             if ( (*p)->unum() == 1 )
//             {
//                 dlog.addText( Logger::ANALYZER,
//                               __FILE__" (checkPlayerSpeedMax) type=%d real_speed_max=%f max_move=%f",
//                               t, ptype->realSpeedMax(), max_move );
//             }

            //double max_move = ptype->realSpeedMax() * ( 1.0 + ServerParam::i().playerRand() );

            if ( last_move_dist > max_move )
            {
                data.invalid_flags_[t] = 1;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeAnalyzer::checkTurnMoment()
{
    const int max_types = PlayerParam::i().playerTypes();
    const double max_moment = std::max( std::fabs( ServerParam::i().minMoment() ),
                                        std::fabs( ServerParam::i().maxMoment() ) );

    const std::vector< const GlobalPlayerObject * >::const_iterator o_end = M_world.opponents().end();
    for ( std::vector< const GlobalPlayerObject * >::const_iterator p = M_world.opponents().begin();
          p != o_end;
          ++p )
    {
		if(!(*p)) continue;
        if ( (*p)->unum() < 1 || 11 < (*p)->unum() ) continue;

        Data & data = M_opponent_data[(*p)->unum() - 1];

        if ( ! data.turned_ ) continue;

        const double player_speed = data.vel_.r();
        const double turn_angle = ( (*p)->body() - data.body_ ).abs();

        for ( int t = 0; t < max_types; ++t )
        {
            if ( data.invalid_flags_[t] != 0 ) continue;

            const PlayerType * ptype = PlayerTypeSet::i().get( t );
            if ( ! ptype ) continue;

            const double max_turn = max_moment / ( 1.0 + ptype->inertiaMoment() * player_speed );

            if ( turn_angle > max_turn * ( 1.0 + ServerParam::i().playerRand() ) + 1.0001 )
            {
                data.invalid_flags_[t] = 1;
            }
        }

    }
}

}

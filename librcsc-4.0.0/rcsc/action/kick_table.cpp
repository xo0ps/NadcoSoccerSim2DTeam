// -*-c++-*-

/*!
  \file kick_table.cpp
  \brief kick planner and cache holder class Source File
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

#include "kick_table.h"

#include <rcsc/player/world_model.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/game_time.h>
#include <rcsc/math_util.h>
#include <rcsc/soccer_math.h>
#include <rcsc/timer.h>

#include <algorithm>
#include <functional>
#include <fstream>

#define DEBUG_PROFILE
// #define DEBUG
#define DEBUG_EVALUATE
// #define DEBUG_THREE_STEP

namespace rcsc {

const double KickTable::NEAR_SIDE_RATE = 0.3;
const double KickTable::MID_RATE = 0.5;
const double KickTable::FAR_SIDE_RATE = 0.7;

/*!
 \struct TableCmp
 \brief kick path evaluator
*/
struct TableCmp {
    /*!
      \brief compare operation function
      \param lhs left hand side variable
      \param rhs right hand side variable
      \return compared result
     */
    bool operator()( const KickTable::Path & lhs,
                     const KickTable::Path & rhs )
      {
          if ( lhs.max_speed_ == rhs.max_speed_ )
          {
              return lhs.power_ < rhs.power_;
          }
          return lhs.max_speed_ > rhs.max_speed_;
      }
};


/*!
 \struct Sequence
 \brief kick sequence evaluator
*/
struct SequenceCmp {
    /*!
      \brief compare operation function
      \param lhs left hand side variable
      \param rhs right hand side variable
      \return compared result
     */
    bool operator()( const KickTable::Sequence & lhs,
                     const KickTable::Sequence & rhs )
      {
          return lhs.score_ < rhs.score_;
      }
};

/*-------------------------------------------------------------------*/
/*!

 */
double
KickTable::calc_near_dist( const PlayerType & player_type )
{
    //       0.3 + 0.6*0.3 + 0.085 = 0.565
    // near: 0.3 + 0.7*0.3 + 0.085 = 0.595
    //       0.3 + 0.8*0.3 + 0.085 = 0.625
    return bound( player_type.playerSize() + ServerParam::i().ballSize() + 0.1,
                  ( player_type.playerSize()
                    + ( player_type.kickableMargin() * NEAR_SIDE_RATE )
                    + ServerParam::i().ballSize() ),
                  player_type.kickableArea() - 0.2 );
}

/*-------------------------------------------------------------------*/
/*!

 */
double
KickTable::calc_mid_dist( const PlayerType & player_type )
{
    //      0.3 + 0.6*0.5 + 0.085 = 0.705
    // mid: 0.3 + 0.7*0.5 + 0.085 = 0.735
    //      0.3 + 0.8*0.5 + 0.085 = 0.765
    return bound( player_type.playerSize() + ServerParam::i().ballSize() + 0.1,
                  ( player_type.playerSize()
                    + ( player_type.kickableMargin() * MID_RATE )
                    + ServerParam::i().ballSize() ),
                  player_type.kickableArea() - 0.2 );
}

/*-------------------------------------------------------------------*/
/*!

 */
double
KickTable::calc_far_dist( const PlayerType & player_type )
{
    //      0.3 + 0.6*0.7 + 0.085 = 0.865 (=0.985-0.12 -> 0.785)
    // far: 0.3 + 0.7*0.7 + 0.085 = 0.875 (=1.085-0.21)
    //      0.3 + 0.8*0.7 + 0.085 = 0.945 (=1.185-0.24)

    //      0.3 + 0.6*0.68 + 0.085 = 0.793 (=0.985-0.192 -> 0.760)
    // far: 0.3 + 0.7*0.68 + 0.085 = 0.861 (=1.085-0.224 -> 0.860)
    //      0.3 + 0.8*0.68 + 0.085 = 0.929 (=1.185-0.256)

    //      0.3 + 0.6*0.675 + 0.085 = 0.79   (=0.985-0.195)
    // far: 0.3 + 0.7*0.675 + 0.085 = 0.8575 (=1.085-0.2275)
    //      0.3 + 0.8*0.675 + 0.085 = 0.925  (=1.185-0.26)

    return bound( player_type.playerSize() + ServerParam::i().ballSize() + 0.1,
                  ( player_type.playerSize()
                    + ( player_type.kickableMargin() * FAR_SIDE_RATE )
                    + ServerParam::i().ballSize() ),
                  player_type.kickableArea() - 0.2 );
                  //player_type.kickableArea() - 0.22 );
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
KickTable::calc_max_velocity( const AngleDeg & target_angle,
                              const double & krate,
                              const Vector2D & ball_vel )
{
    const double ball_speed_max2 = std::pow( ServerParam::i().ballSpeedMax(), 2 );
    const double max_accel
        = std::min( ServerParam::i().maxPower() * krate,
                    ServerParam::i().ballAccelMax() );

    Ray2D desired_ray( Vector2D( 0.0, 0.0 ), target_angle );
    Circle2D next_reachable_circle( ball_vel, max_accel );

    Vector2D vel1, vel2;
    int num = next_reachable_circle.intersection( desired_ray, &vel1, &vel2 );

    if ( num == 0 )
    {
        return Vector2D( 0.0, 0.0 );
    }

    if ( num == 1 )
    {
        if ( vel1.r2() > ball_speed_max2 )
        {
            // next inertia ball point is within reachable circle.
            if ( next_reachable_circle.contains( Vector2D( 0.0, 0.0 ) ) )
            {
                // can adjust angle at least
                vel1.setLength( ServerParam::i().ballSpeedMax() );
            }
            else
            {
                // failed
                vel1.assign( 0.0, 0.0 );
            }
        }
        return vel1;
    }

    //
    // num == 2
    //   ball reachable circle does not contain the current ball pos.

    double length1 = vel1.r2();
    double length2 = vel2.r2();

    if ( length1 < length2 )
    {
        std::swap( vel1, vel2 );
        std::swap( length1, length2 );
    }

    if ( length1 > ball_speed_max2 )
    {
        if ( length2 > ball_speed_max2 )
        {
            // failed
            vel1.assign( 0.0, 0.0 );
        }
        else
        {
            vel1.setLength( ServerParam::i().ballSpeedMax() );
        }
    }

    return vel1;
}

/*-------------------------------------------------------------------*/
/*!

 */
KickTable &
KickTable::instance()
{
    static KickTable s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
KickTable::KickTable()
    : M_player_size( 0.0 )
    , M_kickable_margin( 0.0 )
    , M_ball_size( 0.0 )
{
    for ( int i = 0; i < MAX_DEPTH; ++ i )
    {
        M_state_cache[i].reserve( NUM_STATE );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::createTables()
{
    const PlayerType player_type; // default type

    if ( std::fabs( M_player_size - player_type.playerSize() ) < rcsc::EPS
         && std::fabs( M_kickable_margin - player_type.kickableMargin() ) < rcsc::EPS
         && std::fabs( M_ball_size - ServerParam::i().ballSize() ) < rcsc::EPS )
    {
        return false;
    }

    //std::cerr << "createTables" << std::endl;

    M_player_size = player_type.playerSize();
    M_kickable_margin = player_type.kickableMargin();
    M_ball_size = ServerParam::i().ballSize();

    createStateList( player_type );

    MSecTimer timer;

    const double angle_step = 360.0 / DEST_DIR_DIVS;
    AngleDeg angle = -180.0;

    for ( int i = 0; i < DEST_DIR_DIVS; ++i, angle += angle_step )
    {
        createTable( angle, M_tables[i] );
    }

#if 0
    const double kprate = ServerParam::i().kickPowerRate();
    for ( std::vector< State >::iterator s = M_state_list.begin();
          s != M_state_list.end();
          ++s )
    {
        std::cout << "  state "
                  << " index=" << s->index_
                  << " angle="  << s->pos_.th()
                  << " dist="  << s->pos_.r()
                  << " pos=" << s->pos_
                  << " kick_rate=" << s->kick_rate_
                  << " (" << s->kick_rate_ / kprate * 100.0 << "%)"
                  << std::endl;
    }

    for ( int i = 0; i < DEST_DIR_DIVS; ++i, angle += angle_step )
    {
        std::cout << "create table " << i << " : angle="  << angle << std::endl;
        for ( std::vector< Path >::iterator p = M_tables[i].begin();
              p != M_tables[i].end();
              ++p )
        {
            std::cout << "  table "
                      << " origin=" << p->origin_
                      << " dest=" << p->dest_
                      << " max_speed=" << p->max_speed_
                      << " power=" << p->power_
                      << std::endl;
        }
    }
#endif
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::read( const std::string & file_path )
{
    int n_line = 0;

    std::ifstream fin( file_path.c_str() );
    if ( ! fin.is_open() )
    {
        return false;
    }

    M_state_list.clear();
    M_state_list.reserve( NUM_STATE );

    for ( int dir = 0; dir < DEST_DIR_DIVS; ++dir )
    {
        M_tables[dir].clear();
        M_tables[dir].reserve( NUM_STATE * NUM_STATE );
    }

    std::string line_buf;

    //
    // read simulator paramters
    //
    double player_size = 0.0;
    double kickable_margin = 0.0;
    double ball_size = 0.0;

    ++n_line;
    if ( ! std::getline( fin, line_buf ) )
    {
        return false;
    }

    if ( std::sscanf( line_buf.c_str(),
                      " %lf %lf %lf ",
                      &player_size, &kickable_margin, &ball_size ) != 3 )
    {
        return false;
    }

    //
    // read state size
    //
    int state_size = 0;

    ++n_line;
    if ( ! std::getline( fin, line_buf ) )
    {
        return false;
    }

    if ( std::sscanf( line_buf.c_str(), " %d ", &state_size ) != 1 )
    {
        return false;
    }

    //
    // read state list
    //
    for ( int i = 0; i < state_size; ++i )
    {
        ++n_line;
        if ( ! std::getline( fin, line_buf ) )
        {
            return false;
        }

        State state;
        if ( std::sscanf( line_buf.c_str(),
                          " %d %lf %lf %lf ",
                          &state.index_,
                          &state.pos_.x, &state.pos_.y,
                          &state.kick_rate_ ) != 4 )
        {
            return false;
        }

        state.flag_ = SAFETY;
        M_state_list.push_back( state );

    }

    for ( int dir = 0; dir < DEST_DIR_DIVS; ++dir )
    {
        //
        // read path size
        //
        int path_size = 0;

        ++n_line;
        if ( ! std::getline( fin, line_buf ) )
        {
            return false;
        }

        if ( std::sscanf( line_buf.c_str(), " %d ", &path_size ) != 1 )
        {
            std::cerr << "read kick table ... failed. line=" << n_line
                      << std::endl;
            return false;
        }

        //
        // read path list
        //
        for ( int i = 0; i < path_size; ++i )
        {
            ++n_line;
            if ( ! std::getline( fin, line_buf ) )
            {
                std::cerr << "read kick table ... failed. line=" << n_line
                          << std::endl;
                return false;
            }

            Path path( 0, 0 );
            if ( std::sscanf( line_buf.c_str(),
                              " %d %d %lf %lf ",
                              &path.origin_,
                              &path.dest_,
                              &path.max_speed_,
                              &path.power_ ) != 4 )
            {
                return false;
            }

            M_tables[dir].push_back( path );
        }
    }

    M_player_size = player_size;
    M_kickable_margin = kickable_margin;
    M_ball_size = ball_size;

    std::cerr << "read kick table ... ok" << std::endl;

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::write( const std::string & file_path )
{
    std::ofstream fout( file_path.c_str() );
    if ( ! fout.is_open() )
    {
        return false;
    }

    //
    // write server parameters
    //
    fout << M_player_size << ' '
         << M_kickable_margin << ' '
         << M_ball_size << '\n';

    //
    // write state size
    //
    fout << M_state_list.size() << '\n';

    //
    // write state list
    //
    for ( std::vector< State >::const_iterator s = M_state_list.begin();
          s != M_state_list.end();
          ++s )
    {
        fout << s->index_ << ' '
             << s->pos_.x << ' ' << s->pos_.y << ' '
             << s->kick_rate_ << '\n';
    }
    std::cerr << std::flush;

    for ( int dir = 0; dir < DEST_DIR_DIVS; ++dir )
    {
        fout << M_tables[dir].size() << '\n';

        for ( std::vector< Path >::const_iterator t = M_tables[dir].begin();
              t != M_tables[dir].end();
              ++t )
        {
            fout << t->origin_ << ' '
                 << t->dest_ << ' '
                 << t->max_speed_ << ' '
                 << t->power_ << '\n';
        }
    }

    fout.flush();
    fout.close();

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::createStateList( const PlayerType & player_type )
{
    const double near_dist = calc_near_dist( player_type );
    const double mid_dist = calc_mid_dist( player_type );
    const double far_dist = calc_far_dist( player_type );

    const double near_angle_step = 360.0 / STATE_DIVS_NEAR;
    const double mid_angle_step = 360.0 / STATE_DIVS_MID;
    const double far_angle_step = 360.0 / STATE_DIVS_FAR;

#if 0
    std::cout << "createStateList"
              << "\n  near_dist=" << near_dist
              << " mid_dist=" << mid_dist
              << " far_dist=" << far_dist
              << "\n  near_angle_step=" << near_angle_step
              << " mid_angle_step=" << mid_angle_step
              << " far_angle_step=" << far_angle_step
              << std::endl;
#endif

    int index = 0;
    M_state_list.clear();
    M_state_list.reserve( NUM_STATE );

    for ( int near = 0; near < STATE_DIVS_NEAR; ++near )
    {
        AngleDeg angle = -180.0 + ( near_angle_step * near );
        Vector2D pos = Vector2D::polar2vector( near_dist, angle );
        double krate = player_type.kickRate( near_dist, angle.degree() );
        M_state_list.push_back( State( index, near_dist, pos, krate ) );
        ++index;
    }

    for ( int mid = 0; mid < STATE_DIVS_MID; ++mid )
    {
        AngleDeg angle = -180.0 + ( mid_angle_step * mid );
        Vector2D pos = Vector2D::polar2vector( mid_dist, angle );
        double krate = player_type.kickRate( mid_dist, angle.degree() );
        M_state_list.push_back( State( index, mid_dist, pos, krate ) );
        ++index;
    }

    for ( int far = 0; far < STATE_DIVS_FAR; ++far )
    {
        AngleDeg angle = -180.0 + ( far_angle_step * far );
        Vector2D pos = Vector2D::polar2vector( far_dist, angle );
        double krate = player_type.kickRate( far_dist, angle.degree() );
        M_state_list.push_back( State( index, far_dist, pos, krate ) );
        ++index;
    }

#if 0
    for ( std::vector< State >::const_iterator s = M_state_list.begin();
          s != M_state_list.end();
          ++s )
    {
        std::cerr << s->index_ << ' '
                  << s->pos_.x << ' ' << s->pos_.y << ' '
                  << s->kick_rate_ << '\n';
    }
    std::cerr << std::flush;
#endif
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::createTable( const AngleDeg & angle,
                        std::vector< Path > & table )
{
    const int max_combination = NUM_STATE * NUM_STATE;
    const int max_state = M_state_list.size();

    table.clear();
    table.reserve( max_combination );

    for ( int origin = 0; origin < max_state; ++origin )
    {
        for ( int dest = 0; dest < max_state; ++dest )
        {
            Vector2D vel = M_state_list[dest].pos_ - M_state_list[origin].pos_;
            Vector2D max_vel = calc_max_velocity( angle,
                                                  M_state_list[dest].kick_rate_,
                                                  vel );
            Vector2D accel = max_vel - vel;

            Path path( origin, dest );
            path.max_speed_ = max_vel.r();
            path.power_ = accel.r() / M_state_list[dest].kick_rate_;
            table.push_back( path );
        }
    }

    std::sort( table.begin(), table.end(), TableCmp() );
    if ( table.size() > MAX_TABLE_SIZE )
    {
        table.erase( table.begin() + MAX_TABLE_SIZE,
                     table.end() );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::updateState( const WorldModel & world )
{
    static GameTime s_update_time( -1, 0 );

    if ( s_update_time == world.time() )
    {
        return;
    }

    s_update_time = world.time();

    //
    // update current state
    //
#ifdef DEBUG_PROFILE
    MSecTimer timer;
#endif

    createStateCache( world );

}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::createStateCache( const WorldModel & world )
{

    const ServerParam & param = ServerParam::i();
    const Rect2D pitch
        = param.keepawayMode()
        ? Rect2D( Vector2D( - param.keepawayLength() * 0.5 + 0.2,
                            - param.keepawayWidth() * 0.5 + 0.2 ),
                  Size2D( param.keepawayLength() - 0.4,
                          param.keepawayWidth() - 0.4 ) )
        : Rect2D( Vector2D( - param.pitchHalfLength(),
                            - param.pitchHalfWidth() ),
                  Size2D( param.pitchLength(),
                          param.pitchWidth() ) );

    const PlayerType & self_type = world.self().playerType();
    const double near_dist = calc_near_dist( self_type );
    const double mid_dist = calc_mid_dist( self_type );
    const double far_dist = calc_far_dist( self_type );

    //
    // current state
    //
    {
        Vector2D rpos = world.ball().rpos();
        rpos.rotate( - world.self().body() );

        const double dist = rpos.r();
        const AngleDeg angle = rpos.th();

        const int dir_div = ( std::fabs( dist - near_dist ) < std::fabs( dist - far_dist )
                              ? STATE_DIVS_NEAR
                              : STATE_DIVS_FAR );

        M_current_state.index_ = static_cast< int >( rint( dir_div * rint( angle.degree() + 180.0 ) / 360.0 ) );
        if ( M_current_state.index_ >= dir_div ) M_current_state.index_ = 0;

        //M_current_state.pos_ = world.ball().rpos();
        M_current_state.pos_ = world.ball().pos();
        M_current_state.kick_rate_ = world.self().kickRate();
        checkInterfereAt( world, 0, M_current_state );
    }

    //
    // create future state
    //

    Vector2D self_pos = world.self().pos();
    Vector2D self_vel = world.self().vel();

    for ( int i = 0; i < MAX_DEPTH; ++i )
    {
        M_state_cache[i].clear();

        self_pos += self_vel;
        self_vel *= self_type.playerDecay();

        int index = 0;
        for ( int near = 0; near < STATE_DIVS_NEAR; ++near )
        {
            Vector2D pos = M_state_list[index].pos_;
            double krate = self_type.kickRate( near_dist, pos.th().degree() );

            pos.rotate( world.self().body() );
            pos.setLength( near_dist );
            pos += self_pos;

            M_state_cache[i].push_back( State( index, near_dist, pos, krate ) );
            checkInterfereAt( world, i + 1, M_state_cache[i].back() );
            if ( ! pitch.contains( pos ) )
            {
                M_state_cache[i].back().flag_ |= OUT_OF_PITCH;
            }
            ++index;
        }

        for ( int mid = 0; mid < STATE_DIVS_MID; ++mid )
        {
            Vector2D pos = M_state_list[index].pos_;
            double krate = self_type.kickRate( mid_dist, pos.th().degree() );

            pos.rotate( world.self().body() );
            pos.setLength( mid_dist );
            pos += self_pos;

            M_state_cache[i].push_back( State( index, mid_dist, pos, krate ) );
            checkInterfereAt( world, i + 1, M_state_cache[i].back() );
            if ( ! pitch.contains( pos ) )
            {
                M_state_cache[i].back().flag_ |= OUT_OF_PITCH;
            }
            ++index;
        }

        for ( int far = 0; far < STATE_DIVS_FAR; ++far )
        {
            Vector2D pos = M_state_list[index].pos_;
            double krate = self_type.kickRate( far_dist, pos.th().degree() );

            pos.rotate( world.self().body() );
            pos.setLength( far_dist );
            pos += self_pos;

            M_state_cache[i].push_back( State( index, far_dist, pos, krate ) );
            checkInterfereAt( world, i + 1, M_state_cache[i].back() );
            if ( ! pitch.contains( pos ) )
            {
                M_state_cache[i].back().flag_ |= OUT_OF_PITCH;
            }
            ++index;
        }

    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::checkCollisionAfterRelease( const WorldModel & world,
                                       const Vector2D & target_point,
                                       const double & first_speed )
{

    const PlayerType & self_type = world.self().playerType();

    const double collide_dist2 = std::pow( self_type.playerSize() + ServerParam::i().ballSize(), 2 );

    Vector2D self_pos = world.self().pos();
    Vector2D self_vel = world.self().vel();

    // check the release kick from current state

    self_pos += self_vel;
    self_vel *= self_type.playerDecay();

    {
        Vector2D release_pos = ( target_point - M_current_state.pos_ );
        release_pos.setLength( first_speed );

        if ( self_pos.dist2( release_pos ) < collide_dist2 )
        {
            M_current_state.flag_ |= SELF_COLLISION;
        }
        else
        {
            M_current_state.flag_ &= ~SELF_COLLISION;
        }
    }

    // check the release kick from future state

    for ( int i = 0; i < MAX_DEPTH; ++ i )
    {
        self_pos += self_vel;
        self_vel *= self_type.playerDecay();

        const std::vector< State >::iterator end = M_state_cache[i].end();
        for ( std::vector< State >::iterator it = M_state_cache[i].begin();
              it != end;
              ++it )
        {
			//if( !(*it) ) continue;
            Vector2D release_pos = ( target_point - it->pos_ );
            release_pos.setLength( first_speed );

            if ( self_pos.dist2( release_pos ) < collide_dist2 )
            {
                it->flag_ |= SELF_COLLISION;
            }
            else
            {
                it->flag_ &= ~SELF_COLLISION;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::checkInterfereAt( const WorldModel & world,
                             const int /*cycle*/,
                             State & state )
{
    static const Rect2D penalty_area( Vector2D( ServerParam::i().theirPenaltyAreaLineX(),
                                                - ServerParam::i().penaltyAreaHalfWidth() ),
                                      Size2D( ServerParam::i().penaltyAreaLength(),
                                              ServerParam::i().penaltyAreaWidth() ) );

    int flag = 0x0000;

    const PlayerPtrCont::const_iterator end = world.opponentsFromBall().end();
    for ( PlayerPtrCont::const_iterator o = world.opponentsFromBall().begin();
          o != end;
          ++o )
    {
		if( !(*o) ) continue;
        if ( (*o)->posCount() >= 8 ) continue;
        if ( (*o)->isGhost() ) continue;
        if ( (*o)->distFromBall() > 10.0 ) break;

        const Vector2D opp_next = (*o)->pos() + (*o)->vel();
        const double opp_dist = opp_next.dist( state.pos_ );

        if ( (*o)->isTackling() )
        {
            if ( opp_dist < ( (*o)->playerTypePtr()->playerSize()
                              + ServerParam::i().ballSize() )
                 )
            {
                flag |= KICKABLE;
                break;
            }

            continue;
        }

        const double control_area = ( ( (*o)->goalie()
                                        && penalty_area.contains( (*o)->pos() )
                                        && penalty_area.contains( state.pos_ ) )
                                      ? ServerParam::i().catchableArea()
                                      : (*o)->playerTypePtr()->kickableArea() );
        //
        // check kick possibility
        //

        if ( ! (*o)->isGhost()
             && (*o)->posCount() <= 2
             && opp_dist < control_area + 0.15 )
        {
            flag |= KICKABLE;
            break;
        }

        //
        //
        //
        const AngleDeg opp_body =  ( (*o)->bodyCount() <= 1
                                     ? (*o)->body()
                                     : ( state.pos_ - opp_next ).th() );
        Vector2D player_2_pos = state.pos_ - opp_next;
        player_2_pos.rotate( - opp_body );

        //
        // check tackle possibility
        //
        {
            double tackle_dist = ( player_2_pos.x > 0.0
                                   ? ServerParam::i().tackleDist()
                                   : ServerParam::i().tackleBackDist() );
            if ( tackle_dist > 1.0e-5 )
            {
                double tackle_prob = ( std::pow( player_2_pos.absX() / tackle_dist,
                                                 ServerParam::i().tackleExponent() )
                                       + std::pow( player_2_pos.absY() / ServerParam::i().tackleWidth(),
                                                   ServerParam::i().tackleExponent() ) );
                if ( tackle_prob < 1.0
                     && 1.0 - tackle_prob > 0.7 ) // success probability
                {
                    flag |= TACKLABLE;
                }
            }
        }

        // check kick or tackle possibility after dash

        const PlayerType * player_type = (*o)->playerTypePtr();
        const double max_accel = ( ServerParam::i().maxDashPower()
                                   * player_type->dashPowerRate()
                                   * player_type->effortMax() );

        if ( player_2_pos.absY() < control_area
             && ( player_2_pos.absX() < max_accel
                  || ( player_2_pos + Vector2D( max_accel, 0.0 ) ).r() < control_area
                  || ( player_2_pos - Vector2D( max_accel, 0.0 ) ).r() < control_area )
             )
        {
            flag |= NEXT_KICKABLE;
        }
        else if ( player_2_pos.absY() < ServerParam::i().tackleWidth() * 0.7
                  && player_2_pos.x > 0.0
                  && player_2_pos.x - max_accel < ServerParam::i().tackleDist() - 0.3 )
        {
            flag |= NEXT_TACKLABLE;
        }
    }

    state.flag_ = flag;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::checkInterfereAfterRelease( const WorldModel & world,
                                       const Vector2D & target_point,
                                       const double & first_speed )
{
    checkInterfereAfterRelease( world, target_point, first_speed, 1, M_current_state );

    for ( int i = 0; i < MAX_DEPTH; ++i )
    {
        const std::vector< State >::iterator end = M_state_cache[i].end();
        for ( std::vector< State >::iterator state = M_state_cache[i].begin();
              state != end;
              ++state )
        {
			//if( !(*state) ) continue;
            state->flag_ &= ~RELEASE_INTERFERE;
            state->flag_ &= ~MAYBE_RELEASE_INTERFERE;

            checkInterfereAfterRelease( world, target_point, first_speed, i + 2, *state );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::checkInterfereAfterRelease( const WorldModel & world,
                                       const Vector2D & target_point,
                                       const double & first_speed,
                                       const int cycle,
                                       State & state )
{
    static const Rect2D penalty_area( Vector2D( ServerParam::i().theirPenaltyAreaLineX(),
                                                - ServerParam::i().penaltyAreaHalfWidth() ),
                                      Size2D( ServerParam::i().penaltyAreaLength(),
                                              ServerParam::i().penaltyAreaWidth() ) );

    Vector2D ball_pos = target_point - state.pos_;
    ball_pos.setLength( first_speed );
    ball_pos += state.pos_;

    const PlayerPtrCont::const_iterator o_end = world.opponentsFromBall().end();
    for ( PlayerPtrCont::const_iterator o = world.opponentsFromBall().begin();
          o != o_end;
          ++o )
    {
		if( (*o) == NULL ) continue;
        if ( (*o)->posCount() >= 8 ) continue;
        if ( (*o)->isGhost() ) continue;
        if ( (*o)->distFromBall() > 10.0 ) break;

        Vector2D opp_pos = (*o)->inertiaPoint( cycle );
        if ( ! opp_pos.isValid() )
        {
            opp_pos = (*o)->pos() + (*o)->vel();
        }

        if ( (*o)->isTackling() )
        {
            if ( opp_pos.dist( ball_pos ) < ( (*o)->playerTypePtr()->playerSize()
                                              + ServerParam::i().ballSize() )
                 )
            {
                state.flag_ |= RELEASE_INTERFERE;
            }

            continue;
        }

        double control_area = ( ( (*o)->goalie()
                                  && penalty_area.contains( opp_pos )
                                  && penalty_area.contains( ball_pos ) )
                                ? ServerParam::i().catchableArea()
                                : (*o)->playerTypePtr()->kickableArea() );
        control_area += 0.1;
        double control_area2 = std::pow( control_area, 2 );

        if ( ball_pos.dist2( opp_pos ) < control_area2 )
        {
            if ( cycle <= 1 )
            {
                state.flag_ |= RELEASE_INTERFERE;
            }
            else
            {
                //state.flag_ |= MAYBE_RELEASE_INTERFERE;
                state.flag_ |= RELEASE_INTERFERE;
            }
        }
#if 1
        else //if ( cycle <= 1 )
        {
            const AngleDeg opp_body =  ( (*o)->bodyCount() <= 1
                                     ? (*o)->body()
                                     : ( ball_pos - opp_pos ).th() );
            Vector2D player_2_pos = ball_pos - opp_pos;
            player_2_pos.rotate( - opp_body );

            {
                double tackle_dist = ( player_2_pos.x > 0.0
                                       ? ServerParam::i().tackleDist()
                                       : ServerParam::i().tackleBackDist() );
                if ( tackle_dist > 1.0e-5 )
                {
                    double tackle_prob = ( std::pow( player_2_pos.absX() / tackle_dist,
                                                     ServerParam::i().tackleExponent() )
                                           + std::pow( player_2_pos.absY() / ServerParam::i().tackleWidth(),
                                                       ServerParam::i().tackleExponent() ) );
                    if ( tackle_prob < 1.0
                         && 1.0 - tackle_prob > 0.8 ) // success probability
                    {
                        state.flag_ |= MAYBE_RELEASE_INTERFERE;
                    }
                }
            }

            //if ( (*o)->bodyCount() <= 1 )
            {
                const PlayerType * player_type = (*o)->playerTypePtr();
                const double max_accel
                    = ServerParam::i().maxDashPower()
                    * player_type->dashPowerRate()
                    * player_type->effortMax()
                    * 0.8;

                if ( player_2_pos.absY() < control_area - 0.1
                     && ( player_2_pos.absX() < max_accel
                          || ( player_2_pos + Vector2D( max_accel, 0.0 ) ).r() < control_area - 0.25
                          || ( player_2_pos - Vector2D( max_accel, 0.0 ) ).r() < control_area - 0.25 )
                     )
                {
                    state.flag_ |= MAYBE_RELEASE_INTERFERE;
                }
                else if ( player_2_pos.absY() < ServerParam::i().tackleWidth() * 0.7
                          && player_2_pos.x - max_accel < ServerParam::i().tackleDist() - 0.5 )
                {
                    state.flag_ |= MAYBE_RELEASE_INTERFERE;
                }
            }
        }
#endif
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::simulateOneStep( const WorldModel & world,
                            const Vector2D & target_point,
                            const double & first_speed )
{
    if ( M_current_state.flag_ & SELF_COLLISION )
    {
        return false;
    }

    if ( M_current_state.flag_ & RELEASE_INTERFERE )
    {
        return false;
    }

    const double current_max_accel = std::min( M_current_state.kick_rate_ * ServerParam::i().maxPower(),
                                               ServerParam::i().ballAccelMax() );
    Vector2D target_vel = ( target_point - world.ball().pos() );
    target_vel.setLength( first_speed );

    Vector2D accel = target_vel - world.ball().vel();
    double accel_r = accel.r();
    if ( accel_r > current_max_accel )
    {
        Vector2D max_vel = calc_max_velocity( target_vel.th(),
                                              M_current_state.kick_rate_,
                                              world.ball().vel() );
        accel = max_vel - world.ball().vel();
        M_candidates.push_back( Sequence() );
        M_candidates.back().flag_ = M_current_state.flag_;
        M_candidates.back().pos_list_.push_back( world.ball().pos() + max_vel );
        M_candidates.back().speed_ = max_vel.r();
        M_candidates.back().power_ = accel.r() / M_current_state.kick_rate_;
        return false;
    }

    M_candidates.push_back( Sequence() );
    M_candidates.back().flag_ = M_current_state.flag_;
    M_candidates.back().pos_list_.push_back( world.ball().pos() + target_vel );
    M_candidates.back().speed_ = first_speed;
    M_candidates.back().power_ = accel_r / M_current_state.kick_rate_;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::simulateTwoStep( const WorldModel & world,
                            const Vector2D & target_point,
                            const double & first_speed )
{
    static const double max_power = ServerParam::i().maxPower();
    static const double accel_max = ServerParam::i().ballAccelMax();
    static const double ball_decay = ServerParam::i().ballDecay();

    const PlayerType & self_type = world.self().playerType();
    const double current_max_accel = std::min( M_current_state.kick_rate_ * max_power, accel_max );
#if 1
    const ServerParam & param = ServerParam::i();
    const double my_kickable_area = self_type.kickableArea();

    const double my_noise = world.self().vel().r() * param.playerRand();
    const double current_dir_diff_rate
        = ( world.ball().angleFromSelf() - world.self().body() ).abs() / 180.0;
    const double current_dist_rate = ( ( world.ball().distFromSelf()
                                         - self_type.playerSize()
                                         - param.ballSize() )
                                       / self_type.kickableMargin() );
    const double current_pos_rate
        = 0.5 + 0.25 * ( current_dir_diff_rate + current_dist_rate );
    const double current_speed_rate
        = 0.5 + 0.5 * ( world.ball().vel().r()
                        / ( param.ballSpeedMax() * param.ballDecay() ) );
#endif
    const Vector2D my_final_pos
        = world.self().pos()
        + world.self().vel()
        + world.self().vel() * self_type.playerDecay();

    int success_count = 0;
    double max_speed2 = 0.0;

    for ( int i = 0; i < NUM_STATE; ++i )
    {
        const State & state = M_state_cache[0][i];

        if ( state.flag_ & OUT_OF_PITCH )
        {
            continue;
        }

        if ( state.flag_ & KICKABLE )
        {
            continue;
        }

        if ( state.flag_ & SELF_COLLISION )
        {
            continue;
        }

        if ( state.flag_ & RELEASE_INTERFERE )
        {
            return false;
        }

        int kick_miss_flag = SAFETY;
        const Vector2D target_vel = ( target_point - state.pos_ ).setLengthVector( first_speed );

        Vector2D vel = state.pos_ - world.ball().pos();
        Vector2D accel = vel - world.ball().vel();
        double accel_r = accel.r();

        if ( accel_r > current_max_accel )
        {
            continue;
        }
#if 1
        {
            double kick_power = accel_r / world.self().kickRate();
            double ball_noise = vel.r() * param.ballRand();
            double max_kick_rand
                = self_type.kickRand()
                * ( kick_power / param.maxPower() )
                * ( current_pos_rate + current_speed_rate );
            if ( ( my_noise + ball_noise + max_kick_rand ) //* 0.9
                 > my_kickable_area - state.dist_ - 0.05 ) //0.1 )
            {
                kick_miss_flag |= KICK_MISS_POSSIBILITY;
            }
        }
#endif
        vel *= ball_decay;

        accel = target_vel - vel;
        accel_r = accel.r();

        if ( accel_r > std::min( state.kick_rate_ * max_power, accel_max ) )
        {
            if ( success_count == 0 )
            {
                Vector2D max_vel = calc_max_velocity( target_vel.th(),
                                                      state.kick_rate_,
                                                      vel );
                double d2 = max_vel.r2();
                if ( max_speed2 < d2 )
                {
                    if ( max_speed2 == 0.0 )
                    {
                        M_candidates.push_back( Sequence() );
                    }
                    max_speed2 = d2;
                    accel = max_vel - vel;

                    M_candidates.back().flag_ = ( ( M_current_state.flag_ & ~RELEASE_INTERFERE )
                                                  | state.flag_ );
                    M_candidates.back().pos_list_.clear();
                    M_candidates.back().pos_list_.push_back( state.pos_ );
                    M_candidates.back().pos_list_.push_back( state.pos_ + max_vel );
                    M_candidates.back().speed_ = std::sqrt( max_speed2 );
                    M_candidates.back().power_ = accel.r() / state.kick_rate_;
                }
            }
            continue;
        }

        M_candidates.push_back( Sequence() );
        M_candidates.back().flag_ = ( ( M_current_state.flag_ & ~RELEASE_INTERFERE )
                                      | state.flag_
                                      | kick_miss_flag );
        M_candidates.back().pos_list_.push_back( state.pos_ );
        M_candidates.back().pos_list_.push_back( state.pos_ + target_vel );
        M_candidates.back().speed_ = first_speed;
        M_candidates.back().power_ = accel_r / state.kick_rate_;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::simulateThreeStep( const WorldModel & world,
                              const Vector2D & target_point,
                              const double & first_speed )
{
    static const double max_power = ServerParam::i().maxPower();
    static const double accel_max = ServerParam::i().ballAccelMax();
    static const double ball_decay = ServerParam::i().ballDecay();

    const double current_max_accel = std::min( M_current_state.kick_rate_ * max_power,
                                               accel_max );
    const double current_max_accel2 = current_max_accel * current_max_accel;
#if 1
    const ServerParam & param = ServerParam::i();
    const PlayerType & self_type = world.self().playerType();

    const double my_kickable_area = self_type.kickableArea();

    const double my_noise1 = world.self().vel().r() * param.playerRand();
    //const double my_noise2 = my_noise1 * self_type.playerDecay();
    const double current_dir_diff_rate
        = ( world.ball().angleFromSelf() - world.self().body() ).abs() / 180.0;
    const double current_dist_rate = ( ( world.ball().distFromSelf()
                                         - self_type.playerSize()
                                         - param.ballSize() )
                                       / self_type.kickableMargin() );
    const double current_pos_rate
        = 0.5 + 0.25 * ( current_dir_diff_rate + current_dist_rate );
    const double current_speed_rate
        = 0.5 + 0.5 * ( world.ball().vel().r()
                        / ( param.ballSpeedMax() * param.ballDecay() ) );
#endif
    AngleDeg target_rel_angle = ( target_point - world.self().pos() ).th() - world.self().body();
    double angle_deg = target_rel_angle.degree() + 180.0;
    int target_angle_index = static_cast< int >( rint( DEST_DIR_DIVS * ( angle_deg / 360.0 ) ) );
    if ( target_angle_index >= DEST_DIR_DIVS ) target_angle_index = 0;

    const std::vector< Path > & table = M_tables[target_angle_index];

    int success_count = 0;
    double max_speed2 = 0.0;

    int count = 0;
    const std::vector< Path >::const_iterator end = table.end();
    for ( std::vector< Path >::const_iterator it = table.begin();
          it != end && count < MAX_TABLE_SIZE && success_count <= 10;
          ++it, ++count )
    {
		//if( *it )
        const State & state_1st = M_state_cache[0][it->origin_];
        const State & state_2nd = M_state_cache[1][it->dest_];

        if ( state_1st.flag_ & OUT_OF_PITCH )
        {
            continue;
        }

        if ( state_2nd.flag_ & OUT_OF_PITCH )
        {
            continue;
        }

        if ( state_1st.flag_ & KICKABLE )
        {
            continue;
        }

        if ( state_2nd.flag_ & KICKABLE )
        {
            continue;
        }

        if ( state_2nd.flag_ & SELF_COLLISION )
        {
            continue;
        }

        if ( state_2nd.flag_ & RELEASE_INTERFERE )
        {
            return false;
        }


        const Vector2D target_vel = ( target_point - state_2nd.pos_ ).setLengthVector( first_speed );

        int kick_miss_flag = SAFETY;

        Vector2D vel1 = state_1st.pos_ - world.ball().pos();
        Vector2D accel = vel1 - world.ball().vel();
        double accel_r2 = accel.r2();

        if ( accel_r2 > current_max_accel2 )
        {
            continue;
        }

#if 1
        {
            double kick_power = std::sqrt( accel_r2 ) / world.self().kickRate();
            double ball_noise = vel1.r() * param.ballRand();
            double max_kick_rand
                = self_type.kickRand()
                * ( kick_power / param.maxPower() )
                * ( current_pos_rate + current_speed_rate );
            if ( ( my_noise1 + ball_noise + max_kick_rand ) //* 0.95
                 > my_kickable_area - state_1st.dist_ - 0.05 ) //0.1 )
            {
                kick_miss_flag |= KICK_MISS_POSSIBILITY;
            }
        }
#endif

        vel1 *= ball_decay;

        Vector2D vel2 = state_2nd.pos_ - state_1st.pos_;
        accel = vel2 - vel1;
        accel_r2 = accel.r2();

        if ( accel_r2 > square( std::min( state_1st.kick_rate_ * max_power, accel_max ) ) )
        {
            continue;
        }
        vel2 *= ball_decay;

        accel = target_vel - vel2;
        accel_r2 = accel.r2();
        if ( accel_r2 > square( std::min( state_2nd.kick_rate_ * max_power, accel_max ) ) )
        {
            if ( success_count == 0 )
            {
                Vector2D max_vel = calc_max_velocity( target_vel.th(),
                                                      state_2nd.kick_rate_,
                                                      vel2 );
                double d2 = max_vel.r2();
                if ( max_speed2 < d2 )
                {
                    if ( max_speed2 == 0.0 )
                    {
                        M_candidates.push_back( Sequence() );
                    }
                    max_speed2 = d2;
                    accel = max_vel - vel2;

                    M_candidates.back().flag_ = ( ( M_current_state.flag_ & ~RELEASE_INTERFERE )
                                                  | ( state_1st.flag_ & ~RELEASE_INTERFERE )
                                                  | state_2nd.flag_ );
                    M_candidates.back().pos_list_.clear();
                    M_candidates.back().pos_list_.push_back( state_1st.pos_ );
                    M_candidates.back().pos_list_.push_back( state_2nd.pos_ );
                    M_candidates.back().pos_list_.push_back( state_2nd.pos_ + max_vel );
                    M_candidates.back().speed_ = std::sqrt( max_speed2 );
                    M_candidates.back().power_ = accel.r() / state_2nd.kick_rate_;

                }
            }
            continue;
        }

        M_candidates.push_back( Sequence() );
        M_candidates.back().flag_ = ( ( M_current_state.flag_ & ~RELEASE_INTERFERE )
                                      | ( state_1st.flag_ & ~RELEASE_INTERFERE )
                                      | state_2nd.flag_
                                      | kick_miss_flag );
        M_candidates.back().pos_list_.push_back( state_1st.pos_ );
        M_candidates.back().pos_list_.push_back( state_2nd.pos_ );
        M_candidates.back().pos_list_.push_back( state_2nd.pos_ + target_vel );
        M_candidates.back().speed_ = first_speed;
        M_candidates.back().power_ = std::sqrt( accel_r2 ) / state_2nd.kick_rate_;

        ++success_count;
    }


    return success_count > 0;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
KickTable::evaluate( const double & first_speed,
                     const double & allowable_speed )
{

    const double power_thr1 = ServerParam::i().maxPower() * 0.94;
    const double power_thr2 = ServerParam::i().maxPower() * 0.9;

    const std::vector< Sequence >::iterator end = M_candidates.end();
    for ( std::vector< Sequence >::iterator it = M_candidates.begin();
          it != end;
          ++it )
    {
		//if( ! (*it) ) continue;
        const int n_kick = it->pos_list_.size();

        it->score_ = 1000.0;

        if ( it->speed_ < first_speed )
        {
            if ( n_kick > 1
                 || it->speed_ < allowable_speed )
            {
                it->score_ = -10000.0;
                it->score_ -= ( first_speed - it->speed_ ) * 100000.0;
            }
            else
            {
                it->score_ -= 50.0;
            }
        }

        if ( it->flag_ & TACKLABLE ) it->score_ -= 500.0;

        if ( it->flag_ & NEXT_TACKLABLE ) it->score_ -= 300.0;

        if ( it->flag_ & NEXT_KICKABLE ) it->score_ -= 600.0;

        if ( it->flag_ & MAYBE_RELEASE_INTERFERE )
        {
            if ( n_kick == 1 )
            {
                it->score_ -= 250.0;
            }
            else
            {
                it->score_ -= 200.0;
            }
        }

        if ( n_kick == 3 )
        {
            it->score_ -= 200.0;
        }
        else if ( n_kick == 2 )
        {
            it->score_ -= 50.0;
        }

        if ( n_kick > 1 )
        {
            if ( it->power_ > power_thr1 )
            {
                it->score_ -= 75.0;
            }
            else if ( it->power_ > power_thr2 )
            {
                it->score_ -= 25.0;
            }
        }

        it->score_ -= it->power_ * 0.5;

        if ( it->flag_ & KICK_MISS_POSSIBILITY )
        {
            it->score_ -= 30.0;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
KickTable::simulate( const WorldModel & world,
                     const Vector2D & target_point,
                     const double & first_speed,
                     const double & allowable_speed,
                     const int max_step,
                     Sequence & sequence )
{
    if ( M_state_list.empty() )
    {
        return false;
    }

#ifdef DEBUG_PROFILE
    MSecTimer timer;
#endif

    double target_speed = bound( 0.0,
                                 first_speed,
                                 ServerParam::i().ballSpeedMax() );
    double speed_thr = bound( 0.0,
                              allowable_speed,
                              target_speed );

    M_candidates.clear();

    updateState( world );

    checkCollisionAfterRelease( world,
                                target_point,
                                target_speed );
    checkInterfereAfterRelease( world,
                                target_point,
                                target_speed );

    if ( max_step >= 1
         && simulateOneStep( world,
                             target_point,
                             target_speed ) )
    {
    }

    if ( max_step >= 2
         && simulateTwoStep( world,
                             target_point,
                             target_speed ) )
    {
    }

    if ( max_step >= 3
         && simulateThreeStep( world,
                               target_point,
                               target_speed ) )
    {
    }

    // TODO:
    // 4 steps simulation

    // TODO:
    // dynamic evaluator
    evaluate( target_speed, speed_thr );

    if ( M_candidates.empty() )
    {
        return false;
    }

    sequence = *std::max_element( M_candidates.begin(),
                                  M_candidates.end(),
                                  SequenceCmp() );

#ifdef DEBUG_PROFILE
#endif
    return sequence.speed_ >= target_speed - rcsc::EPS;
}

}

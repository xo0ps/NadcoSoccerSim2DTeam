// -*-c++-*-

/*!
  \file player_type.h
  \brief heterogenious player parametor Header File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA, Hiroki SHIMORA

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

#ifndef RCSC_PARAM_PLAYER_TYPE_H
#define RCSC_PARAM_PLAYER_TYPE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/rcg/types.h>
#include <rcsc/soccer_math.h>
#include <rcsc/types.h>

#include <map>
#include <vector>
#include <iostream>

namespace rcsc {


class ServerParam;

/*!
  \class PlayerType
  \brief heterogeneous player parametor class
 */
class PlayerType {
private:
    int M_id; //!< player type id
    double M_player_speed_max; //!< maximum speed
    double M_stamina_inc_max; //!< stamina inc max
    double M_player_decay; //!< player decay
    double M_inertia_moment; //!< inertia moment
    double M_dash_power_rate; //!< dash power rate
    double M_player_size; //!< player radius
    double M_kickable_margin; //!< kickable area margin
    double M_kick_rand; //!< kick random
    double M_extra_stamina; //!< extra stamina value
    double M_effort_max; //!< maximum(initial) effort
    double M_effort_min; //!< minimum effort
    // v14
    double M_kick_power_rate; //!< kick power rate
    double M_foul_detect_probability; //!< foul detect probability
    double M_catchable_area_l_stretch; //!< catch area length stretch factor

    //
    // additional parameters
    //

    double M_kickable_area; //!< cached kickable area
    double M_reliable_catchable_dist; //!< cached reliable catchable dist
    double M_max_catchable_dist; //!< cached maximum catchable dist

    // if player's dprate & effort is not enough,
    // player never reach player_speed_max
    double M_real_speed_max;

    double M_player_speed_max2; // squared value
    double M_real_speed_max2;   // squared value

    //! dash cycles to reach max speed
    int M_cycles_to_reach_max_speed;

    //! distance table by continuous dashes from the velocity 0.
    std::vector< double > M_dash_distance_table;

    // stamina cconsumption table by continuous dashes
    //std::vector< double > M_stamina_table;

public:
    /*!
      \brief default constructor. create default type parameter using ServerParam
     */
    PlayerType();

    /*!
      \brief construct with message from rcssserver
      \param server_msg raw message from rcssserver
      \param version client version that determins message protocol
    */
    PlayerType( const char * server_msg,
                const double & version );

    /*!
      \brief construct with monitor protocol
      \param from monitor protocol data
     */
    explicit
    PlayerType( const rcg::player_type_t & from );

    /*!
      \brief conver to the monitor protocol format
      \param to reference to the data variable.
     */
    void convertTo( rcg::player_type_t & to ) const;

    /*!
      \brief convert to the rcss parameter message
      \return parameter message string
     */
    std::string toStr() const;

private:

    /*!
      \brief set default values by ServerParam.
     */
    void setDefault();

    /*!
      \brief analyze version 8 protocol server message
      \param msg raw message string from rcssserver
     */
    void parseV8( const char * msg );

    /*!
      \brief analyze version 7 protocol server message
      \param msg raw message string from rcssserver
     */
    void parseV7( const char * msg );

    /*!
      \brief set additional parameters
     */
    void initAdditionalParams();

public:

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    int id() const
      {
          return M_id;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & playerSpeedMax() const
      {
          return M_player_speed_max;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & staminaIncMax() const
      {
          return M_stamina_inc_max;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & playerDecay() const
      {
          return M_player_decay;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & inertiaMoment() const
      {
          return M_inertia_moment;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & dashPowerRate() const
      {
          return M_dash_power_rate;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & playerSize() const
      {
          return M_player_size;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & kickableMargin() const
      {
          return M_kickable_margin;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & kickRand() const
      {
          return M_kick_rand;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & extraStamina() const
      {
          return M_extra_stamina;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & effortMax() const
      {
          return M_effort_max;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & effortMin() const
      {
          return M_effort_min;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & kickPowerRate() const
      {
          return M_kick_power_rate;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & foulDetectProbability() const
      {
          return M_foul_detect_probability;
      }

    /*!
      \brief get the player_type parameter
      \return player_type parameter
     */
    const
    double & catchAreaLengthStretch() const
      {
          return M_catchable_area_l_stretch;
      }

    ////////////////////////////////////////////////
    // additional parameters

    /*!
      \brief get the maximum kickable radius
      \return maximum kickable radius
     */
    const
    double & kickableArea() const
      {
          return M_kickable_area;
      }

    /*!
      \brief get reliable catchable distance
      \return threshold distance not to fail
     */
    double reliableCatchableDist() const;

    /*!
      \brief get reliable catchable distance
      \param prob threshold of catch probability
      \return threshold distance that catch probability is greater equal to prob.
      if catch_probalibity server parameter is not 1, this method returns 0.0
     */
    double reliableCatchableDist( const double prob ) const;

    /*!
      \brief get catch probability
      \param dist distance of player and ball
      \return probability of successful catching
     */
    double getCatchProbability( const double dist ) const;

    /*!
      \brief get the maximum catchable distance
      \return maximum catchable distance
     */
    const
    double & maxCatchableDist() const
      {
          return M_max_catchable_dist;
      }

    /*!
      \brief get the reachable speed max
      \return reachable speed max
     */
    const
    double & realSpeedMax() const
      {
          return M_real_speed_max;
      }

    /*!
      \brief get the squared player speed max
      \return squared player speed max
     */
    const
    double & playerSpeedMax2() const
      {
          return M_player_speed_max2;
      }

    /*!
      \brief get the squared real speed max
      \return squared real speed max
     */
    const
    double & realSpeedMax2() const
      {
          return M_real_speed_max2;
      }

    /*!
      \brief get dash reachable distance table
      \return const reference to the distance table container
     */
    const
    std::vector< double > & dashDistanceTable() const
      {
          return M_dash_distance_table;
      }

    ////////////////////////////////////////////////
    /*!
      \brief calculate enable cycles to keep to dash using max power
      \param dash_power used dash power
      \param current_stamina current agent's stamina
      \param current_recovery current agent's recovery
      \return max cycles to keep same dash power
    */
    int getMaxDashCyclesSavingRecovery( const double & dash_power,
                                        const double & current_stamina,
                                        const double & current_recovery ) const;

    int getMaxDashCyclesSavingStamina( const ServerParam & sparam,
                                       const double & dash_power,
                                       const double & current_stamina,
                                       const double & current_recovery ) const;

    /*
      \brief estimate the number of available dashes with max power
      \param stamina available stamina
      \return estimated cycle
     */
    //int maxDashCyclesWith( const double & stamina ) const;

    /*
      \brief get the consumed stamina value after nr dashes with the max power
      from the velocity 0.
      \param n_dash dash count
      \return consumed stamina value

      this method can be used when player's recover is not decayed.
     */
    //double consumedStaminaAfterNrDash( const int n_dash ) const;

    ////////////////////////////////////////////////
    /*!
      \brief estimate cycles to reach max speed from zero.
      \param dash_power used dash power
      \return estimated cycles to reach.
    */
    int cyclesToReachMaxSpeed( const double & dash_power ) const;

    /*!
      \brief estimate cycles to reach max speed from zero using max dash power.
      \return estimated cycles to reach.

      returned value is calculated by initAdditionalParams()
    */
    int cyclesToReachMaxSpeed() const
      {
          return M_cycles_to_reach_max_speed;
      }
    ////////////////////////////////////////////////
    /*!
      \brief estimate cycles to reach the specific distance with start speed 0.
      \param dash_dist distance to reach
      \return estimated cycles to reach
    */
    int cyclesToReachDistance( const double & dash_dist ) const;
    ////////////////////////////////////////////////
    /*!
      \brief check if this type player can over player_speed_max
      \param dash_power used dash_power
      \param effort current effort value
      \return true if player has the potential to go over the max speed
     */
    bool canOverSpeedMax( const double & dash_power,
                          const double & effort ) const
      {
          return ( std::fabs( dash_power ) * dashPowerRate() * effort
                   > playerSpeedMax() * ( 1.0 - playerDecay() ) );
      }
    ////////////////////////////////////////////////
    /*!
      \brief estimate dash power to keep max speed
      \param effort current agent's effort
      \return conserved dash power
    */
    double getDashPowerToKeepMaxSpeed( const ServerParam & sparam,
                                       const double & effort ) const;

    /*!
      \brief estimate dash power to keep max speed with max effort value
      \return conserved dash power
    */
    double getDashPowerToKeepMaxSpeed( const ServerParam & sparam ) const
      {
          return getDashPowerToKeepMaxSpeed( sparam, effortMax() );
      }

    /*!
      \brief estimate dash power to keep the specified speed
      \param speed the desired speed
      \param effort current effort value
      \return estimated dash power, but not normalized
     */
    double getDashPowerToKeepSpeed( const double & speed,
                                    const double & effort ) const
      {
          return speed * ( ( 1.0 - playerDecay() )
                           / ( dashPowerRate() * effort ) );
      }

    /*!
      \brief estimate one cycle stamina comsumption to keep mas speed
      \return get the comsumed stamina value
     */

    double getOneStepStaminaComsumption( const ServerParam & sparam ) const
      {
          return getDashPowerToKeepMaxSpeed( sparam, effortMax() ) - staminaIncMax();
      }

    ////////////////////////////////////////////////
    /*!
      \brief calculate kick rate
      \param ball_dist ball distance from agent
      \param dir_diff ball angle difference from agent body angle
      \return kick rate value
    */
    double kickRate( const double & ball_dist,
                     const double & dir_diff ) const;

    /*!
      \brief calculate dash rate
      \param effort current effort value
      \return dash power rate multiplied by effort
     */
    double dashRate( const double & effort ) const
      {
          return effort * dashPowerRate();
      }

    /*!
      \brief calculate dash rate
      \param effort current effort value
      \param rel_dir dash direction
      \return dash power rate multiplied by effort
     */
    double dashRate( const double & effort,
                     const double & rel_dir ) const;

    /*!
      \brief calculate effective turn angle
      \param command_moment turn command argument
      \param speed current speed
      \return estimated result turn angle
     */
    double effectiveTurn( const double & command_moment,
                          const double & speed ) const
      {
          return command_moment / ( 1.0 + inertiaMoment() * speed );
      }

    /*!
      \brief calculate final reachable speed
      \param dash_power used dash power
      \param effort current effort
      \return maximal speed value with the specified params
     */
    double finalSpeed( const double & dash_power,
                       const double & effort ) const
      {
          return std::min( playerSpeedMax(),
                           ( ( std::fabs(dash_power) * dashPowerRate() * effort ) // == accel
                             / ( 1.0 - playerDecay() ) ) ); // sum inf geom series
      }
    ////////////////////////////////////////////////
    /*!
      \brief calculate inertia movement vector
      \param initial_vel initial velocity vector
      \param n_step cycles to be estimated
      \return total travel vector
     */
    Vector2D inertiaTravel( const Vector2D & initial_vel,
                            const int n_step ) const
      {
          return inertia_n_step_travel( initial_vel, n_step, playerDecay() );
      }

    /*!
      \brief calculate reach point
      \param initial_pos initial point
      \param initial_vel initial velocity vector
      \param n_step cycles to be estimated
      \return the reached point
     */
    Vector2D inertiaPoint( const Vector2D & initial_pos,
                           const Vector2D & initial_vel,
                           const int n_step ) const
      {
          return inertia_n_step_point( initial_pos,
                                       initial_vel,
                                       n_step,
                                       playerDecay() );
      }

    /*!
      \brief calculate total ineartia movement vector
      \param initial_vel initial velocity vector
      \return total travel vector when plyer stops
     */
    Vector2D inertiaFinalTravel( const Vector2D & initial_vel ) const
      {
          return inertia_final_travel( initial_vel, playerDecay() );
      }

    /*!
      \brief calculate final reach point
      \param initial_pos initial position
      \param initial_vel initial velocity vector
      \return the reached point when player stops
     */
    Vector2D inertiaFinalPoint( const Vector2D & initial_pos,
                                const Vector2D & initial_vel ) const
      {
          return inertia_final_point( initial_pos, initial_vel, playerDecay() );
      }

    ////////////////////////////////////////////////
    /*!
      \brief normalize accel range when try to new dash(accel_mag, accel_angle)
      \param velocity current agent's velocity
      \param accel_angle accel angle -> agent's body angle or reversed body angle.
      \param accel_mag pointer to accel magnitude variable
      \return true if normalized, false otherwise.
    */
    bool normalizeAccel( const Vector2D & velocity,
                         const AngleDeg & accel_angle,
                         double * accel_mag ) const;

    /*!
      \brief normalize accel range when try to new dash(accel)
      \param velocity current agent's velocity
      \param accel new accel
      \return true if normalized, false otherwise
    */
    bool normalizeAccel( const Vector2D & velocity,
                         Vector2D * accel ) const;

    /*!
      \brief output parameters to stream
      \param os reference to the output stream
      \return reference to the output stream
     */
    void predictStaminaAfterWait( const ServerParam & sparam,
                                  const int n_wait,
                                  double * stamina,
                                  double * effort,
                                  const double & recovery ) const;

    /*!
      \brief predict agent's stamina related values after one dash
      \param sparam server parameter
      \param dash_power used dash power
      \param stamina pointer to stamina variable
      \param effort pointer to effort variable
      \param recovery pointer to recovery variable
    */
    void predictStaminaAfterOneDash( const ServerParam & sparam,
                                     const double & dash_power,
                                     double * stamina,
                                     double * effort,
                                     double * recovery ) const;

    /*!
      \brief predict stamina related values after nr dashes
      \param sparam server parameter
      \param dash_power used dash power
      \param n_dash number of dash cycles
      \param stamina pointer to stamina variable
      \param effort pointer to effort variable
      \param recovery pointer to recovery variable
     */
    void predictStaminaAfterDashes( const ServerParam & sparam,
                                    const double & dash_power,
                                    const int n_dash,
                                    double * stamina,
                                    double * effort,
                                    double * recovery ) const;


    std::ostream & print( std::ostream & os ) const;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

/*!
  \class PlayerTypeSet
  \brief PlayerType parameter holder
*/
class PlayerTypeSet {
public:
    //! typedef of the player type contaier. key: id, value: player type
    typedef std::map< int, PlayerType > PlayerTypeMap;
private:
    //! map for hetero player id and parameter
    PlayerTypeMap M_player_type_map;

    //! dummy player type parameter
    PlayerType M_dummy_type;

    /*!
      \brief create dummy type. private access for singleton.
     */
    PlayerTypeSet();

public:
    /*!
      \brief destcut members
     */
    ~PlayerTypeSet();

    /*!
      \brief singleton interface. get player type set instance
      \return reference to the player type set instance
     */
    static
    PlayerTypeSet & instance();

    /*!
      \brief singleton interface. get player type set instance
      \return const reference to the player type set instance
     */
    inline
    static const
    PlayerTypeSet & i()
      {
          return instance();
      }

    /*!
      \brief regenerate default player type parameter using server param
     */
    void resetDefaultType();

    /*!
      \brief add new player type parameter
      \param param const reference to the new parameter object
     */
    void insert( const PlayerType & param );

private:
    /*!
      \brief regenerate dummy player type parameter
      using the most effective parameter in existing parameters

      Generated player type parameter will be the fastest type
     */
    void createDummyType();

public:

    /*!
      \brief get player type map
      \return const reference to the player type map object
     */
    const
    PlayerTypeMap & playerTypeMap() const
      {
          return M_player_type_map;
      }

    /*!
      \brief get player type parameter that Id is id
      \param id wanted player type Id
      \return const pointer to the player type parameter object
     */
    const
    PlayerType * get( const int id ) const;

    /*!
      \brief put parameters to the output stream
      \param os reference to the output stream
      \return reference to the output stream
     */
    std::ostream & print( std::ostream & os ) const;
};

}

#endif

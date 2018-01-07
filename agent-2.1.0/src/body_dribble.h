
/////////////////////////////////////////////////////////////////////

#ifndef BODY_DRIBBLE_H
#define BODY_DRIBBLE_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <vector>
using namespace rcsc;

class WorldModel;

class Body_Dribble
    : public BodyAction {
public:

    struct KeepDribbleInfo {
        Vector2D first_ball_vel_; //!< first ball velocity
        Vector2D last_ball_rel_;
        double ball_forward_travel_; //!< ball travel distance for the dribble target dir
        int dash_count_; //!< queued dash count
        double min_opp_dist_; //!< estimated distance to the nearest opponent while dribbling

        KeepDribbleInfo()
            : first_ball_vel_( 0.0, 0.0 )
            , last_ball_rel_( 0.0, 0.0 )
            , ball_forward_travel_( 0.0 )
            , dash_count_( 0 )
            , min_opp_dist_( 0.0 )
          { }
    };

private:
    //! target point to be reached
    const Vector2D M_target_point;
    //! distance threshond to the target point
    const double M_dist_thr;
    //! power parameter for dash command
    double M_dash_power;
    //! the number of dash command after kick
    int M_dash_count;
    //! switch that determines whether player agent avoid opponents or not
    const bool M_dodge_mode;
public:

    Body_Dribble( const Vector2D & target_point,
                      const double & dist_thr,
                      const double & dash_power,
                      const int dash_count,
                      const bool dodge = true )
        : M_target_point( target_point )
        , M_dist_thr( dist_thr )
        , M_dash_power( dash_power )
        , M_dash_count( dash_count )
        , M_dodge_mode( dodge )
      { }

    bool execute( PlayerAgent * agent );

    static bool isDribble( PlayerAgent * agent , double power );

private:

    void say( PlayerAgent * agent,
              const Vector2D & target_point,
              const int queu_count );

    bool doAction( PlayerAgent * agent,
                   const Vector2D & target_point,
                   const double & dash_power,
                   const int dash_count,
                   const bool dodge,
                   const bool dodge_mode = false );

    bool doTurn( PlayerAgent * agent,
                 const Vector2D & target_point,
                 const double & dash_power,
                 const int dash_count,
                 const bool dodge );


    bool doTurnOnly( PlayerAgent * agent,
                     const Vector2D & target_point,
                     const double & dash_power,
                     const double & dir_diff );

    bool doCollideWithBall( PlayerAgent * agent );

    bool doCollideForTurn( PlayerAgent * agent,
                           const double & dir_diff_abs,
                           const bool kick_first );

    bool doKickTurnsDash( PlayerAgent * agent,
                          const Vector2D & target_point,
                          const double & dash_power,
                          const double & dir_diff,
                          const double & dir_margin_abs );

    bool doKickTurnsDashes( PlayerAgent * agent,
                            const Vector2D & target_point,
                            const double & dash_power,
                            const int n_turn );

    bool doKickDashes( PlayerAgent * agent,
                       const Vector2D & target_point,
                       const double & dash_power,
                       const int dash_count );

    bool doKickDashesWithBall( PlayerAgent * agent,
                               const Vector2D & target_point,
                               const double & dash_power,
                               const int dash_count,
                               const bool dodge_mode );
    void createSelfCache( PlayerAgent * agent,
                          const Vector2D & target_point,
                          const double & dash_power,
                          const int turn_count,
                          const int dash_count,
                          std::vector< Vector2D > & self_cache );
                          
    bool simulateKickDashes( PlayerAgent * agent ,
                             const std::vector< Vector2D > & self_cache,
                             const int dash_count,
                             const AngleDeg & accel_angle,
                             const Vector2D & first_ball_pos,
                             const Vector2D & first_ball_vel,
                             KeepDribbleInfo * dribble_info );
                             
    bool existKickableOpponent( PlayerAgent * agent,
                                const Vector2D & ball_pos,
                                double * min_opp_dist ) const;

    bool doDodge( PlayerAgent * agent,
                  const Vector2D & target_point );

    bool doAvoidKick( PlayerAgent * agent,
                      const AngleDeg & avoid_angle );

    bool isDodgeSituation( const PlayerAgent * agent,
                           const Vector2D & target_point );

    bool canKickAfterDash( const PlayerAgent * agent,
                           double * dash_power );

    bool existCloseOpponent( const PlayerAgent * agent,
                             AngleDeg * keep_angle );

    AngleDeg getAvoidAngle( const PlayerAgent * agent,
                            const AngleDeg & target_angle );

};


#endif

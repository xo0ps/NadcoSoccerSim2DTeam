
/////////////////////////////////////////////////////////////////////

#ifndef BHV_PREDICTOR_H
#define BHV_PREDICTOR_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <vector>
#include <rcsc/player/abstract_player_object.h>

namespace rcsc {
class AbstractPlayerObject;
class WorldModel;
}

class Bhv_Predictor {

public:


	static
    bool is_ball_moving_to_our_goal( const rcsc::Vector2D & ball_pos,
                                     const rcsc::Vector2D & ball_vel,
                                     const double & post_buffer );
    static
    int estimate_min_reach_cycle( const rcsc::Vector2D & player_pos,
                                  const double & player_speed_max,
                                  const rcsc::Vector2D & target_point,
                                  const rcsc::AngleDeg & target_move_angle );
    static
    int predict_player_turn_cycle( const rcsc::PlayerType * player_type,
                                   const rcsc::AngleDeg & player_body,
                                   const double & player_speed,
                                   const double & target_dist,
                                   const rcsc::AngleDeg & target_angle,
                                   const double & dist_thr,
                                   const bool use_back_dash );
	static
	int predict_player_reach_cycle( const rcsc::AbstractPlayerObject * player,
                                    const rcsc::Vector2D & target_point,
                                    const double & dist_thr,
                                    const double & penalty_distance,
                                    const int body_count_thr,
                                    const int default_n_turn,
                                    const int wait_cycle,
                                    const bool use_back_dash );

    static
    int predictOpponentsReachStep( const rcsc::WorldModel & wm,
                                   const rcsc::Vector2D & first_ball_pos,
                                   const rcsc::Vector2D & first_ball_vel,
                                   const rcsc::AngleDeg & ball_move_angle );
    static
    int predictOpponentReachStep( const rcsc::AbstractPlayerObject * opponent,
                                  const rcsc::Vector2D & first_ball_pos,
                                  const rcsc::Vector2D & first_ball_vel,
                                  const rcsc::AngleDeg & ball_move_angle,
                                  const int max_cycle );

};

#endif

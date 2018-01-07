
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SET_PLAY_H
#define BHV_SET_PLAY_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/world_model.h>

class Bhv_SetPlay
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;
public:
    Bhv_SetPlay( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

    static
    double get_set_play_dash_power( const rcsc::PlayerAgent * agent );

	static bool
	is_delaying_tactics_situation( const rcsc::PlayerAgent * agent );

    static
    bool is_kicker( const rcsc::PlayerAgent * agent );

    static
    rcsc::Vector2D get_avoid_circle_point2( const rcsc::WorldModel & world,
                                           const rcsc::Vector2D & target_point );


private:
    void doBasicTheirSetPlayMove( rcsc::PlayerAgent * agent,
                                  const rcsc::Vector2D & target_point , 
                                  bool mark );

};

#endif

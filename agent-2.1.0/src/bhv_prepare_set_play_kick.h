
/////////////////////////////////////////////////////////////////////

#ifndef BHV_PREPARE_SET_PLAY_KICK_H
#define BHV_PREPARE_SET_PLAY_KICK_H

#include <rcsc/geom/angle_deg.h>
#include <rcsc/player/soccer_action.h>

class Bhv_PrepareSetPlayKick
    : public rcsc::SoccerBehavior {
private:
    const rcsc::AngleDeg M_ball_place_angle; // global angle from self final kick position
    const int M_wait_cycle;

public:
    Bhv_PrepareSetPlayKick( const rcsc::AngleDeg & ball_place_angle,
                            const int wait_cycle )
        : M_ball_place_angle( ball_place_angle )
        , M_wait_cycle( wait_cycle )
      { }

    bool execute( rcsc::PlayerAgent * agent );


private:
    bool doGoToStaticBall( rcsc::PlayerAgent * agent );

};

#endif

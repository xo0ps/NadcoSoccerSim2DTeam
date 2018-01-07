
/////////////////////////////////////////////////////////////////////

#ifndef BHV_GO_TO_STATIC_BALL_H
#define BHV_GO_TO_STATIC_BALL_H

#include <rcsc/geom/angle_deg.h>
#include <rcsc/player/soccer_action.h>

class Bhv_GoToStaticBall
    : public rcsc::SoccerBehavior {
private:
    const rcsc::AngleDeg M_ball_place_angle; // global angle from self final kick position

public:
    Bhv_GoToStaticBall( const rcsc::AngleDeg & ball_place_angle )
        : M_ball_place_angle( ball_place_angle )
      { }

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

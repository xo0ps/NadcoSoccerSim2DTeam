
/////////////////////////////////////////////////////////////////////

#ifndef BHV_GOALIE_CHASE_BALL_H
#define BHV_GOALIE_CHASE_BALL_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_GoalieChaseBall
    : public rcsc::SoccerBehavior {
private:


public:

    bool execute( rcsc::PlayerAgent * agent );

    static
    bool is_ball_chase_situation( const rcsc::PlayerAgent * agent );
    static
    bool is_ball_shoot_moving( const rcsc::PlayerAgent * agent );

private:
    void doGoToCatchPoint( rcsc::PlayerAgent * agent,
                           const rcsc::Vector2D & target_point );

};

#endif

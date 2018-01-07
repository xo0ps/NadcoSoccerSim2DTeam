
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BEFORE_KICK_OFF_H
#define BHV_BEFORE_KICK_OFF_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_BeforeKickOff
    : public rcsc::SoccerBehavior {
private:
    //! target position for move command
    rcsc::Vector2D M_move_point;
public:
    explicit
    Bhv_BeforeKickOff( const rcsc::Vector2D & point )
        : M_move_point( point )
      { }

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

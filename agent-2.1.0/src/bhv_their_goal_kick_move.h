
/////////////////////////////////////////////////////////////////////

#ifndef BHV_THEIR_GOAL_KICK_MOVE_H
#define BHV_THEIR_GOAL_KICK_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_TheirGoalKickMove
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;
public:
    Bhv_TheirGoalKickMove( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    void doNormal( rcsc::PlayerAgent * agent );
    bool doChaseBall( rcsc::PlayerAgent * agent );
};

#endif

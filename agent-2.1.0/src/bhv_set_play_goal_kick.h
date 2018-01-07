
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SET_PLAY_GOAL_KICK_H
#define BHV_SET_PLAY_GOAL_KICK_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

// our goal kick

class Bhv_SetPlayGoalKick
    : public rcsc::SoccerBehavior {
private:
    static rcsc::GameTime S_last_kick_off_start_time;

    const rcsc::Vector2D M_home_pos;

public:
    Bhv_SetPlayGoalKick( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool isKicker( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
    bool doSecondKick( rcsc::PlayerAgent * agent );
    bool doKickWait( rcsc::PlayerAgent * agent );
    bool doPass( rcsc::PlayerAgent * agent );
    bool doKickToFarSide( rcsc::PlayerAgent * agent );
    bool doIntercept( rcsc::PlayerAgent * agent );
};

#endif

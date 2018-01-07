
/////////////////////////////////////////////////////////////////////

#ifndef AGENT2D_BHV_SET_PLAY_KICK_OFF_H
#define AGENT2D_BHV_SET_PLAY_KICK_OFF_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

// our kick off

class Bhv_SetPlayKickOff
    : public rcsc::SoccerBehavior {
private:
    static rcsc::GameTime S_last_kick_off_start_time;

    const rcsc::Vector2D M_home_pos;

public:
    Bhv_SetPlayKickOff( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool isKicker( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};

#endif

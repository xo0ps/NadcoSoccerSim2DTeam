
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SET_PLAY_FREE_KICK_H
#define BHV_SET_PLAY_FREE_KICK_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

// our free kick

class Bhv_SetPlayFreeKick
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;

public:
    Bhv_SetPlayFreeKick( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool isKicker( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
    bool doKickWait( rcsc::PlayerAgent * agent );

};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef BHV_SET_PLAY_INDIRECT_FREE_KICK_H
#define BHV_SET_PLAY_INDIRECT_FREE_KICK_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_SetPlayIndirectFreeKick
    : public rcsc::SoccerBehavior {
public:

    Bhv_SetPlayIndirectFreeKick()
      { }

    bool execute( rcsc::PlayerAgent * agent );

    static void doOffenseMove( rcsc::PlayerAgent * agent );
    static void doDefenseMove( rcsc::PlayerAgent * agent );

private:

    void doKicker( rcsc::PlayerAgent * agent );
    bool doKickWait( rcsc::PlayerAgent * agent );
    bool doKickToShooter( rcsc::PlayerAgent * agent );
};

#endif

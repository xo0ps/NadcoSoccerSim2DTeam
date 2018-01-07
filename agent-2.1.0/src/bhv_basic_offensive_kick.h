
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BASIC_OFFENSIVE_KICK_H
#define BHV_BASIC_OFFENSIVE_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_BasicOffensiveKick
    : public rcsc::SoccerBehavior {
private:

public:

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

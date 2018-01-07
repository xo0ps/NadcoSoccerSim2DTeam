
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DANGER_KICK_H
#define BHV_DANGER_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_DangerKick
    : public rcsc::SoccerBehavior {
private:

public:

	static bool doDefensiveKick( rcsc::PlayerAgent * agent );
    bool execute( rcsc::PlayerAgent * agent );
};

#endif

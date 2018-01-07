
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SHOOTCHANCE_KICK_H
#define BHV_SHOOTCHANCE_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_ShootChanceKick
    : public rcsc::SoccerBehavior {
private:
	
	bool doShootChance( rcsc::PlayerAgent * agent );
	
public:

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

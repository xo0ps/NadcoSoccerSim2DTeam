
/////////////////////////////////////////////////////////////////////

#ifndef BHV_OFFMID_DEFMID_KICK_H
#define BHV_OFFMID_DEFMID_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_OffmidDefmidKick
    : public rcsc::SoccerBehavior {
private:

	void offensivePlan( rcsc::PlayerAgent * agent );
	bool RC( rcsc::PlayerAgent * agent );
	
public:

	static bool doMiddleAreaKick( rcsc::PlayerAgent * agent );

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

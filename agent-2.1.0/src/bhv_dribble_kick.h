
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DRIBBLE_KICK_H
#define BHV_DRIBBLE_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_DribbleKick
    : public rcsc::SoccerBehavior {

private:

	void offensivePlan( rcsc::PlayerAgent * agent );
	bool RC( rcsc::PlayerAgent * agent );

public:

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

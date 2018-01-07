
/////////////////////////////////////////////////////////////////////

#ifndef BHV_CROSS_KICK_H
#define BHV_CROSS_KICK_H

#include <rcsc/player/soccer_action.h>


class Bhv_CrossKick
    : public rcsc::SoccerBehavior {
private:

	void offensivePlan( rcsc::PlayerAgent * agent );
	bool RC( rcsc::PlayerAgent * agent );
	
public:

    bool execute( rcsc::PlayerAgent * agent );
};

#endif

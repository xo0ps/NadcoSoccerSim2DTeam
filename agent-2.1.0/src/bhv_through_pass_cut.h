
/////////////////////////////////////////////////////////////////////

#ifndef BHV_THROUGH_PASS_CUT_H
#define BHV_THROUGH_PASS_CUT_H

#include <rcsc/player/soccer_action.h>

class Bhv_ThroughPassCut
    : public rcsc::SoccerBehavior {

public:
    Bhv_ThroughPassCut( )
		{ }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif

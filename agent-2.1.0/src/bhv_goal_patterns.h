
/////////////////////////////////////////////////////////////////////

#ifndef BHV_GOAL_PATTERNS_H
#define BHV_GOAL_PATTERNS_H

#include <rcsc/player/soccer_action.h>

class Bhv_GoalPatterns
    : public rcsc::SoccerBehavior {

public:
    Bhv_GoalPatterns( )
		{ }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif


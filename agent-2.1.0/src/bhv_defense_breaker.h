
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DEFENSE_BREAKER_H
#define BHV_DEFENSE_BREAKER_H

#include <rcsc/player/soccer_action.h>

class Bhv_DefenseBreaker
    : public rcsc::SoccerBehavior {
private:

public:
    Bhv_DefenseBreaker( )
      { }

    bool execute( rcsc::PlayerAgent * agent );
    
};

#endif


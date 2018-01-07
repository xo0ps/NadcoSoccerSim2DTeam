
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DANGER_AREA_TACKLE_H
#define BHV_DANGER_AREA_TACKLE_H

#include <rcsc/player/soccer_action.h>

class Bhv_DangerAreaTackle
    : public rcsc::SoccerBehavior {
private:
    const double M_min_probability;
public:
    Bhv_DangerAreaTackle( const double & min_prob = 0.85 )
        : M_min_probability( min_prob )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool clearGoal( rcsc::PlayerAgent * agent );
    bool executeV12( rcsc::PlayerAgent * agent );

};

#endif

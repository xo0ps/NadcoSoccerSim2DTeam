
/////////////////////////////////////////////////////////////////////

#ifndef BHV_AFTER_GOAL_H
#define BHV_AFTER_GOAL_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_AfterGoal
    : public rcsc::SoccerBehavior {
public:
    Bhv_AfterGoal()
      {}

    bool execute( rcsc::PlayerAgent * agent );
};
#endif

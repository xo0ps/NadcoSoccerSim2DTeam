
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DRIBBLE_TARGET_CALCULATOR2_H
#define BHV_DRIBBLE_TARGET_CALCULATOR2_H

#include <rcsc/player/world_model.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_DribbleTargetCalculator2 {


public:
    Bhv_DribbleTargetCalculator2()
      { }

    bool calculate( const rcsc::WorldModel & wm , rcsc::Vector2D * dribble_target );
};

#endif

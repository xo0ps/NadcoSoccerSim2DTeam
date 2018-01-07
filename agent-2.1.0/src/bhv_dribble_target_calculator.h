
/////////////////////////////////////////////////////////////////////

#ifndef BHV_DRIBBLE_TARGET_CALCULATOR_H
#define BHV_DRIBBLE_TARGET_CALCULATOR_H

#include <rcsc/player/world_model.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_DribbleTargetCalculator {
private:
    const double M_dist;
    const double M_dist2;

public:
    Bhv_DribbleTargetCalculator( const double dist , const double dist2 )
        : M_dist( dist )
        , M_dist2( dist2 )
      { }

    bool calculate( const rcsc::WorldModel & wm , rcsc::Vector2D * dribble_target , const bool cross );
};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef BHV_HASSLE_H
#define BHV_HASSLE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_Hassle
    : public rcsc::SoccerBehavior {

private:
    const rcsc::Vector2D M_home_pos;

public:
    Bhv_Hassle( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
		{ }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif

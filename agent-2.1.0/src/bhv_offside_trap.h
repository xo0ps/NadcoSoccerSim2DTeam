
/////////////////////////////////////////////////////////////////////

#ifndef BHV_OFFSIDE_TRAP_H
#define BHV_OFFSIDE_TRAP_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_OffsideTrap
    : public rcsc::SoccerBehavior {
private:
	
	const rcsc::Vector2D M_home_pos;
	
public:

    Bhv_OffsideTrap( const rcsc::Vector2D & home_pos )
		: M_home_pos( home_pos )
			{ }

    bool execute( rcsc::PlayerAgent * agent );
	
};

#endif

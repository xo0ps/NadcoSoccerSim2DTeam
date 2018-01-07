
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SPREAD_POSITIONING_H
#define BHV_SPREAD_POSITIONING_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_SpreadPositioning
    : public rcsc::SoccerBehavior {

private:
	
	const rcsc::Vector2D M_home_pos;
	
public:

    Bhv_SpreadPositioning( const rcsc::Vector2D & home_pos )
		: M_home_pos( home_pos )
			{ }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif

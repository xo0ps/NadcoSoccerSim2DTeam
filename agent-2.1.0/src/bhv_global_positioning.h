
/////////////////////////////////////////////////////////////////////

#ifndef BHV_GLOBAL_POSITIONING_H
#define BHV_GLOBAL_POSITIONING_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_GlobalPositioning
    : public rcsc::SoccerBehavior {
private:
	
	const rcsc::Vector2D M_home_pos;
	double M_dash_power;
	
public:

    Bhv_GlobalPositioning( const rcsc::Vector2D & home_pos )
		: M_home_pos( home_pos )
			{ }

    bool execute( rcsc::PlayerAgent * agent );

	static double getDashPower( const rcsc::PlayerAgent * agent , const rcsc::Vector2D & target_point );

private:
	rcsc::Vector2D cross( rcsc::PlayerAgent * agent );
	rcsc::Vector2D dribbleAttack( rcsc::PlayerAgent * agent );
	rcsc::Vector2D dribbleBlock( rcsc::PlayerAgent * agent );
	rcsc::Vector2D shootchance( rcsc::PlayerAgent * agent );
	rcsc::Vector2D defmid( rcsc::PlayerAgent * agent );
	rcsc::Vector2D offmid( rcsc::PlayerAgent * agent );
	
};

#endif

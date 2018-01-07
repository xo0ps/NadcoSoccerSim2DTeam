
/////////////////////////////////////////////////////////////////////

#ifndef BODY_DASH_H
#define BODY_DASH_H

#include <rcsc/player/soccer_action.h>


class Body_Dash
    : public rcsc::BodyAction {
private:
	const rcsc::AngleDeg M_dash_angle;

public:

	Body_Dash( const rcsc::AngleDeg dash_angle = 0.0 )
		: M_dash_angle( dash_angle )
		{ }
		
    bool execute( rcsc::PlayerAgent * agent );
    static bool turn( rcsc::PlayerAgent * agent );
    static bool test( rcsc::PlayerAgent * agent );
};

#endif

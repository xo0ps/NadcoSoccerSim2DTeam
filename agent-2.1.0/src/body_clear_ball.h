
/////////////////////////////////////////////////////////////////////

#ifndef BODY_CLEAR_BALL_H
#define BODY_CLEAR_BALL_H

#include <rcsc/player/soccer_action.h>

class Body_ClearBall
    : public rcsc::BodyAction {

public:
	Body_ClearBall()
    	    { }

	bool execute( rcsc::PlayerAgent * agent );
	static bool furthestTM( rcsc::PlayerAgent * agent );
	static bool bestAngle( rcsc::PlayerAgent * agent );
	static bool clear( rcsc::PlayerAgent * agent );
};
#endif

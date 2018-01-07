
/////////////////////////////////////////////////////////////////////

#ifndef BODY_THROUGH_PASS2_H
#define BODY_THROUGH_PASS2_H

#include <vector>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Body_ThroughPass2
    : public rcsc::BodyAction {

public:
	Body_ThroughPass2( )
		{ }
    
    
    bool execute( rcsc::PlayerAgent * agent );
	bool calculate( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, int * receiver );
	static bool test( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, int * receiver );
	
};
#endif

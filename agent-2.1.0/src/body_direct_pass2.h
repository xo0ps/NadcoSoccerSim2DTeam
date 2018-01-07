
/////////////////////////////////////////////////////////////////////

#ifndef BODY_DIRECT_PASS2_H
#define BODY_DIRECT_PASS2_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <vector>

class Body_DirectPass2
    : public rcsc::BodyAction {

	
private:

	struct pass{
		rcsc::Vector2D point;
		double value;
		int unum;
	}p;
	
	
public:
	
	Body_DirectPass2()
    	    { }
	
	bool execute( rcsc::PlayerAgent * agent );
	bool evaluate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , int * unum );
	static bool test( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point );
};

#endif

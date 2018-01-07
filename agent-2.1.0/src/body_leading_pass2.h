
/////////////////////////////////////////////////////////////////////

#ifndef BODY_LEADING_PASS2_H
#define BODY_LEADING_PASS2_H

#include <vector>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Body_LeadingPass2
    : public rcsc::BodyAction {

public:
	Body_LeadingPass2()
    	    { }
    
    bool execute( rcsc::PlayerAgent * agent );
};
#endif

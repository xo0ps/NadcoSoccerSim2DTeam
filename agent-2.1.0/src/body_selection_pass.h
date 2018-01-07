
/////////////////////////////////////////////////////////////////////

#ifndef BODY_SELECTION_PASS_H
#define BODY_SELECTION_PASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <vector>

class Body_SelectionPass
    : public rcsc::BodyAction {


	//friend class Body_DirectPass;
	
private:

	struct pass{
		rcsc::Vector2D point;
		double value;
		int unum;
		char type;
	}pass_route;
	
public:
	Body_SelectionPass( )
    	    { }
    
    bool execute( rcsc::PlayerAgent * agent );
    bool select_pass( rcsc::PlayerAgent * agent , std::vector< pass > * pass );
    
};
#endif

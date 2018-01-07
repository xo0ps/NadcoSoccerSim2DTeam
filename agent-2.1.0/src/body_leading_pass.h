
/////////////////////////////////////////////////////////////////////

#ifndef BODY_LEADING_PASS_H
#define BODY_LEADING_PASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Body_LeadingPass
    : public rcsc::BodyAction {

private:

	const std::string M_mode;

public:
	Body_LeadingPass( const std::string mode )
		: M_mode( mode )
    	    { }
    
    bool execute( rcsc::PlayerAgent * agent );	    
	bool evaluate( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	bool offendResponsePass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	bool toCornersPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	
	bool old_leading( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	
	static
	bool test( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point );
};
#endif

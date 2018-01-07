
/////////////////////////////////////////////////////////////////////

#ifndef BODY_THROUGH_PASS_H
#define BODY_THROUGH_PASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Body_ThroughPass
    : public rcsc::BodyAction {

private:

	const std::string M_mode;

public:
	Body_ThroughPass( const std::string mode )
		: M_mode( mode )
    	    { }
    
    struct ThroughPass
    {
		rcsc::Vector2D point;
		double value;
		int unum;
	};
    
    bool execute( rcsc::PlayerAgent * agent );
	bool toBehindDefendersPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	bool pointToPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	static bool test( rcsc::PlayerAgent * agent );
	static bool test1( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	
	struct compareClass
	{
		bool operator()( double v1 , double v2 )
		{
			return ( v1 < v2 );
		}
	} cmp;
	
};
#endif

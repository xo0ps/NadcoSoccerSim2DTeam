
/////////////////////////////////////////////////////////////////////

#ifndef BODY_DIRECT_PASS_H
#define BODY_DIRECT_PASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <vector>

class Body_DirectPass
    : public rcsc::BodyAction {

	
private:

	struct pass{
		rcsc::Vector2D point;
		double value;
		int unum;
	};
	
	const std::string M_mode;
	std::vector< std::pair< rcsc::Vector2D , int > >M_pass;
	std::vector< pass >P_pass;
	
public:
	
	Body_DirectPass( const std::string mode )
		: M_mode( mode )
    	    { }
	
	Body_DirectPass( std::vector< std::pair< rcsc::Vector2D , int > >pass )
		: M_pass( pass )
    	    { }
	
	Body_DirectPass( std::vector< pass >pass )
		: P_pass( pass )
    	    { }

	bool execute( rcsc::PlayerAgent * agent );
	bool evaluate( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	bool shortPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver );
	bool indirectPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver );
	bool deadBallPass( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point , double * first_speed, int * receiver );
	
	bool old_direct( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	bool old_short( rcsc::PlayerAgent * agent, rcsc::Vector2D * target_point, double * first_speed, int * receiver );
	void fill_vector( rcsc::PlayerAgent * agent );
	static bool test( rcsc::PlayerAgent * agent );
	static bool test( rcsc::PlayerAgent * agent , rcsc::Vector2D * point );
	static bool test_new( rcsc::PlayerAgent * agent , rcsc::Vector2D * point );
	static bool test_new( rcsc::PlayerAgent * agent );
};
#endif

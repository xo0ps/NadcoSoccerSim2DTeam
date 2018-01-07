
/////////////////////////////////////////////////////////////////////

#ifndef BODY_PASS_H
#define BODY_PASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_object.h>

class Body_Pass
    : public rcsc::BodyAction {
		
public:
    Body_Pass( )
		{ }

	static bool isLastValid( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , double * first_speed , int * receiver );
	
	bool simulate( rcsc::PlayerAgent * agent , rcsc::Vector2D * target_point , double * first_speed , int * receiver );
	
	static bool passRequest( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point );
	static bool requestedPass( rcsc::PlayerAgent * agent );
	static bool requestedPass2( rcsc::PlayerAgent * agent );
	bool runRequest( rcsc::PlayerAgent * agent , const int runner , rcsc::Vector2D pass_point );
	static bool requestedRun( rcsc::PlayerAgent * agent );
	static double theta( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point );
	static double end_speed( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point , char type );
	static bool exist_opponent( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point );
	static bool exist_opponent2( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point );
	static bool exist_opponent3( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point , bool through = false );
	static bool can_pass( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point );
	static bool say_pass( rcsc::PlayerAgent * agent , int unum , rcsc::Vector2D pass_point );
	static double first_speed( rcsc::PlayerAgent * agent , rcsc::Vector2D pass_point , char type );

	
	
};
#endif

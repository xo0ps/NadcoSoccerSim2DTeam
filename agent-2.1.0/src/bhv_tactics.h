
/////////////////////////////////////////////////////////////////////

#ifndef BHV_TACTICS_H
#define BHV_TACTICS_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>


class Bhv_Tactics
    : public rcsc::SoccerBehavior {

public:
	
	const std::string M_mode;

    Bhv_Tactics( std::string mode )
		:M_mode( mode )
			{ }

    bool execute( rcsc::PlayerAgent * agent );
    
private:
	bool oneTwoPass( rcsc::PlayerAgent * agent );
	bool coolDown( rcsc::PlayerAgent * agent );
	bool timeKill( rcsc::PlayerAgent * agent );
	bool farPass( rcsc::PlayerAgent * agent );
	bool substitueRequest( rcsc::PlayerAgent * agent );
	bool substitueKick( rcsc::PlayerAgent * agent );

};

#endif

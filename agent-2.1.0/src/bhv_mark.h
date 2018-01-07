
/////////////////////////////////////////////////////////////////////

#ifndef BHV_MARK_H
#define BHV_MARK_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_Mark
    : public rcsc::SoccerBehavior {
private:

    const std::string M_mode;
    const rcsc::Vector2D M_home_pos;

public:

    Bhv_Mark( const std::string mode , const rcsc::Vector2D & home_pos )
		: M_mode( mode ) ,
		  M_home_pos( home_pos )
			{ }

    bool execute( rcsc::PlayerAgent * agent );
    bool markPassLine( rcsc::PlayerAgent * agent );
    bool markEscape( rcsc::PlayerAgent * agent );
    bool mark( rcsc::PlayerAgent * agent );

};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef BHV_SHOOTCHANCE_MOVE_H
#define BHV_SHOOTCHANCE_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_ShootChanceMove
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;
    const bool M_turn_at;
    const bool M_turn_neck;

public:
    Bhv_ShootChanceMove( const rcsc::Vector2D & home_pos,
                   const bool turn_at = true,
                   const bool turn_neck = true )
        : M_home_pos( home_pos )
        , M_turn_at( turn_at )
        , M_turn_neck( turn_neck )
      { }

    bool execute( rcsc::PlayerAgent * agent );
	
	static bool doGoToCrossMove( rcsc::PlayerAgent * agent );

private:

	void goToReceivablePoint( rcsc::PlayerAgent * agent );
	
};

#endif

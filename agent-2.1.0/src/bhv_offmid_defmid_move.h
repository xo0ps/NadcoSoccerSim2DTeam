
/////////////////////////////////////////////////////////////////////

#ifndef BHV_OFFMID_DEFMID_MOVE_H
#define BHV_OFFMID_DEFMID_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_OffmidDefmidMove
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;
    bool M_sideback;

public:
    Bhv_OffmidDefmidMove( const rcsc::Vector2D & home_pos,
                   bool sideback = false )
        : M_home_pos( home_pos )
        , M_sideback( sideback )
      { }

    bool execute( rcsc::PlayerAgent * agent );
    
    static bool doAttackMove( rcsc::PlayerAgent * agent );

private:
	void goToReceivablePoint( rcsc::PlayerAgent * agent );
};

#endif

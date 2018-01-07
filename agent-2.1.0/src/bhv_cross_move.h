
/////////////////////////////////////////////////////////////////////

#ifndef BHV_CROSS_MOVE_H
#define BHV_CROSS_MOVE_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>

class Bhv_CrossMove
    : public rcsc::SoccerBehavior {

private:
    const rcsc::Vector2D M_home_pos;
    bool M_sideback;
    void goToReceivablePoint( rcsc::PlayerAgent * agent );

public:
    Bhv_CrossMove( const rcsc::Vector2D & home_pos, bool sideback = false )
        : M_home_pos( home_pos )
        , M_sideback( sideback )
      { }

    static double getDashPower( const rcsc::PlayerAgent * agent, const rcsc::Vector2D & target_point );

    static double get_dash_power( const rcsc::PlayerAgent * agent );

	static bool doCrossBlockMove( rcsc::PlayerAgent * agent );
	bool execute( rcsc::PlayerAgent * agent );

};

#endif

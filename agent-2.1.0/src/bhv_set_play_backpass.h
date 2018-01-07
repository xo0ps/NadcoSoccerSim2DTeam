
/////////////////////////////////////////////////////////////////////

#ifndef BHV_SET_PLAY_BACKPASS_H
#define BHV_SET_PLAY_BACKPASS_H

#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>


class Bhv_SetPlayBackpass
    : public rcsc::SoccerBehavior {
private:
    const rcsc::Vector2D M_home_pos;

public:
    Bhv_SetPlayBackpass( const rcsc::Vector2D & home_pos )
        : M_home_pos( home_pos )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool isKicker( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
    void doSave( rcsc::PlayerAgent * agent );

};

#endif

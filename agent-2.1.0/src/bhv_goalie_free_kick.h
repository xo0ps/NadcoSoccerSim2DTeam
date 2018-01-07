
/////////////////////////////////////////////////////////////////////

#ifndef BHV_GOALIE_FREE_KICK_H
#define BHV_GOALIE_FREE_KICK_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_GoalieFreeKick
    : public rcsc::SoccerBehavior {
private:

public:
    Bhv_GoalieFreeKick()
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    rcsc::Vector2D getKickPoint( const rcsc::PlayerAgent * agent );
    void doKick( rcsc::PlayerAgent * agent );

    void doWait( rcsc::PlayerAgent * agent );

};

#endif

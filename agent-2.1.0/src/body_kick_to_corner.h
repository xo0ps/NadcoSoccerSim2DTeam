
/////////////////////////////////////////////////////////////////////

#ifndef BODY_KICK_TO_CORNER_H
#define BODY_KICK_TO_CORNER_H

#include <rcsc/player/soccer_action.h>


class Body_KickToCorner
    : public rcsc::BodyAction {
private:
    const bool M_to_left;

public:
    // if left == ( world.self().pos().y < 0.0 )
    //   kick to the near side corner
    explicit
    Body_KickToCorner( const bool left )
        : M_to_left( left )
      { }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif

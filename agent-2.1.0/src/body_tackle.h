
/////////////////////////////////////////////////////////////////////

#ifndef BODY_TACKLE_H
#define BODY_TACKLE_H

#include <rcsc/player/soccer_action.h>

class Body_Tackle
    : public rcsc::SoccerBehavior {

public:
    Body_Tackle()
      { }

    bool execute( rcsc::PlayerAgent * agent );

};

#endif

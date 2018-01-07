
/////////////////////////////////////////////////////////////////////

#ifndef NECk_GOALIE_TURN_NECK_H
#define NECk_GOALIE_TURN_NECK_H

#include <rcsc/player/soccer_action.h>


class Neck_GoalieTurnNeck
    : public rcsc::NeckAction {
private:


public:

    bool execute( rcsc::PlayerAgent * agent );

    NeckAction * clone() const
      {
          return new Neck_GoalieTurnNeck;
      }
};

#endif

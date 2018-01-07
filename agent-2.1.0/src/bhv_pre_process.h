
/////////////////////////////////////////////////////////////////////

#ifndef BHV_PRE_PROCESS_H
#define BHV_PRE_PROCESS_H

#include <rcsc/player/soccer_action.h>

class Strategy;

class Bhv_PreProcess
    : public rcsc::SoccerBehavior {
public:
    explicit
    Bhv_PreProcess( )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:
    void doReceiveMove( rcsc::PlayerAgent * agent );
};

#endif

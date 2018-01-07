
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BLOCK_H
#define BHV_BLOCK_H

#include <rcsc/player/soccer_action.h>

class Bhv_Block
    : public rcsc::SoccerBehavior {

public:
    Bhv_Block( )
		{ }

    bool execute( rcsc::PlayerAgent * agent );
    bool newBlock( rcsc::PlayerAgent * agent );

};

#endif

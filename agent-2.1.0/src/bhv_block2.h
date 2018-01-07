		
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BLOCK2_H
#define BHV_BLOCK_H

#include <rcsc/player/soccer_action.h>

class Bhv_Block2
    : public rcsc::SoccerBehavior {


public:
    Bhv_Block2( )
		{ }

    struct Block
    {
		rcsc::Vector2D target;
		int unum;
		double value;
	}block;


    bool execute( rcsc::PlayerAgent * agent );
};

#endif

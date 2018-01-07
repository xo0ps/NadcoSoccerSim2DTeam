
/////////////////////////////////////////////////////////////////////

#ifndef BHV_OPPONENT_STRATEGY_LEARNING_H
#define BHV_OPPONENT_STRATEGY_LEARNING_H

#include <rcsc/player/soccer_action.h>

class Bhv_OpponentStrategyLearning
    : public rcsc::SoccerBehavior {

public:

	Bhv_OpponentStrategyLearning( )
		{ }
		
    bool execute( rcsc::PlayerAgent * agent );

private:

};

#endif


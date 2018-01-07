
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BEST_ACTION_H
#define BHV_BEST_ACTION_H

#include <rcsc/player/soccer_action.h>

class Bhv_BestAction
    : public rcsc::SoccerBehavior {

public:
    Bhv_BestAction( )
		{ };

    bool execute( rcsc::PlayerAgent * agent );
    
	enum type{
		
		DIRECT,
		THROUGH,
		DASH,
		DRIBBLE,
		TAC_KICK,
		CLEAR,
		CORNER,
		SHOOT,
		ADVANCE,
		HOLD,
		TACKLE,
		BLOCK,
		DEF_BREAK,
		POSITION,
		HASSLE,
		MARK,
		TAC_MOVE,
		TH_CUT,
		INTERCEPT,
		RUN,
	};
	
	
private:
	
	int direct( rcsc::PlayerAgent * agent );
	int through( rcsc::PlayerAgent * agent );
	int dash( rcsc::PlayerAgent * agent );
	int dribble( rcsc::PlayerAgent * agent );
	int tactic_kick( rcsc::PlayerAgent * agent );
	int clear( rcsc::PlayerAgent * agent );
	int corner( rcsc::PlayerAgent * agent );
	int shoot( rcsc::PlayerAgent * agent );
	int advance( rcsc::PlayerAgent * agent );
	int hold( rcsc::PlayerAgent * agent );
	int tackle( rcsc::PlayerAgent * agent );
	int block( rcsc::PlayerAgent * agent );
	int defense_breaker( rcsc::PlayerAgent * agent );
	int positioning( rcsc::PlayerAgent * agent );
	int hassle( rcsc::PlayerAgent * agent );
	int mark( rcsc::PlayerAgent * agent );
	int tactic_move( rcsc::PlayerAgent * agent );
	int through_cut( rcsc::PlayerAgent * agent );
	int intercept( rcsc::PlayerAgent * agent );
	int run( rcsc::PlayerAgent * agent );
	
	int calculate( rcsc::PlayerAgent * agent );
	
};

#endif

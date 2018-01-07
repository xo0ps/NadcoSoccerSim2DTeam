
//////////////////////////////////////////////////////

#ifndef BHV_GOALIE_H
#define BHV_GOALIE_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_Goalie
    : public rcsc::SoccerBehavior {
 public:
      Bhv_Goalie()
	{ }

      bool execute( rcsc::PlayerAgent * agent );
 private:
      bool doKick( rcsc::PlayerAgent * agent );
      bool doMove( rcsc::PlayerAgent * agent );
      bool chaseMove( rcsc::PlayerAgent * agent );
      bool basicMove( rcsc::PlayerAgent * agent );
      rcsc::Vector2D centroid( rcsc::PlayerAgent * agent,
			       rcsc::Vector2D * left_goal_side,
			       rcsc::Vector2D * right_goal_side,
			       int * left_cycle,
			       int * right_cycle );
      bool dangerSituation( rcsc::PlayerAgent * agent );
      void message( rcsc::PlayerAgent * agent );
      double dashPower( rcsc::PlayerAgent * agent,
			const rcsc::Vector2D & target_point );

      static int M_passMate;
      static int M_neck_cnt;
};

#endif

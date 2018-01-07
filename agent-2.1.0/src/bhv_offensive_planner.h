
/////////////////////////////////////////////////////////////////////

#ifndef BHV_OFFENSIVE_PLANNER_H
#define BHV_OFFENSIVE_PLANNER_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <vector>


class Bhv_OffensivePlanner
    : public rcsc::BodyAction {
private:

public:

    Bhv_OffensivePlanner()
      { }

    static bool execute( rcsc::PlayerAgent * agent , rcsc::Vector2D target_point , int receiver , double first_speed );
	static bool simplePass( rcsc::PlayerAgent * agent );
	static bool createShootLine( rcsc::PlayerAgent * agent );
	static bool keepShootChance( rcsc::PlayerAgent * agent );
	static bool isShootable( rcsc::PlayerAgent * agent , rcsc::Vector2D tm_pos );
	static bool isShootLineExist( rcsc::PlayerAgent * agent , rcsc::Vector2D & target_point );
	static bool isShootLineExist2( rcsc::PlayerAgent * agent , rcsc::Vector2D target_point , bool checkShootable = false );
	static bool findPassNode( rcsc::PlayerAgent * agent , int src_unum , int dest_unum , std::vector<int> & pass );
	static bool exist( int value , std::vector<int> & a );
	static bool existPassLine( rcsc::PlayerAgent * agent , int src_unum , int dest_unum );
	static bool findPassPos( rcsc::PlayerAgent * agent , int dest_unum );

};

#endif

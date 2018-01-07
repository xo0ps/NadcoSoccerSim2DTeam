
/////////////////////////////////////////////////////////////////////

#ifndef BODY_SHOOT_H
#define BODY_SHOOT_H

#include "shoot_table.h"
#include <rcsc/player/soccer_action.h>

class Body_Shoot
    : public rcsc::BodyAction {
private:

    static ShootTable S_shoot_table;

public:

    Body_Shoot()
      { }

    bool execute( rcsc::PlayerAgent * agent );

    static
    ShootTable & shoot_table()
      {
          return S_shoot_table;
      }
	static bool createShootLine( rcsc::PlayerAgent * agent );
	/*
	static bool keepShootChance( rcsc::PlayerAgent * agent );
	static bool isShootable( rcsc::PlayerAgent * agent , rcsc::Vector2D tm_pos );
	static bool isShootLineExist( rcsc::PlayerAgent * agent , rcsc::Vector2D & target_point );
	static bool isShootLineExist2( rcsc::PlayerAgent * agent , rcsc::Vector2D & target_point , bool checkShootable = false );
	static bool findPassNode( rcsc::PlayerAgent * agent , int src_unum , int dest_unum , std::vector<int> & pass );
	static bool exist( int value , std::vector<int> & a );
	*/
};

#endif

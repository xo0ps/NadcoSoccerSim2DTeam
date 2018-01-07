
/////////////////////////////////////////////////////////////////////

#ifndef ROLE_GOALIE_H
#define ROLE_GOALIE_H

#include <rcsc/game_time.h>

#include "soccer_role.h"

#include <rcsc/geom/vector_2d.h>
#include <rcsc/geom/polygon_2d.h>
#include <vector>

class RoleGoalie
    : public SoccerRole {
private:
    rcsc::GameTime M_last_goalie_kick_time;

public:

    static const std::string NAME;

    explicit
    RoleGoalie( )
        : SoccerRole( ),
          M_last_goalie_kick_time( 0, 0 )
      { }

    ~RoleGoalie()
      {
          //std::cerr << "delete RoleGoalie" << std::endl;
      }

    virtual
    void execute( rcsc::PlayerAgent * agent );

    static
    std::string name()
      {
          return std::string( "Goalie" );
      }


    static
    SoccerRole::Ptr create()
      {
          SoccerRole::Ptr ptr( new RoleGoalie() );
          return ptr;
      }

private:
    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};

#endif

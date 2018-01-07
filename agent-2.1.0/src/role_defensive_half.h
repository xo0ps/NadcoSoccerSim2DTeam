
/////////////////////////////////////////////////////////////////////

#ifndef ROLE_DEFENSIVE_HALF_H
#define ROLE_DEFENSIVE_HALF_H

#include "soccer_role.h"

class RoleDefensiveHalf
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleDefensiveHalf()
      { }

    ~RoleDefensiveHalf()
      { }

    virtual
    void execute( rcsc::PlayerAgent * agent );


    static
    const
    std::string & name()
      {
          return NAME;
      }

    static
    SoccerRole::Ptr create()
      {
          SoccerRole::Ptr ptr( new RoleDefensiveHalf() );
          return ptr;
      }

private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};


#endif

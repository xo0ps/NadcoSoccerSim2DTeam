
/////////////////////////////////////////////////////////////////////

#ifndef ROLE_OFFENSIVE_HALF_H
#define ROLE_OFFENSIVE_HALF_H

#include "soccer_role.h"

class RoleOffensiveHalf
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleOffensiveHalf()
      { }

    ~RoleOffensiveHalf()
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
          SoccerRole::Ptr ptr( new RoleOffensiveHalf() );
          return ptr;
      }

private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef ROLE_SIDE_FORWARD_H
#define ROLE_SIDE_FORWARD_H

#include "soccer_role.h"

class RoleSideForward
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleSideForward()
      { }

    ~RoleSideForward()
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
          SoccerRole::Ptr ptr( new RoleSideForward() );
          return ptr;
      }
private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};


#endif

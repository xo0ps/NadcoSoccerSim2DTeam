
/////////////////////////////////////////////////////////////////////

#ifndef ROLE_CENTER_FORWARD_H
#define ROLE_CENTER_FORWARD_H

#include "soccer_role.h"

class RoleCenterForward
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleCenterForward()
      { }

    ~RoleCenterForward()
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
          SoccerRole::Ptr ptr( new RoleCenterForward() );
          return ptr;
      }
private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};


#endif


/////////////////////////////////////////////////////////////////////

#ifndef ROLE_CENTER_BACK_H
#define ROLE_CENTER_BACK_H

#include "soccer_role.h"

class RoleCenterBack
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleCenterBack()
      { }

    ~RoleCenterBack()
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
          SoccerRole::Ptr ptr( new RoleCenterBack() );
          return ptr;
      }

private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef ROLE_SIDE_BACK_H
#define ROLE_SIDE_BACK_H

#include "soccer_role.h"

class RoleSideBack
    : public SoccerRole {
private:

public:

    static const std::string NAME;

    RoleSideBack()
      { }

    ~RoleSideBack()
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
          SoccerRole::Ptr ptr( new RoleSideBack() );
          return ptr;
      }
private:

    void doKick( rcsc::PlayerAgent * agent );
    void doMove( rcsc::PlayerAgent * agent );
};


#endif

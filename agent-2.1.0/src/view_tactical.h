
/////////////////////////////////////////////////////////////////////

#ifndef VIEW_TACTICAL_H
#define VIEW_TACTICAL_H

#include <rcsc/player/soccer_action.h>

class View_Tactical
    : public rcsc::ViewAction {
private:

public:

    bool execute( rcsc::PlayerAgent * agent );


    ViewAction * clone() const
      {
          return new View_Tactical;
      }

private:

    bool doDefault( rcsc::PlayerAgent * agent );
    bool doOurGoalieFreeKick( rcsc::PlayerAgent * agent );

};

#endif

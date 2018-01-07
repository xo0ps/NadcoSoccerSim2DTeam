
/////////////////////////////////////////////////////////////////////

#ifndef BHV_BASIC_TACKLE_H
#define BHV_BASIC_TACKLE_H

#include <rcsc/player/soccer_action.h>

class Bhv_BasicTackle
    : public rcsc::SoccerBehavior {
private:
    const double M_min_probability;
    const double M_body_thr;
public:
    Bhv_BasicTackle( const double & min_prob,
                     const double & body_thr )
        : M_min_probability( min_prob )
        , M_body_thr( body_thr )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool executeV14( rcsc::PlayerAgent * agent );
};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef INTENAION_RECEIVE_H
#define INTENAION_RECEIVE_H

#include <rcsc/game_time.h>
#include <rcsc/geom/vector_2d.h>

#include <rcsc/player/soccer_intention.h>

class IntentionReceive
    : public rcsc::SoccerIntention {

private:
    const rcsc::Vector2D M_target_point; // global coordinate
    const double M_dash_power;
    const double M_buffer;
    int M_step;

    rcsc::GameTime M_last_execute_time;

public:

    IntentionReceive( const rcsc::Vector2D & target_point,
                      const double & dash_power,
                      const double & buf,
                      const int max_step,
                      const rcsc::GameTime & start_time);

    bool finished( const rcsc::PlayerAgent * agent );

    bool execute( rcsc::PlayerAgent * agent );

};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef BODY_LONG_DRIBBLE_H
#define BODY_LONG_DRIBBLE_H

#include <rcsc/player/player_agent.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>

#include <vector>

namespace rcsc {
class PlayerObject;
class WorldModel;
}

class Body_LongDribble {
private:

    rcsc::GameTime M_update_time;
    int M_total_count;

    // private for singleton
    Body_LongDribble();

public:

    bool generate( const rcsc::WorldModel & wm );
    bool execute( const rcsc::PlayerAgent * agent );

private:

    void createCourses( const rcsc::WorldModel & wm );

    void createSelfCache( const rcsc::WorldModel & wm,
                          const rcsc::AngleDeg & dash_angle,
                          const int n_turn,
                          const int n_dash,
                          std::vector< rcsc::Vector2D > & self_cache );

    bool canKick( const rcsc::WorldModel & wm,
                  const int n_turn,
                  const int n_dash,
                  const rcsc::Vector2D & receive_pos );

    bool checkOpponent( const rcsc::WorldModel & wm,
                        const int n_turn,
                        const int n_dash,
                        const rcsc::Vector2D & ball_pos,
                        const rcsc::Vector2D & receive_pos );

};

#endif

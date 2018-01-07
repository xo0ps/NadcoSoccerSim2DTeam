
/////////////////////////////////////////////////////////////////////

#ifndef BODY_KICK_TO_CENTER_H
#define BODY_KICK_TO_CENTER_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>

#include <vector>
#include <functional>

namespace rcsc {
class PlayerObject;
}

class Body_KickToCenter
    : public rcsc::SoccerBehavior {
public:

    struct Target {
        const rcsc::PlayerObject * receiver;
        rcsc::Vector2D target_point;
        double first_speed;
        double score;

        Target( const rcsc::PlayerObject * recv,
                const rcsc::Vector2D & pos,
                const double & speed,
                const double & scr )
            : receiver( recv ),
              target_point( pos ),
              first_speed( speed ),
              score( scr )
          { }
    };

    class TargetCmp
        : public std::binary_function< Target, Target, bool > {
    public:
        result_type operator()( const first_argument_type & lhs,
                                const second_argument_type & rhs ) const
          {
              return lhs.score < rhs.score;
          }
    };

private:

    static std::vector< Target > S_cached_target;

public:

    bool execute( rcsc::PlayerAgent * agent );


    static
    bool get_best_point( const rcsc::PlayerAgent * agent,
                         rcsc::Vector2D * target_point );
private:

    static
    void search( const rcsc::PlayerAgent * agent );

    static
    bool create_close( const rcsc::PlayerAgent * agent,
                       const rcsc::PlayerObject * receiver );
    static
    void create_target( const rcsc::PlayerAgent * agent,
                        const rcsc::PlayerObject * receiver );

};

#endif

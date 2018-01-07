
/////////////////////////////////////////////////////////////////////

#ifndef BHV_PENALTY_KICK_H
#define BHV_PENALTY_KICK_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

class Bhv_PenaltyKick
    : public rcsc::SoccerBehavior {
private:

public:

    bool execute( rcsc::PlayerAgent * agent );

private:
    bool isKickTaker( rcsc::PlayerAgent * agent );

    bool doKickerWait( rcsc::PlayerAgent * agent );
    bool doKickerSetup( rcsc::PlayerAgent * agent );
    bool doKickerReady( rcsc::PlayerAgent * agent );
    bool doKicker( rcsc::PlayerAgent * agent );

    // used only for one kick PK
    bool doOneKickShoot( rcsc::PlayerAgent * agent );
    // used only for multi kick PK
    bool doShoot( rcsc::PlayerAgent * agent );
    bool getShootTarget( rcsc::PlayerAgent * agent,
                         rcsc::Vector2D * point,
                         double * first_speed );
    bool doDribble( rcsc::PlayerAgent * agent );


    bool doGoalieWait( rcsc::PlayerAgent * agent );
    bool doGoalieSetup( rcsc::PlayerAgent * agent );
    bool doGoalie( rcsc::PlayerAgent * agent );

    bool doGoalieBasicMove( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getGoalieMovePos( rcsc::PlayerAgent * agent,
                                     const rcsc::Vector2D & ball_pos,
                                     const rcsc::Vector2D & my_pos );
    bool doGoalieChaseBall( rcsc::PlayerAgent * agent );
    bool isGoalieBallChaseSituation();
    bool doGoalieSlideChase( rcsc::PlayerAgent * agent );
    bool doFirstKick( rcsc::PlayerAgent * agent );
    bool goToStaticBall( rcsc::PlayerAgent * agent, rcsc::AngleDeg ball_place_angle );
    double get_set_play_dash_power( const rcsc::PlayerAgent * agent );
};

#endif

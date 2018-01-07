
/////////////////////////////////////////////////////////////////////

#ifndef SAMPLE_COMMUNICATION_H
#define SAMPLE_COMMUNICATION_H

#include "communication.h"

#include <rcsc/game_time.h>
#include <rcsc/types.h>

namespace rcsc {
class WorldModel;
}

class SampleCommunication
    : public Communication {
private:

    int M_current_sender_unum;
    int M_next_sender_unum;
    rcsc::GameTime M_ball_send_time;
    rcsc::GameTime M_teammate_send_time[12];
    rcsc::GameTime M_opponent_send_time[12];

public:

    SampleCommunication();
    ~SampleCommunication();

    virtual
    bool execute( rcsc::PlayerAgent * agent );

    int currentSenderUnum() const { return M_current_sender_unum; }

    int nextSenderUnum() const { return M_next_sender_unum; }

private:
    void updateCurrentSender( const rcsc::PlayerAgent * agent );

    void updatePlayerSendTime( const rcsc::WorldModel & wm,
                               const rcsc::SideID side,
                               const int unum );

    bool shouldSayBall( const rcsc::PlayerAgent * agent );
    bool shouldSayOpponentGoalie( const rcsc::PlayerAgent * agent );
    bool goalieSaySituation( const rcsc::PlayerAgent * agent );

    bool sayBallAndPlayers( rcsc::PlayerAgent * agent );
    bool sayBall( rcsc::PlayerAgent * agent );
    bool sayPlayers( rcsc::PlayerAgent * agent );
    bool sayGoalie( rcsc::PlayerAgent * agent );

    bool sayIntercept( rcsc::PlayerAgent * agent );
    bool sayOffsideLine( rcsc::PlayerAgent * agent );
    bool sayDefenseLine( rcsc::PlayerAgent * agent );

    bool sayOpponents( rcsc::PlayerAgent * agent );
    bool sayTwoOpponents( rcsc::PlayerAgent * agent );
    bool sayThreeOpponents( rcsc::PlayerAgent * agent );

    bool saySelf( rcsc::PlayerAgent * agent );

    bool sayStamina( rcsc::PlayerAgent * agent );
    bool sayRecovery( rcsc::PlayerAgent * agent );

    void attentiontoSomeone( rcsc::PlayerAgent * agent );
};

#endif

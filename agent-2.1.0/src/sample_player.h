
/////////////////////////////////////////////////////////////////////

#ifndef SAMPLE_PLAYER_H
#define SAMPLE_PLAYER_H

#include <rcsc/player/player_agent.h>
#include "communication.h"

class SamplePlayer
    : public rcsc::PlayerAgent {
public:

    SamplePlayer();

    virtual
    ~SamplePlayer();

protected:

    virtual
    bool initImpl( rcsc::CmdLineParser & cmd_parser );

    virtual
    void actionImpl();

    virtual
    void communicationImpl();

    virtual
    void handleServerParam();
    virtual
    void handlePlayerParam();
    virtual
    void handlePlayerType();


private:

    Communication::Ptr M_communication;
	
    bool doPreprocess();
    bool doShoot();
    bool doForceKick();
    bool doHeardPassReceive();
    bool sayBall();
    bool saySelf();
    bool sayGoalie();
    bool sayIntercept();
    bool sayOffsideLine();
    bool sayDefenseLine();

    void attentiontoSomeone();
    void reset();
};

#endif

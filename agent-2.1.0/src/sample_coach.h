
/////////////////////////////////////////////////////////////////////

#ifndef SAMPLE_COACH_H
#define SAMPLE_COACH_H

#include <vector>
#include <map>
#include <set>

#include <rcsc/types.h>

#include <rcsc/coach/coach_agent.h>

namespace rcsc {
class PlayerType;
}

class SampleCoach
    : public rcsc::CoachAgent {
private:
    typedef std::vector< const rcsc::PlayerType * > PlayerTypePtrCont;

    int M_substitute_count; // substitution count after kick-off
    std::vector< int > M_available_player_type_id;
    std::map< int, int > M_assigned_player_type_id;

    int M_opponent_player_types[11];
    double M_teammate_recovery[11];
    rcsc::TeamGraphic M_team_graphic;

public:

    SampleCoach();

    virtual
    ~SampleCoach();

private:

    /*!
      \brief substitute player.

      This methos should be overrided in the derived class
    */
    void doSubstitute();
    void updateRecoveryInfo();
    
    void doFirstSubstitute();
    void doSubstituteTiredPlayers();
    void TeamStatistics();
    void DangerousOpponents();
    
    void substituteTo( const int unum,
                       const int type );

    int getFastestType( PlayerTypePtrCont & candidates );

    void sayPlayerTypes();

    void sendTeamGraphic();

protected:

    /*!
      You can override this method.
      But you must call PlayerAgent::doInit() in this method.
    */
    virtual
    bool initImpl( rcsc::CmdLineParser & cmd_parser );

    //! main decision
    virtual
    void actionImpl();

};

#endif


/////////////////////////////////////////////////////////////////////

#ifndef STRATEGY_H
#define STRATEGY_H

#include "soccer_role.h"

#include <rcsc/formation/formation.h>
#include <rcsc/geom/vector_2d.h>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>
#include <string>

// # define USE_GENERIC_FACTORY 1

namespace rcsc {
class CmdLineParser;
class WorldModel;
}

enum PositionType {
    Position_Left = -1,
    Position_Center = 0,
    Position_Right = 1,
};

enum StrategyType {
    Normal,
    Offensive,
    Defensive,
};

class Strategy {
public:
    static const std::string BEFORE_KICK_OFF_CONF;
    static const std::string NORMAL_FORMATION_CONF;
    static const std::string GOALIE_FORMATION_CONF;
    static const std::string GOAL_KICK_OPP_FORMATION_CONF;
    static const std::string GOAL_KICK_OUR_FORMATION_CONF;
    static const std::string GOALIE_CATCH_OPP_FORMATION_CONF;
    static const std::string GOALIE_CATCH_OUR_FORMATION_CONF;
    static const std::string KICKIN_OUR_FORMATION_CONF;
    static const std::string SETPLAY_OPP_FORMATION_CONF;
    static const std::string SETPLAY_OUR_FORMATION_CONF;
    static const std::string INDIRECT_FREEKICK_OPP_FORMATION_CONF;
    static const std::string INDIRECT_FREEKICK_OUR_FORMATION_CONF;
    static const std::string DEFENSE_FORMATION_CONF;
    static const std::string OFFENSE_FORMATION_CONF;

    enum BallArea {
        BA_CrossBlock, BA_DribbleBlock, BA_DribbleAttack, BA_Cross,
        BA_Stopper,    BA_DefMidField,  BA_OffMidField,   BA_ShootChance,
        BA_Danger,

        BA_None
    };


    StrategyType M_opponent_offense_strategy;
    StrategyType M_opponent_defense_strategy;
      
      
    bool goalie() const
      {
          return M_advanced_goalie;
      }
    bool dash() const
      {
          return M_dash;
      }
    bool block() const
      {
          return M_block;
      }
    bool mark() const
      {
          return M_mark;
      }
    bool mark_escape() const
      {
          return M_mark_escape;
      }
    bool tackle() const
      {
          return M_tackle;
      }
    bool def_break() const
      {
          return M_defense_breaker;
      }
    bool goal_patterns() const
      {
          return M_goal_patterns;
      }
    bool off_planner() const
      {
          return M_offensive_planner;
      }
    bool static_learning() const
      {
          return M_static_learning;
      }
    bool tactics() const
      {
          return M_tactics;
      }
    bool selection_pass() const
      {
          return M_selection_pass;
      }
    bool fast_pass() const
      {
          return M_fast_pass;
      }
    bool old_pass() const
      {
          return M_old_pass;
      }
    bool danger_fast_pass() const
      {
          return M_danger_fast_pass;
      }
    bool hassle() const
      {
          return M_hassle;
      }
    bool offside_trap() const
      {
          return M_offside_trap;
      }
    bool field_cover() const
      {
          return M_field_cover;
      }
    bool strategy_learning() const
      {
          return M_strategy_learning;
      }
    bool formation_changer() const
      {
          return M_formation_changer;
      }
    bool decision_pass() const
      {
          return M_decision_pass;
      }
    bool long_dribble() const
      {
          return M_long_dribble;
      }
    bool th_cut() const
      {
          return M_th_cut;
      }
    bool rc() const
      {
          return M_rc;
      }


private:
    //
    // factories
    //
#ifndef USE_GENERIC_FACTORY
    typedef std::map< std::string, SoccerRole::Creator > RoleFactory;
    typedef std::map< std::string, rcsc::Formation::Creator > FormationFactory;

    RoleFactory M_role_factory;
    FormationFactory M_formation_factory;
#endif


    //
    // formations
    //

    rcsc::Formation::Ptr M_before_kick_off_formation;
    rcsc::Formation::Ptr M_normal_formation;
    rcsc::Formation::Ptr M_goalie_formation;
    rcsc::Formation::Ptr M_goal_kick_opp_formation;
    rcsc::Formation::Ptr M_goal_kick_our_formation;
    rcsc::Formation::Ptr M_goalie_catch_opp_formation;
    rcsc::Formation::Ptr M_goalie_catch_our_formation;
    rcsc::Formation::Ptr M_kickin_our_formation;
    rcsc::Formation::Ptr M_setplay_opp_formation;
    rcsc::Formation::Ptr M_setplay_our_formation;
    rcsc::Formation::Ptr M_indirect_freekick_opp_formation;
    rcsc::Formation::Ptr M_indirect_freekick_our_formation;
    rcsc::Formation::Ptr M_defense_formation;
    rcsc::Formation::Ptr M_offense_formation;

    //
    // current home positions
    //

    std::vector< PositionType > M_position_types;
    std::vector< rcsc::Vector2D > M_positions;
    
    bool M_advanced_goalie;
    bool M_dash;
    bool M_block;
    bool M_mark;
    bool M_mark_escape;
    bool M_tackle;
    bool M_defense_breaker;
    bool M_goal_patterns;
    bool M_offensive_planner;
	bool M_static_learning;
	bool M_tactics;
	bool M_selection_pass;
	bool M_fast_pass;
	bool M_old_pass;
	bool M_danger_fast_pass;
	bool M_hassle;
	bool M_offside_trap;
	bool M_field_cover;
	bool M_strategy_learning;
	bool M_formation_changer;
	bool M_decision_pass;
	bool M_long_dribble;
	bool M_th_cut;
	bool M_rc;
	
    // private for singleton
    Strategy();

    // not used
    Strategy( const Strategy & );
    const Strategy & operator=( const Strategy & );
public:

    static
    Strategy & instance();

    static
    const
    Strategy & i()
      {
          return instance();
      }

    //
    // initialization
    //

    bool init( rcsc::CmdLineParser & cmd_parser );
    bool read( const std::string & config_dir );


    //
    // update
    //

    void update( const rcsc::WorldModel & wm );

    //
    // accessor to the current information
    //
    SoccerRole::Ptr createRole( const int number,
                                const rcsc::WorldModel & wm ) const;
    PositionType getPositionType( const int number ) const;
    rcsc::Vector2D getPosition( const int number ) const;



private:
    void updateSituation( const rcsc::WorldModel & wm );
    // update the current position table
    void updatePosition( const rcsc::WorldModel & wm );
    void analyzeOpponentStrategy( const rcsc::WorldModel & wm );

    rcsc::Formation::Ptr readFormation( const std::string & filepath );
    rcsc::Formation::Ptr createFormation( const std::string & type_name ) const;

    rcsc::Formation::Ptr getFormation( const rcsc::WorldModel & wm ) const;

public:
    static
    BallArea get_ball_area( const rcsc::WorldModel & wm );
    static
    BallArea get_ball_area( const rcsc::Vector2D & ball_pos );
};

#endif

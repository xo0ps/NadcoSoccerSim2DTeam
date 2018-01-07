
/////////////////////////////////////////////////////////////////////

#ifndef OPTIONS_H
#define OPTIONS_H

#include <rcsc/param/cmd_line_parser.h>

class Options {

public:

    Options();
    
    static bool initial();
    static bool set( rcsc::CmdLineParser & cmd_parser );
    
    static Options & instance()
	{
		static Options s_instance;
		return s_instance;
	}

    static Options & i()
      {
          return instance();
      }
    
    
    /*
		-1 = Defensive
		 0 = Normal
		+1 = Offensive
    */
    
    int offense_strategy()
      {
          return M_opponent_offense_strategy;
      }
      
    int defense_strategy()
      {
          return M_opponent_defense_strategy;
      }
    
    bool goalie()
      {
          return M_advanced_goalie;
      }
    bool dash()
      {
          return M_dash;
      }
    bool block()
      {
          return M_block;
      }
      
    bool mark()
      {
          return M_mark;
      }
      
    bool mark_escape()
      {
          return M_mark_escape;
      }
      
    bool tackle()
      {
          return M_tackle;
      }
      
    bool def_break()
      {
          return M_defense_breaker;
      }
      
    bool goal_patterns()
      {
          return M_goal_patterns;
      }
      
    bool off_planner()
      {
          return M_offensive_planner;
      }
      
    bool static_learning()
      {
          return M_static_learning;
      }
      
    bool tactics()
      {
          return M_tactics;
      }
      
    bool selection_pass()
      {
          return M_selection_pass;
      }
      
    bool fast_pass()
      {
          return M_fast_pass;
      }
      
    bool old_pass()
      {
          return M_old_pass;
      }
      
    bool danger_fast_pass()
      {
          return M_danger_fast_pass;
      }
      
    bool hassle()
      {
          return M_hassle;
      }
      
    bool offside_trap()
      {
          return M_offside_trap;
      }
      
    bool field_cover()
      {
          return M_field_cover;
      }
      
    bool strategy_learning()
      {
          return M_strategy_learning;
      }
      
    bool formation_changer()
      {
          return M_formation_changer;
      }
      
    bool decision_pass()
      {
          return M_decision_pass;
      }
      
    bool long_dribble()
      {
          return M_long_dribble;
      }
      
    bool th_cut()
      {
          return M_th_cut;
      }
      
    bool rc()
      {
          return M_rc;
      }

	void set_goalie( bool en )
	{
		M_advanced_goalie = en;
	}

	void set_dash( bool en )
	{
		M_dash = en;
	}
	
	void set_block( bool en )
	{
		M_block = en;
	}
	
	void set_mark( bool en )
	{
		M_mark = en;
	}

	void set_mark_escape( bool en )
	{
		M_mark_escape = en;
	}
	
	void set_tackle( bool en )
	{
		M_tackle = en;
	}
	
	void set_def_break( bool en )
	{
		M_defense_breaker = en;
	}
	
	void set_goal_patterns( bool en )
	{
		M_goal_patterns = en;
	}
	
	void set_off_planner( bool en )
	{
		M_offensive_planner = en;
	}
	
	void set_static_learning( bool en )
	{
		M_static_learning = en;
	}
	
	void set_tactics( bool en )
	{
		M_tactics = en;
	}
	
	void set_selection_pass( bool en )
	{
		M_selection_pass = en;
	}
	
	void set_fast_pass( bool en )
	{
		M_fast_pass = en;
	}
	
	void set_old_pass( bool en )
	{
		M_old_pass = en;
	}
	
	void set_danger_fast_pass( bool en )
	{
		M_danger_fast_pass = en;
	}
	
	void set_hassle( bool en )
	{
		M_hassle = en;
	}
	
	void set_offside_trap( bool en )
	{
		M_offside_trap = en;
	}
	
	void set_field_cover( bool en )
	{
		M_field_cover = en;
	}
	
	void set_strategy_learning( bool en )
	{
		M_strategy_learning = en;
	}
	
	void set_formation_changer( bool en )
	{
		M_formation_changer = en;
	}
	
	void set_decision_pass( bool en )
	{
		M_decision_pass = en;
	}
	
	void set_long_dribble( bool en )
	{
		M_long_dribble = en;
	}
	
	void set_th_cut( bool en )
	{
		M_th_cut = en;
	}
	
	void set_rc( bool en )
	{
		M_rc = en;
	}
		
    void set_offense_strategy( int en )
	{
		M_opponent_offense_strategy = en;
	}
	
	void set_defense_strategy( int en )
	{
		M_opponent_defense_strategy = en;
	}
	  
//private:

    static bool M_advanced_goalie;
    static bool M_dash;
    static bool M_block;
    static bool M_mark;
    static bool M_mark_escape;
    static bool M_tackle;
    static bool M_defense_breaker;
    static bool M_goal_patterns;
    static bool M_offensive_planner;
	static bool M_static_learning;
	static bool M_tactics;
	static bool M_selection_pass;
	static bool M_fast_pass;
	static bool M_old_pass;
	static bool M_danger_fast_pass;
	static bool M_hassle;
	static bool M_offside_trap;
	static bool M_field_cover;
	static bool M_strategy_learning;
	static bool M_formation_changer;
	static bool M_decision_pass;
	static bool M_long_dribble;
	static bool M_th_cut;
	static bool M_rc;
	
	static int M_opponent_offense_strategy;
    static int M_opponent_defense_strategy;  
    
};

#endif

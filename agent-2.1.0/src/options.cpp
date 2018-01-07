// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/param/conf_file_parser.h>

	int M_opponent_offense_strategy = 0;
	int M_opponent_defense_strategy = 0;
	bool M_advanced_goalie = false;
    bool M_dash = true;
    bool M_block = true;
    bool M_mark = true;
    bool M_mark_escape = true;
    bool M_tackle = true;
    bool M_defense_breaker = false;
	bool M_goal_patterns = false;
    bool M_offensive_planner = false;
    bool M_static_learning = false;
    bool M_tactics = true;
    bool M_selection_pass = true;
    bool M_fast_pass = false;
    bool M_old_pass = false;
    bool M_danger_fast_pass = true;
    bool M_hassle = false;
    bool M_offside_trap = false;
    bool M_field_cover = true;
    bool M_strategy_learning = false;
    bool M_formation_changer = false;
    bool M_decision_pass = false;
    bool M_long_dribble = false;
    bool M_th_cut = false;
    bool M_rc = true;

/*-------------------------------------------------------------------*/

bool
Options::initial()
{
	/*
	

	*/
	
	return true;
}


/*-------------------------------------------------------------------*/

bool
Options::set( rcsc::CmdLineParser & cmd_parser )
{
	
	rcsc::ParamMap param_map( "NADCO-2D Options" );

	/*
    param_map.add()
        ( "Opponent_Offensive_Strategy", "a", &M_opponent_offense_strategy, "Opponent_Offensive_Strategy" )
        ( "Opponent_Defensive_Strategy", "b", &M_opponent_defense_strategy, "Opponent_Defensive_Strategy" )
        ( "Advanced_Goalie", "c", rcsc::BoolSwitch( &M_advanced_goalie ), "Advanced_Goalie" )
        ( "Dash_Mode", "d", rcsc::BoolSwitch( &M_dash ), "Dash_Mode" )
        ( "Block_Mode", "e", rcsc::BoolSwitch( &M_block ), "Block_Mode" )
        ( "Non-PlayOn_Mark_Mode", "f", rcsc::BoolSwitch( &M_mark ), "Non-PlayOn_Mark_Mode" )
        ( "Non-PlayOn_Mark_Escape", "g", rcsc::BoolSwitch( &M_mark_escape ), "Non-PlayOn_Mark_Escape" )
        ( "Advanced_Tackle_Mode", "h", rcsc::BoolSwitch( &M_tackle ), "Advanced_Tackle_Mode" )
        ( "Defense_Breaker_Mode", "i", rcsc::BoolSwitch( &M_defense_breaker ), "Defense_Breaker_Mode" )
        ( "Goal_Patterns_Mode", "j", rcsc::BoolSwitch( &M_goal_patterns ), "Goal_Patterns_Mode" )
        ( "Offensive_Planner_Mode", "k", rcsc::BoolSwitch( &M_offensive_planner ), "Offensive_Planner_Mode" )
        ( "Statistical_Learning_Mode", "l", rcsc::BoolSwitch( &M_static_learning ), "Statistical_Learning_Mode" )
        ( "Long_Term_Tactics_Mode", "m", rcsc::BoolSwitch( &M_tactics ), "Long_Term_Tactics_Mode" )
        ( "Selection_Pass_Mode", "n", rcsc::BoolSwitch( &M_selection_pass ), "Selection_Pass_Mode" )
        ( "Fast_Pass_Mode", "o", rcsc::BoolSwitch( &M_fast_pass ), "Fast_Pass_Mode" )
        ( "Old_Pass_Mode", "p", rcsc::BoolSwitch( &M_old_pass ), "Old_Pass_Mode" )
        ( "Danger_Fast_Pass_Mode", "q", rcsc::BoolSwitch( &M_danger_fast_pass ), "Danger_Fast_Pass_Mode" )
        ( "PlayOn_Hassle_Mode", "r", rcsc::BoolSwitch( &M_hassle ), "PlayOn_Hassle_Mode" )
        ( "Offside_Trap_Mode", "s", rcsc::BoolSwitch( &M_offside_trap ), "Offside_Trap_Mode" )
        ( "Field_View_Cover_Mode", "t", rcsc::BoolSwitch( &M_field_cover ), "Field_View_Cover_Mode" )
        ( "Strategy_Learning_Mode", "u", rcsc::BoolSwitch( &M_strategy_learning ), "Strategy_Learning_Mode" )
        ( "Formation_Changer_Mode", "v", rcsc::BoolSwitch( &M_formation_changer ), "Formation_Changer_Mode" )
        ( "Decision_Pass_Mode", "w", rcsc::BoolSwitch( &M_decision_pass ), "Decision_Pass_Mode" )
        ( "Long_Dribble_Mode", "x", rcsc::BoolSwitch( &M_long_dribble ), "Long_Dribble_Mode" )
        ( "Through_Pass_Cut", "y", rcsc::BoolSwitch( &M_th_cut ), "Through_Pass_Cut" )
        ( "RC_Mode", "z", rcsc::BoolSwitch( &M_rc ), "RC_Mode" );
	*/

    rcsc::ConfFileParser confparser( "Options.conf" );

    if ( ! confparser.parse( param_map ) )
    {
        std::cerr << "Invalid config file" << std::endl;
        return false;
    }

    if ( ! cmd_parser.parse( param_map ) )
    {
        std::cerr << "Invalid command line argument" << std::endl;
        return false;
    }

	return true;
}

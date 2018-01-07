// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

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

#include "strategy.h"

#include "soccer_role.h"

#include "role_goalie.h"
#include "role_side_back.h"
#include "role_center_back.h"
#include "role_defensive_half.h"
#include "role_offensive_half.h"
#include "role_side_forward.h"
#include "role_center_forward.h"

#include <rcsc/formation/formation_static.h>
#include <rcsc/formation/formation_dt.h>

#include <rcsc/player/intercept_table.h>
#include <rcsc/player/world_model.h>

#include <rcsc/common/server_param.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/game_mode.h>

#include <set>
#include <fstream>
#include <iostream>
#include <cstdio>


const std::string Strategy::BEFORE_KICK_OFF_CONF = "before-kick-off.conf";
const std::string Strategy::NORMAL_FORMATION_CONF = "normal-formation.conf";
const std::string Strategy::GOALIE_FORMATION_CONF = "goalie-formation.conf";
const std::string Strategy::GOAL_KICK_OPP_FORMATION_CONF = "goal-kick-opp.conf";
const std::string Strategy::GOAL_KICK_OUR_FORMATION_CONF = "goal-kick-our.conf";
const std::string Strategy::GOALIE_CATCH_OPP_FORMATION_CONF = "goalie-catch-opp.conf";
const std::string Strategy::GOALIE_CATCH_OUR_FORMATION_CONF = "goalie-catch-our.conf";
const std::string Strategy::KICKIN_OUR_FORMATION_CONF = "kickin-our-formation.conf";
const std::string Strategy::SETPLAY_OPP_FORMATION_CONF = "setplay-opp-formation.conf";
const std::string Strategy::SETPLAY_OUR_FORMATION_CONF = "setplay-our-formation.conf";
const std::string Strategy::INDIRECT_FREEKICK_OPP_FORMATION_CONF = "indirect-freekick-opp-formation.conf";
const std::string Strategy::INDIRECT_FREEKICK_OUR_FORMATION_CONF = "indirect-freekick-our-formation.conf";
const std::string Strategy::DEFENSE_FORMATION_CONF = "defense-formation.conf";
const std::string Strategy::OFFENSE_FORMATION_CONF = "offense-formation.conf";

/*-------------------------------------------------------------------*/
/*!

*/
Strategy::Strategy()
    : M_opponent_offense_strategy( Normal )
    , M_opponent_defense_strategy( Normal )
    , M_position_types( 11, Position_Center )
    , M_positions( 11 )
    , M_advanced_goalie( false )
    , M_dash( true )
    , M_block( true )
    , M_mark( true )
    , M_mark_escape( true )
    , M_tackle( true )
    , M_defense_breaker( false )
    , M_goal_patterns( false )
    , M_offensive_planner( false )
    , M_static_learning( false )
    , M_tactics( true )
    , M_selection_pass( true )
    , M_fast_pass( false )
    , M_old_pass( false )
    , M_danger_fast_pass( true )
    , M_hassle( false )
    , M_offside_trap( false )
    , M_field_cover( true )
    , M_strategy_learning( false )
    , M_formation_changer( false )
    , M_decision_pass( false )
    , M_long_dribble( false )
    , M_th_cut( false )
    , M_rc( true )
{
    //
    // roles
    //

    M_role_factory[RoleGoalie::name()] = &RoleGoalie::create;
    M_role_factory["Sweeper"] = &RoleCenterBack::create;
    M_role_factory[RoleCenterBack::name()] = &RoleCenterBack::create;
    M_role_factory[RoleSideBack::name()] = &RoleSideBack::create;
    M_role_factory[RoleDefensiveHalf::name()] = &RoleDefensiveHalf::create;
    M_role_factory[RoleOffensiveHalf::name()] = &RoleOffensiveHalf::create;
    M_role_factory[RoleSideForward::name()] = &RoleSideForward::create;
    M_role_factory[RoleCenterForward::name()] = &RoleCenterForward::create;

    //
    // formations
    //

    M_formation_factory[rcsc::FormationStatic::name()] = &rcsc::FormationStatic::create;
    //M_formation_factory[rcsc::FormationBPN::name()] = &rcsc::FormationBPN::create;
    M_formation_factory[rcsc::FormationDT::name()] = &rcsc::FormationDT::create;
    //M_formation_factory[rcsc::FormationNGNet::name()] = &rcsc::FormationNGNet::create;
    //M_formation_factory[rcsc::FormationUvA::name()] = &rcsc::FormationUvA::create;
}

/*-------------------------------------------------------------------*/
/*!

*/
Strategy &
Strategy::instance()
{
    static Strategy s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Strategy::init( rcsc::CmdLineParser & cmd_parser )
{
	/*
    rcsc::ParamMap param_map( "HELIOS_base options" );

    // std::string fconf;
    //param_map.add()
    //    ( "fconf", "", &fconf, "another formation file." );

    //
    //
    //

    if ( cmd_parser.count( "help" ) > 0 )
    {
        param_map.printHelp( std::cout );
        return false;
    }

    //
    //
    //

    cmd_parser.parse( param_map );
	*/
	
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Strategy::read( const std::string & formation_dir )
{
    static bool s_initialized = false;

    if ( s_initialized )
    {
        std::cerr << __FILE__ << ' ' << __LINE__ << ": already initialized."
                  << std::endl;
        return false;
    }

    std::string configpath = formation_dir;
    if ( ! configpath.empty()
         && configpath[ configpath.length() - 1 ] != '/' )
    {
        configpath += '/';
    }

    // before kick off
    M_before_kick_off_formation = readFormation( configpath + BEFORE_KICK_OFF_CONF );
    if ( ! M_before_kick_off_formation )
    {
        std::cerr << "Failed to read before_kick_off formation" << std::endl;
        return false;
    }
    else
    {
		std::cout<<"\n Loading Formation File : "<<configpath + BEFORE_KICK_OFF_CONF<<"          "<<"[ "<<"\033[0;32mOK\033[0m"<<" ]"<<std::endl;
	}

    ///////////////////////////////////////////////////////////
    M_normal_formation = readFormation( configpath + NORMAL_FORMATION_CONF );
    if ( ! M_normal_formation )
    {
        std::cerr << "Failed to read normal formation" << std::endl;
        return false;
    }
    else
    {
		std::cout<<" Loading Formation File : "<<configpath + NORMAL_FORMATION_CONF<<"         "<<"[ "<<"\033[0;32mOK\033[0m"<<" ]"<<std::endl;
	}

    M_goalie_formation = readFormation( configpath + GOALIE_FORMATION_CONF );
    if ( ! M_goalie_formation )
    {
        std::cerr << "Failed to read goalie formation" << std::endl;
        return false;
    }
    else
    {
		std::cout<<" Loading Formation File : "<<configpath + GOALIE_FORMATION_CONF<<"         "<<"[ "<<"\033[0;32mOK\033[0m"<<" ]"<<std::endl;
	}

    M_goal_kick_opp_formation = readFormation( configpath + GOAL_KICK_OPP_FORMATION_CONF );
    if ( ! M_goal_kick_opp_formation )
    {
        std::cerr << "Failed to read goal kick opp formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + GOAL_KICK_OPP_FORMATION_CONF<<std::endl;
	}

    M_goal_kick_our_formation = readFormation( configpath + GOAL_KICK_OUR_FORMATION_CONF );
    if ( ! M_goal_kick_our_formation )
    {
		std::cerr << "Failed to read goal kick our formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + GOAL_KICK_OUR_FORMATION_CONF<<std::endl;
	}

    M_goalie_catch_opp_formation = readFormation( configpath + GOALIE_CATCH_OPP_FORMATION_CONF );
    if ( ! M_goalie_catch_opp_formation )
    {
		std::cerr << "Failed to read goalie catch opp formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + GOALIE_CATCH_OPP_FORMATION_CONF<<std::endl;
	}

    M_goalie_catch_our_formation = readFormation( configpath + GOALIE_CATCH_OUR_FORMATION_CONF );
    if ( ! M_goalie_catch_our_formation )
    {
		std::cerr << "Failed to read goalie catch our formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + GOALIE_CATCH_OUR_FORMATION_CONF<<std::endl;
	}

    M_kickin_our_formation = readFormation( configpath + KICKIN_OUR_FORMATION_CONF );
    if ( ! M_kickin_our_formation )
    {
        std::cerr << "Failed to read kickin our formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + KICKIN_OUR_FORMATION_CONF<<std::endl;
	}

    M_setplay_opp_formation = readFormation( configpath + SETPLAY_OPP_FORMATION_CONF );
    if ( ! M_setplay_opp_formation )
    {
        std::cerr << "Failed to read setplay opp formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + SETPLAY_OPP_FORMATION_CONF<<std::endl;
	}

    M_setplay_our_formation = readFormation( configpath + SETPLAY_OUR_FORMATION_CONF );
    if ( ! M_setplay_our_formation )
    {
        std::cerr << "Failed to read setplay our formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + SETPLAY_OUR_FORMATION_CONF<<std::endl;
	}

    M_indirect_freekick_opp_formation = readFormation( configpath + INDIRECT_FREEKICK_OPP_FORMATION_CONF );
    if ( ! M_indirect_freekick_opp_formation )
    {
        std::cerr << "Failed to read indirect freekick opp formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + INDIRECT_FREEKICK_OPP_FORMATION_CONF<<std::endl;
	}

    M_indirect_freekick_our_formation = readFormation( configpath + INDIRECT_FREEKICK_OUR_FORMATION_CONF );
    if ( ! M_indirect_freekick_our_formation )
    {
        std::cerr << "Failed to read indirect freekick our formation" << std::endl;
        return false;
    }
    else
    {
		//std::cout<<" Loading Formation File : "<<configpath + INDIRECT_FREEKICK_OUR_FORMATION_CONF<<std::endl;
	}
    
    M_defense_formation = readFormation( configpath + DEFENSE_FORMATION_CONF );
    if ( ! M_defense_formation )
    {
        std::cerr << "Failed to read defense formation" << std::endl;
        return false;
    }
    else
    {
		std::cout<<" Loading Formation File : "<<configpath + DEFENSE_FORMATION_CONF<<"        "<<"[ "<<"\033[0;32mOK\033[0m"<<" ]"<<std::endl;
	}
    
    M_offense_formation = readFormation( configpath + OFFENSE_FORMATION_CONF );
    if ( ! M_offense_formation )
    {
        std::cerr << "Failed to read offense formation" << std::endl;
        return false;
    }
    else
    {
		std::cout<<" Loading Formation File : "<<configpath + OFFENSE_FORMATION_CONF<<"        "<<"[ "<<"\033[0;32mOK\033[0m"<<" ]"<<std::endl;
	}

    s_initialized = true;
    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Formation::Ptr
Strategy::readFormation( const std::string & filepath )
{
    rcsc::Formation::Ptr f;

    std::ifstream fin( filepath.c_str() );
    if ( ! fin.is_open() )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << " ***ERROR*** failed to open file [" << filepath << "]"
                  << std::endl;
        return f;
    }

    std::string temp, type;
    fin >> temp >> type; // read training method type name
    fin.seekg( 0 );

    f = createFormation( type );

    if ( ! f )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << " ***ERROR*** failed to create formation [" << filepath << "]"
                  << std::endl;
        return f;
    }

    //
    // read data from file
    //
    if ( ! f->read( fin ) )
    {
        std::cerr << __FILE__ << ':' << __LINE__ << ':'
                  << " ***ERROR*** failed to read formation [" << filepath << "]"
                  << std::endl;
        f.reset();
        return f;
    }


    //
    // check role names
    //
    for ( int unum = 1; unum <= 11; ++unum )
    {
#ifdef USE_GENERIC_FACTORY
        SoccerRole::Ptr role = SoccerRole::create( f->getRoleName( unum ) );
        if ( ! role )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " ***ERROR*** Unsupported role name ["
                      << f->getRoleName( unum ) << "] is appered in ["
                      << filepath << "]" << std::endl;
            f.reset();
            return f;
        }
#else
        if ( M_role_factory.find( f->getRoleName( unum ) ) == M_role_factory.end() )
        {
            std::cerr << __FILE__ << ':' << __LINE__ << ':'
                      << " ***ERROR*** Unsupported role name ["
                      << f->getRoleName( unum ) << "] is appered in ["
                      << filepath << "]" << std::endl;
            f.reset();
            return f;
        }
#endif
    }

    return f;
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Formation::Ptr
Strategy::createFormation( const std::string & type_name ) const
{
    rcsc::Formation::Ptr f;

#ifdef USE_GENERIC_FACTORY
    f = rcsc::Formation::create( type_name );
#else
    FormationFactory::const_iterator creator = M_formation_factory.find( type_name );
    if ( creator == M_formation_factory.end() )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** unsupported formation type ["
                  << type_name << "]"
                  << std::endl;
        return f;
    }
    f = creator->second();
#endif

    if ( ! f )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** unsupported formation type ["
                  << type_name << "]"
                  << std::endl;
    }

    return f;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Strategy::update( const rcsc::WorldModel & wm )
{
    static rcsc::GameTime s_update_time( -1, 0 );

    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    updatePosition( wm );
}

/*-------------------------------------------------------------------*/
/*!

*/
SoccerRole::Ptr
Strategy::createRole( const int number,
                      const rcsc::WorldModel & world ) const
{
    SoccerRole::Ptr role;

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** Invalid player number " << number
                  << std::endl;
        return role;
    }

    rcsc::Formation::Ptr f = getFormation( world );
    if ( ! f )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** faled to create role. Null formation" << std::endl;
        return role;
    }

    const std::string role_name = f->getRoleName( number );

#ifdef USE_GENERIC_FACTORY
    role = SoccerRole::create( role_name );
#else
    RoleFactory::const_iterator factory = M_role_factory.find( role_name );
    if ( factory != M_role_factory.end() )
    {
        role = factory->second();
    }
#endif

    if ( ! role )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " ***ERROR*** unsupported role name ["
                  << role_name << "]"
                  << std::endl;
    }
    return role;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
Strategy::updatePosition( const rcsc::WorldModel & wm )
{
    static rcsc::GameTime s_update_time( 0, 0 );
    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    rcsc::Formation::Ptr f = getFormation( wm );
    if ( ! f )
    {
        std::cerr << wm.time()
                  << " ***ERROR*** could not get the current formation" << std::endl;
        return;
    }

    int ball_step = 0;
    if ( wm.gameMode().type() == rcsc::GameMode::PlayOn
         || wm.gameMode().type() == rcsc::GameMode::GoalKick_ )
    {
        ball_step = std::min( 1000, wm.interceptTable()->teammateReachCycle() );
        ball_step = std::min( ball_step, wm.interceptTable()->opponentReachCycle() );
        ball_step = std::min( ball_step, wm.interceptTable()->selfReachCycle() );
    }

    rcsc::Vector2D ball_pos = wm.ball().inertiaPoint( ball_step );
    
    M_positions.clear();
    f->getPositions( ball_pos, M_positions );

    if ( rcsc::ServerParam::i().useOffside() )
    {
        double max_x = wm.offsideLineX();
        int mate_step = wm.interceptTable()->teammateReachCycle();
        if ( mate_step < 50 )
        {
            rcsc::Vector2D trap_pos = wm.ball().inertiaPoint( mate_step );
            if ( trap_pos.x > max_x ) max_x = trap_pos.x;
        }

        for ( int unum = 1; unum <= 11; ++unum )
        {
            if ( M_positions[unum-1].x > max_x - 1.0 )
            {
                M_positions[unum-1].x = max_x - 1.0;
            }
        }
    }

    M_position_types.clear();
    for ( int unum = 1; unum <= 11; ++unum )
    {
        PositionType type = Position_Center;
        if ( f->isSideType( unum ) )
        {
            type = Position_Left;
        }
        else if ( f->isSymmetryType( unum ) )
        {
            type = Position_Right;
        }

        M_position_types.push_back( type );
    }
}


/*-------------------------------------------------------------------*/
/*!

*/
PositionType
Strategy::getPositionType( const int number ) const
{
    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return Position_Center;
    }

    try
    {
        return M_position_types.at( number - 1 );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return Position_Center;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Vector2D
Strategy::getPosition( const int number ) const
{
    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return rcsc::Vector2D::INVALIDATED;
    }

    try
    {
        return M_positions.at( number - 1 );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return rcsc::Vector2D::INVALIDATED;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
rcsc::Formation::Ptr
Strategy::getFormation( const rcsc::WorldModel & wm ) const
{
    if ( wm.gameMode().type() == rcsc::GameMode::PlayOn )
    {
        if ( wm.self().goalie()
             && M_goalie_formation )
        {
            return M_goalie_formation;
        }

        return M_normal_formation;
    }

    if ( wm.gameMode().type() == rcsc::GameMode::KickIn_
         || wm.gameMode().type() == rcsc::GameMode::CornerKick_ )
    {
        if ( wm.ourSide() == wm.gameMode().side() )
        {
            // our kick-in or corner-kick
            return M_kickin_our_formation;
        }
        else
        {
            return M_setplay_opp_formation;
        }
    }

    if ( ( wm.gameMode().type() == rcsc::GameMode::BackPass_
           && wm.gameMode().side() == wm.theirSide() )
         || ( wm.gameMode().type() == rcsc::GameMode::IndFreeKick_
              && wm.gameMode().side() == wm.ourSide() ) )
    {
        return M_indirect_freekick_our_formation;
    }

    if ( ( wm.gameMode().type() == rcsc::GameMode::BackPass_
           && wm.gameMode().side() == wm.ourSide() )
         || ( wm.gameMode().type() == rcsc::GameMode::IndFreeKick_
              && wm.gameMode().side() == wm.theirSide() ) )
    {
        return M_indirect_freekick_opp_formation;
    }

    if ( wm.gameMode().type() == rcsc::GameMode::GoalKick_ )
    {
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            return M_goal_kick_our_formation;
        }
        else
        {
            return M_goal_kick_opp_formation;
        }
    }

    if ( wm.gameMode().type() == rcsc::GameMode::GoalieCatch_ )
    {
        if ( wm.gameMode().side() == wm.ourSide() )
        {
            return M_goalie_catch_our_formation;
        }
        else
        {
            return M_goalie_catch_opp_formation;
        }
    }

    if ( wm.gameMode().type() == rcsc::GameMode::BeforeKickOff
         || wm.gameMode().type() == rcsc::GameMode::AfterGoal_ )
    {
        return M_before_kick_off_formation;
    }

    if ( wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        return M_setplay_our_formation;
    }

    if ( wm.gameMode().type() != rcsc::GameMode::PlayOn )
    {
        return M_setplay_opp_formation;
    }

    if ( wm.self().goalie()
         && M_goalie_formation )
    {
        return M_goalie_formation;
    }

//our formations

	if ( wm.gameMode().type() == rcsc::GameMode::PlayOn 
		 && wm.ball().pos().x > -10.0 
		 && wm.existKickableTeammate() )
    {
		if ( M_opponent_offense_strategy == Offensive )
			return M_offense_formation;
		else
			return M_normal_formation;
    }

	if ( wm.gameMode().type() == rcsc::GameMode::PlayOn 
		 && wm.ball().pos().x < 10.0 
		 //&& wm.existKickableOpponent() )
		 )
    {
		if ( M_opponent_defense_strategy == Defensive )
			return M_defense_formation;
		else
			return M_normal_formation;
    }

//end 

    return M_normal_formation;
}

/*-------------------------------------------------------------------*/
/*!

*/
Strategy::BallArea
Strategy::get_ball_area( const rcsc::WorldModel & wm )
{
    int ball_step = 1000;
    ball_step = std::min( ball_step, wm.interceptTable()->teammateReachCycle() );
    ball_step = std::min( ball_step, wm.interceptTable()->opponentReachCycle() );
    ball_step = std::min( ball_step, wm.interceptTable()->selfReachCycle() );

    return get_ball_area( wm.ball().inertiaPoint( ball_step ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
Strategy::BallArea
Strategy::get_ball_area( const rcsc::Vector2D & ball_pos )
{
    if ( ball_pos.x > 35.0 )
    {
        if ( ball_pos.absY() > 20.0 )
        {
            return BA_Cross;
        }
        else
        {
            return BA_ShootChance;
        }
    }
    else if ( ball_pos.x > 0.0 )
    {
        if ( ball_pos.absY() > 20.0 )
        {
            return BA_DribbleAttack;
        }
        else
        {
            return BA_OffMidField;
        }
    }
    else if ( ball_pos.x > -30.0 )
    {
        if ( ball_pos.absY() > 20.0 )
        {
            return BA_DribbleBlock;
        }
        else
        {
            return BA_DefMidField;
        }
    }
    else if ( ball_pos.x > -36.5 )
    {
        if ( ball_pos.absY() > 20.0 )
        {
            return BA_CrossBlock;
        }
        else
        {
            return BA_Stopper;
        }
    }
    else
    {
        if ( ball_pos.absY() > 20.0 )
        {
            return BA_CrossBlock;
        }
        else
        {
            return BA_Danger;
        }
    }

    return BA_None;
}

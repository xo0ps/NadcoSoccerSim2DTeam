// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA
 Modified By Mahdi SADEGHI

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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
#include "sample_player.h"
#include "sample_communication.h"
#include "body_shoot.h"
#include "body_kick_one_step.h"
#include "body_intercept.h"
#include "body_go_to_point.h"
#include "bhv_before_kick_off.h"
#include "bhv_after_goal.h"
#include "strategy.h"
#include "soccer_role.h"
#include "view_tactical.h"
#include "intention_receive.h"
#include "bhv_goalie_free_kick.h"
#include "bhv_penalty_kick.h"
#include "bhv_pre_process.h"
#include "bhv_set_play.h"
#include "bhv_set_play_kick_in.h"
#include "bhv_set_play_indirect_free_kick.h"
#include "bhv_set_play_corner_kick.h"
#include "bhv_set_play_backpass.h"

#include <rcsc/formation/formation.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/freeform_parser.h>
#include <rcsc/action/kick_table.h>
#include <rcsc/action/bhv_emergency.h>
#include <rcsc/action/body_turn_to_ball.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/common/basic_client.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/conf_file_parser.h>

#include <boost/shared_ptr.hpp>

#include <sstream>

/*-------------------------------------------------------------------*/
/*!
  constructor
*/
SamplePlayer::SamplePlayer()
    : rcsc::PlayerAgent()
{
    typedef boost::shared_ptr< rcsc::SayMessageParser > SMP;

    boost::shared_ptr< rcsc::AudioMemory > audio_memory( new rcsc::AudioMemory );

    M_worldmodel.setAudioMemory( audio_memory );

    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::BallMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::PassMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::InterceptMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::GoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::GoalieAndPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::OffsideLineMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::DefenseLineMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::WaitRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::PassRequestMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::DribbleMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::BallGoalieMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::OnePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::TwoPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::ThreePlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::SelfMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::TeammateMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::OpponentMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::BallPlayerMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::StaminaMessageParser( audio_memory ) ) );
    addSayMessageParser( rcsc::SayMessageParser::Ptr( new rcsc::RecoveryMessageParser( audio_memory ) ) );

    typedef boost::shared_ptr< rcsc::FreeformParser > FP;

    setFreeformParser( FP( new rcsc::FreeformParser( M_worldmodel ) ) );
	M_communication = Communication::Ptr( new SampleCommunication() );

}

/*-------------------------------------------------------------------*/
/*!
  destructor
*/
SamplePlayer::~SamplePlayer()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::initImpl( rcsc::CmdLineParser & cmd_parser )
{
	
	bool result = rcsc::PlayerAgent::initImpl( cmd_parser );
	//result &= Strategy::instance().init( cmd_parser );

    // read additional options

	/*
    rcsc::ParamMap my_params( "Additional options" );

    cmd_parser.parse( my_params );

    if ( cmd_parser.failed() )
    {
        std::cerr << "***WARNING*** detected invalid options: ";
        cmd_parser.print( std::cerr );
        std::cerr << std::endl;
    }
    
    
    
    Options::initial();
    if( ! Options::set( cmd_parser ) )
    {
		std::cout<<"===> config file parser error! <==="<<std::endl;
		M_client->setServerAlive( false );
        return false;
	}
    */
    
    if ( ! result )
    {
		std::cout<<"result failed"<<std::endl;
        M_client->setServerAlive( false );
        return false;
    }


    if ( config().goalie() )
    {	
		std::cerr << "NADCO_2D Team based on AGENT2D-2.1.0 and Librcsc-4.0.0" << std::endl;
		std::cerr << "Copyright 2006-2011 Hidehisa Akiyama." << std::endl;
		std::cerr << "Modified by NADCO_2D Members." << std::endl;
		std::cerr << "All Rights Reserved.\n" << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Opponent Offense Strategy  "
                  << ( Strategy::i().M_opponent_offense_strategy == Offensive ? "Offensive" : "Normal" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Opponent Defense Strategy  "
                  << ( Strategy::i().M_opponent_defense_strategy == Defensive ? "Defensive" : "Normal" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Goalie Advanced Mode       " << ( Strategy::i().goalie() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Dashing Mode               " << ( Strategy::i().dash() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Blocking Mode              " << ( Strategy::i().block() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Marking Mode               " << ( Strategy::i().mark() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Mark Escaping Mode         " << ( Strategy::i().mark_escape() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Tackling Mode              " << ( Strategy::i().tackle() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Defense Breaking Mode      " << ( Strategy::i().def_break() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Goal Patterns Mode         " << ( Strategy::i().goal_patterns() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Offensive Planner Mode     " << ( Strategy::i().off_planner() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Statistical Learning Mode  " << ( Strategy::i().static_learning() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Tactics Mode               " << ( Strategy::i().tactics() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Selection Pass Mode        " << ( Strategy::i().selection_pass() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Fast Pass Mode             " << ( Strategy::i().fast_pass() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Old Pass Mode              " << ( Strategy::i().old_pass() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Danger Fast Pass Mode      " << ( Strategy::i().danger_fast_pass() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Opponent Hassle Mode       " << ( Strategy::i().hassle() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Offside Trap Mode          " << ( Strategy::i().offside_trap() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Field View Cover Mode      " << ( Strategy::i().field_cover() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Strategy Learning Mode     " << ( Strategy::i().strategy_learning() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Formation Changer Mode     " << ( Strategy::i().formation_changer() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Decision Pass Mode         " << ( Strategy::i().decision_pass() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "Long Dribble Mode          " << ( Strategy::i().long_dribble() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "ThroughPass Cut Mode       " << ( Strategy::i().th_cut() ? "on" : "off" )
                  << std::endl;
        std::cerr << config().teamName() << ": "
                  << "RC Mode                    " << ( Strategy::i().rc() ? "on" : "off" )
                  << std::endl;
    }

    if ( ! Strategy::instance().read( config().configDir() ) )
    {
        std::cerr << "***ERROR*** Failed to read team strategy." << std::endl;
        return false;
    }
    
    return true;
}



/*-------------------------------------------------------------------*/
/*!
  main decision
  This is virtual in super.
*/
void
SamplePlayer::actionImpl()
{
	/*
    // set defense line
    Strategy::instance().update( world() );

    //////////////////////////////////////////////////////////////
    // check tackle expires
    // check self position accuracy
    // ball search
    // check queued intention
    // check simultaneous kick
    if ( Bhv_PreProcess().execute( this ) )
    {
        return;
    }
	
    //////////////////////////////////////////////////////////////
    // create current role
    boost::shared_ptr< SoccerRole > role_ptr
        = Strategy::i().createRole( world().self().unum(),  world() );

    if ( ! role_ptr )
    {
        std::cerr << config().teamName() << ": "
                  << world().self().unum()
                  << " Error. Role is not registered.\nExit ..."
                  << std::endl;
        M_client->setServerAlive( false );
        return;
    }

    //////////////////////////////////////////////////////////////
    // play_on mode
    if ( world().gameMode().type() == rcsc::GameMode::PlayOn  )
    {
        role_ptr->execute( this );
        return;
    }

    rcsc::Vector2D home_pos = Strategy::i().getPosition( world().self().unum() );
    if ( ! home_pos.isValid() )
    {
        std::cerr << config().teamName() << ": "
                  << world().self().unum()
                  << " ***ERROR*** illegal home position."
                  << std::endl;
        home_pos.assign( 0.0, 0.0 );
    }
    
    //////////////////////////////////////////////////////////////
    // kick_in or corner_kick
    if ( ( world().gameMode().type() == rcsc::GameMode::KickIn_ )
         && world().ourSide() == world().gameMode().side() )
    {
        if ( world().self().goalie() )
        {
            Bhv_GoalieFreeKick().execute( this );
        }
        else
        {
            Bhv_SetPlayKickIn( home_pos ).execute( this );
        }
        return;
    }
    if ( ( world().gameMode().type() == rcsc::GameMode::CornerKick_ )
         && world().ourSide() == world().gameMode().side() )
    {
        if ( world().self().goalie() )
        {
            Bhv_GoalieFreeKick().execute( this );
        }
        else
        {
            Bhv_SetPlayCornerKick( home_pos ).execute( this );
        }
        return;
	}

    //////////////////////////////////////////////////////////////
    if ( world().gameMode().type() == rcsc::GameMode::IndFreeKick_ )
    {
        Bhv_SetPlayIndirectFreeKick( home_pos ).execute( this );
        return;
    }
    if ( world().gameMode().type() == rcsc::GameMode::BackPass_ )
    {
        Bhv_SetPlayBackpass( home_pos ).execute( this );
        return;
    }
    
    //////////////////////////////////////////////////////////////
    // penalty kick mode
    if ( world().gameMode().isPenaltyKickMode() )
    {
    
        Bhv_PenaltyKick().execute( this );
        return;
    }

    //////////////////////////////////////////////////////////////
    // goalie free kick mode
    if ( world().self().goalie() )
    {
        Bhv_GoalieFreeKick().execute( this );
        return;
    }

    //////////////////////////////////////////////////////////////
    // other set play mode

    Bhv_SetPlay( home_pos ).execute( this );
    */
    
    
    Strategy::instance().update( world() );
    if ( doPreprocess() )
    {
        return;
    }

    SoccerRole::Ptr role_ptr;
    {
        role_ptr = Strategy::i().createRole( world().self().unum(), world() );

        if ( ! role_ptr )
        {
            std::cerr << config().teamName() << ": "
                      << world().self().unum()
                      << " Error. Role is not registerd.\nExit ..."
                      << std::endl;
            M_client->setServerAlive( false );
            return;
        }
    }

    if ( role_ptr->acceptExecution( world() ) )
    {
        role_ptr->execute( this );
        return;
    }

    if ( world().gameMode().type() == rcsc::GameMode::PlayOn )
    {
        role_ptr->execute( this );
        return;
    }

    if ( world().gameMode().isPenaltyKickMode() )
    {
        Bhv_PenaltyKick().execute( this );
        return;
    }

   rcsc::Vector2D home_pos = Strategy::i().getPosition( world().self().unum() );
    if ( ! home_pos.isValid() )
    {
        std::cerr << config().teamName() << ": "
                  << world().self().unum()
                  << " ***ERROR*** illegal home position."
                  << std::endl;
        home_pos = world().self().pos();
    }
    
    if ( world().self().goalie() )
    {
        Bhv_GoalieFreeKick().execute( this );
        return;
    }
 
    Bhv_SetPlay( home_pos ).execute( this );
    
}

/*-------------------------------------------------------------------*/
/*!

*/

void
SamplePlayer::handleServerParam()
{
    if ( rcsc::KickTable::instance().createTables() )
    {
		//std::cout<<"kick table server param"<<std::endl;
    }
    else
    {
        M_client->setServerAlive( false );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SamplePlayer::handlePlayerParam()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
SamplePlayer::handlePlayerType()
{

}

/*----------------------------------------------------*/

bool
SamplePlayer::doPreprocess()
{
    // check tackle expires
    // check self position accuracy
    // ball search
    // check queued intention
    // check simultaneous kick

    const rcsc::WorldModel & wm = this->world();
    //
    // freezed by tackle effect
    //
    if ( wm.self().isFrozen() )
    {
        // face neck to ball
        this->setViewAction( new View_Tactical() );
        this->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    //
    // BeforeKickOff or AfterGoal. jump to the initial position
    //


    if ( world().gameMode().type() == rcsc::GameMode::BeforeKickOff 
		|| ( world().gameMode().type() == rcsc::GameMode::AfterGoal_ 
			&& world().gameMode().side() != world().ourSide() ) )
    {
        rcsc::Vector2D move_point =  Strategy::i().getPosition( world().self().unum() );
        Bhv_BeforeKickOff( move_point ).execute( this );
        this->setViewAction( new View_Tactical() );
        return true;
    }

	if( world().gameMode().type() == rcsc::GameMode::AfterGoal_ 
		&& world().gameMode().side() == world().ourSide() )
	{
		Bhv_AfterGoal().execute( this );
		this->setViewAction( new View_Tactical() );
		return true;
	}

    //
    // self localization error
    //
    if ( ! wm.self().posValid() )
    {
        rcsc::Bhv_Emergency().execute( this );
        return true;
    }

    //
    // ball localization error
    //
    const int count_thr = ( wm.self().goalie()
                            ? 10
                            : 5 );
    if ( wm.ball().posCount() > count_thr
         || ( wm.gameMode().type() != rcsc::GameMode::PlayOn
              && wm.ball().seenPosCount() > count_thr + 10 ) )
    {
        this->setViewAction( new View_Tactical() );
        this->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        return true;
    }

    //
    // set default change view
    //

    this->setViewAction( new View_Tactical() );

    if ( doShoot() )
    {
        return true;
    }

    if ( this->doIntention() )
    {
        return true;
    }

    if ( doHeardPassReceive() )
    {
        return true;
    }

    if ( doForceKick() )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doShoot()
{
    const rcsc::WorldModel & wm = this->world();

    if ( wm.gameMode().type() != rcsc::GameMode::IndFreeKick_
         && wm.time().stopped() == 0
         && wm.self().isKickable()
         && Body_Shoot().execute( this ) )
    {
        // reset intention
        this->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doForceKick()
{
    const rcsc::WorldModel & wm = this->world();

    if ( wm.gameMode().type() == rcsc::GameMode::PlayOn
         && ! wm.self().goalie()
         && wm.self().isKickable()
         && wm.existKickableOpponent() )
    {
        rcsc::Vector2D goal_pos( rcsc::ServerParam::i().pitchHalfLength(), 0.0 );

        if ( wm.self().pos().x > 36.0 && wm.self().pos().absY() > 10.0 )
        {
            goal_pos.x = 45.0;
        }
        Body_KickOneStep( goal_pos, rcsc::ServerParam::i().ballSpeedMax() ).execute( this );
        this->setIntention( static_cast< rcsc::SoccerIntention * >( 0 ) );
        this->setNeckAction( new rcsc::Neck_ScanField() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::doHeardPassReceive()
{	
    const rcsc::WorldModel & wm = this->world();

    if ( wm.audioMemory().passTime() != wm.time()
         || wm.audioMemory().pass().empty()
         || wm.audioMemory().pass().front().receiver_ != wm.self().unum() )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    rcsc::Vector2D self_trap_pos = wm.ball().inertiaPoint( self_min );
    rcsc::Vector2D receive_pos = ( wm.audioMemory().pass().empty() ? self_trap_pos : wm.audioMemory().pass().front().receive_pos_ );

	//std::cout<<wm.self().unum()<<" => ["<<wm.time().cycle()<<"] ("<<receive_pos.x<<","<<receive_pos.y<<std::endl;
    
    if ( ( ! wm.existKickableTeammate()
           && wm.ball().posCount() <= 1
           && wm.ball().velCount() <= 1
           && self_min < 6
           && self_trap_pos.dist( receive_pos ) < 8.0 )
         || wm.audioMemory().pass().empty() )
    {
        Body_Intercept().execute( this );
        this->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
    }
    else
    {
		bool back_mode = false;
		rcsc::Vector2D target_rel = receive_pos - wm.self().pos();
		rcsc::AngleDeg target_angle = target_rel.th();
		if ( target_rel.r() < 6.0
			 && ( target_angle - wm.self().body() ).abs() > 100.0
			 && wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.4 )
		{
			back_mode = true;
		}
        //if( !Body_GoToPoint( heard_pos, 0.9 , rcsc::ServerParam::i().maxDashPower() ).execute( this ) )
        if( receive_pos.x < 51.0 && receive_pos.y < 33.0 )
			Body_GoToPoint( receive_pos, 1.0, rcsc::ServerParam::i().maxDashPower(), 100, back_mode ).execute( this );
        //Body_GoToPoint( heard_pos, 0.5 , rcsc::ServerParam::i().maxDashPower() ).execute( this );
		//	rcsc::Body_TurnToBall().execute( this );
        this->setNeckAction( new rcsc::Neck_TurnToBall() );
        
    }

    this->setIntention( new IntentionReceive( receive_pos, rcsc::ServerParam::i().maxDashPower(), 0.9, 5, wm.time() ) );
    return true;
}


/*-------------------------------------------------------------------*/
/*!
  communication decision.
  virtual.method
*/
void
SamplePlayer::communicationImpl()
{
	if ( M_communication )
    {
        M_communication->execute( this );
        return;
    }
    
    if ( ! config().useCommunication()
         || world().gameMode().type() == rcsc::GameMode::BeforeKickOff
         || world().gameMode().type() == rcsc::GameMode::AfterGoal_
         || world().gameMode().isPenaltyKickMode() )
    {
        return;
    }

    sayBall();
    sayDefenseLine();
    sayGoalie();
    sayIntercept();
    sayOffsideLine();
    saySelf();

    attentiontoSomeone();
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::sayBall()
{
    // ball Info: seen at current
    const int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::BallMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( world().ball().posCount() > 0
         || world().ball().velCount() > 0 )
    {
        return false;
    }

    const rcsc::PlayerObject * ball_nearest_teammate = NULL;
    const rcsc::PlayerObject * second_ball_nearest_teammate = NULL;

    const rcsc::PlayerPtrCont::const_iterator t_end = world().teammatesFromBall().end();
    for ( rcsc::PlayerPtrCont::const_iterator t = world().teammatesFromBall().begin();
          t != t_end;
          ++t )
    {
        if ( (*t)->isGhost() || (*t)->posCount() >= 10 ) continue;

        if ( ! ball_nearest_teammate )
        {
            ball_nearest_teammate = *t;
        }
        else if ( ! second_ball_nearest_teammate )
        {
            second_ball_nearest_teammate = *t;
            break;
        }
    }

    int mate_min = world().interceptTable()->teammateReachCycle();
    int opp_min = world().interceptTable()->opponentReachCycle();

    bool send_ball = false;
    if ( world().self().isKickable()  // I am kickable
         || ( ! ball_nearest_teammate
              || ( ball_nearest_teammate->distFromBall()
                   > world().ball().distFromSelf() - 3.0 ) // I am nearest to ball
              || ( ball_nearest_teammate->distFromBall() < 6.0
                   && ball_nearest_teammate->distFromBall() > 1.0
                   && opp_min <= mate_min + 1 ) )
         || ( second_ball_nearest_teammate
              && ( ball_nearest_teammate->distFromBall()  // nearest to ball teammate is
                   > rcsc::ServerParam::i().visibleDistance() - 0.5 ) // over vis dist
              && ( second_ball_nearest_teammate->distFromBall()
                   > world().ball().distFromSelf() - 5.0 ) ) ) // I am second
    {
        send_ball = true;
    }

    if ( send_ball
         && static_cast< int >( len + rcsc::BallGoalieMessage::slength() )
         <= rcsc::ServerParam::i().playerSayMsgSize()
         && world().ball().pos().x > 34.0
         && world().ball().pos().absY() < 20.0 )
    {
        const rcsc::PlayerObject * opp_goalie = world().getOpponentGoalie();
        if ( opp_goalie
             && opp_goalie->posCount() == 0
             && opp_goalie->bodyCount() == 0
             && opp_goalie->unum() != rcsc::Unum_Unknown
             && opp_goalie->distFromSelf() < 25.0
             && opp_goalie->pos().x > 52.5 - 16.0
             && opp_goalie->pos().x < 52.5
             && opp_goalie->pos().absY() < 20.0 )
        {
            addSayMessage( new rcsc::BallGoalieMessage( effector().queuedNextBallPos(),
                                                        effector().queuedNextBallVel(),
                                                        opp_goalie->pos(),
                                                        opp_goalie->body() ) );
            return true;
        }
    }

    if ( send_ball )
    {
        if ( world().self().isKickable()
             && effector().queuedNextBallKickable() )
        {
            // set ball velocity to 0.
            addSayMessage( new rcsc::BallMessage( effector().queuedNextBallPos(),
                                                  rcsc::Vector2D( 0.0, 0.0 ) ) );
        }
        else
        {
            addSayMessage( new rcsc::BallMessage( effector().queuedNextBallPos(),
                                                  effector().queuedNextBallVel() ) );
        }
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::sayGoalie()
{
    // goalie info: ball is in chance area
    int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::GoalieMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( world().ball().pos().x < 34.0
         || world().ball().pos().absY() > 20.0 )
    {
        return false;
    }

    const rcsc::PlayerObject * opp_goalie = world().getOpponentGoalie();
    if ( opp_goalie
         && opp_goalie->posCount() == 0
         && opp_goalie->bodyCount() == 0
         && opp_goalie->unum() != rcsc::Unum_Unknown
         && opp_goalie->distFromSelf() < 25.0 )
    {
        addSayMessage( new rcsc::GoalieMessage( opp_goalie->unum(),
                                                opp_goalie->pos(),
                                                opp_goalie->body() ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::sayIntercept()
{
    // intercept info
    const int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::InterceptMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( world().ball().posCount() > 0
         || world().ball().velCount() > 0 )
    {
        return false;
    }

    int self_min = world().interceptTable()->selfReachCycle();
    int mate_min = world().interceptTable()->teammateReachCycle();
    int opp_min = world().interceptTable()->opponentReachCycle();

    if ( world().self().isKickable() )
    {
        double next_dist =  effector().queuedNextMyPos().dist( effector().queuedNextBallPos() );
        if ( next_dist > world().self().playerType().kickableArea() )
        {
            self_min = 10000;
        }
        else
        {
            self_min = 0;
        }
    }

    if ( self_min <= mate_min
         && self_min <= opp_min
         && self_min <= 10 )
    {
        addSayMessage( new rcsc::InterceptMessage( true,
                                                   world().self().unum(),
                                                   self_min ) );
        return true;
    }
    
    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::sayOffsideLine()
{
    const int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::OffsideLineMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( world().offsideLineCount() <= 1
         && world().opponentsFromSelf().size() >= 8
         && 0.0 < world().ball().pos().x
         && world().ball().pos().x < 37.0
         && world().ball().pos().x > world().offsideLineX() - 20.0
         && std::fabs( world().self().pos().x - world().offsideLineX() ) < 20.0
         )
    {
        addSayMessage( new rcsc::OffsideLineMessage( world().offsideLineX() ) );
        return true;
    }

    return false;
}


/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::sayDefenseLine()
{
    const int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::DefenseLineMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( std::fabs( world().self().pos().x - world().ourDefenseLineX() ) > 40.0 )
    {
        return false;
    }

    int opp_min = world().interceptTable()->opponentReachCycle();

    rcsc::Vector2D opp_trap_pos = world().ball().inertiaPoint( opp_min );

    if ( world().self().goalie()
         && -40.0 < opp_trap_pos.x
         && opp_trap_pos.x < 10.0 )
    {
        addSayMessage( new rcsc::DefenseLineMessage( world().ourDefenseLineX() ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
SamplePlayer::saySelf()
{
    const int len = effector().getSayMessageLength();
    if ( static_cast< int >( len + rcsc::SelfMessage::slength() )
         > rcsc::ServerParam::i().playerSayMsgSize() )
    {
        return false;
    }

    if ( std::fabs( world().self().pos().x - world().ourDefenseLineX() ) > 40.0 )
    {
        return false;
    }

    bool send_self = false;

    if ( len > 0 )
    {
        send_self = true;
    }

    if ( ! send_self
        && world().time().cycle() % 3 == world().self().unum() % 3 )
    {
        const rcsc::PlayerObject * ball_nearest_teammate = NULL;
        const rcsc::PlayerObject * second_ball_nearest_teammate = NULL;

        const rcsc::PlayerPtrCont::const_iterator t_end = world().teammatesFromBall().end();
        for ( rcsc::PlayerPtrCont::const_iterator t = world().teammatesFromBall().begin();
              t != t_end;
              ++t )
        {
            if ( (*t)->isGhost() || (*t)->posCount() >= 10 ) continue;

            if ( ! ball_nearest_teammate )
            {
                ball_nearest_teammate = *t;
            }
            else if ( ! second_ball_nearest_teammate )
            {
                second_ball_nearest_teammate = *t;
                break;
            }
        }

        if ( ball_nearest_teammate
             && ball_nearest_teammate->distFromBall() < world().ball().distFromSelf()
             && ( ! second_ball_nearest_teammate
                  || second_ball_nearest_teammate->distFromBall() > world().ball().distFromSelf() )
             )
        {
            send_self = true;
        }
    }

    if ( send_self )
    {
        addSayMessage( new rcsc::SelfMessage( effector().queuedNextMyPos(),
                                              effector().queuedNextMyBody(),
                                              world().self().stamina() ) );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SamplePlayer::attentiontoSomeone()
{
    if ( world().self().pos().x > world().offsideLineX() - 15.0
         && world().interceptTable()->selfReachCycle() <= 3 )
    {
        std::vector< const rcsc::PlayerObject * > candidates;
        const rcsc::PlayerPtrCont::const_iterator end = world().teammatesFromSelf().end();
        for ( rcsc::PlayerPtrCont::const_iterator p = world().teammatesFromSelf().begin();
              p != end;
              ++p )
        {
            if ( (*p)->goalie() ) continue;
            if ( (*p)->unum() == rcsc::Unum_Unknown ) continue;
            if ( (*p)->pos().x > world().offsideLineX() + 1.0 ) continue;

            if ( (*p)->distFromSelf() > 20.0 ) break;

            candidates.push_back( *p );
        }

        const rcsc::PlayerObject * target_teammate = NULL;
        double max_x = -100.0;
        for ( std::vector< const rcsc::PlayerObject * >::const_iterator p = candidates.begin();
              p != candidates.end();
              ++p )
        {
            if ( (*p)->pos().x > max_x )
            {
                max_x = (*p)->pos().x;
                target_teammate = *p;
            }
        }

        if ( target_teammate )
        {
            doAttentionto( world().ourSide(), target_teammate->unum() );
            return;
        }

        // maybe ball owner
        if ( world().self().attentiontoUnum() > 0 )
        {
            doAttentiontoOff();
        }
        return;
    }

    {
        const rcsc::PlayerObject * fastest_teammate = world().interceptTable()->fastestTeammate();

        int self_min = world().interceptTable()->selfReachCycle();
        int mate_min = world().interceptTable()->teammateReachCycle();
        int opp_min = world().interceptTable()->opponentReachCycle();

        if ( ! fastest_teammate )
        {
            if ( world().self().attentiontoUnum() > 0 )
            {
                doAttentiontoOff();
            }
            return;
        }

        if ( mate_min < self_min
             && mate_min <= opp_min
             && fastest_teammate->unum() != rcsc::Unum_Unknown )
        {
            doAttentionto( world().ourSide(), fastest_teammate->unum() );
            return;
        }
    }

    {
        const rcsc::PlayerObject * nearest_teammate = world().getTeammateNearestToBall( 5 );
        if ( nearest_teammate
             && nearest_teammate->distFromBall() < 20.0
             && nearest_teammate->unum() != rcsc::Unum_Unknown )
        {
            doAttentionto( world().ourSide(), nearest_teammate->unum() );
            return;
        }
    }
    
    doAttentiontoOff();
}

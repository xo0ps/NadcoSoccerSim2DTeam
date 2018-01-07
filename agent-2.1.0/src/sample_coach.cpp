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
#include "config.h"
#endif

#include "sample_coach.h"

#include <rcsc/coach/coach_command.h>
#include <rcsc/coach/coach_config.h>
#include <rcsc/coach/global_world_model.h>

#include <rcsc/common/basic_client.h>
#include <rcsc/common/player_param.h>
#include <rcsc/common/server_param.h>
#include <rcsc/common/player_type.h>
#include <rcsc/common/audio_memory.h>
#include <rcsc/common/say_message_parser.h>

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

#include <cstdio>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

#include "team_logo.xpm"

#define USE_HETERO_GOALIE
/////////////////////////////////////////////////////////

struct TypeStaminaIncComp
    : public std::binary_function< const rcsc::PlayerType *,
                                   const rcsc::PlayerType *,
                                   bool > {

    result_type operator()( first_argument_type lhs,
                            second_argument_type rhs ) const
      {
          return lhs->staminaIncMax() < rhs->staminaIncMax();
      }

};

/////////////////////////////////////////////////////////

struct RealSpeedMaxCmp
    : public std::binary_function< const rcsc::PlayerType *,
                                   const rcsc::PlayerType *,
                                   bool > {

    result_type operator()( first_argument_type lhs,
                            second_argument_type rhs ) const
      {
          if ( std::fabs( lhs->realSpeedMax() - rhs->realSpeedMax() ) < 0.005 )
          {
              return lhs->cyclesToReachMaxSpeed() < rhs->cyclesToReachMaxSpeed();
          }

          return lhs->realSpeedMax() > rhs->realSpeedMax();
      }

};

/////////////////////////////////////////////////////////

struct MaxSpeedReachStepCmp
    : public std::binary_function< const rcsc::PlayerType *,
                                   const rcsc::PlayerType *,
                                   bool > {

    result_type operator()( first_argument_type lhs,
                            second_argument_type rhs ) const
      {
          return lhs->cyclesToReachMaxSpeed() < rhs->cyclesToReachMaxSpeed();
      }

};

/*-------------------------------------------------------------------*/
/*!

*/
SampleCoach::SampleCoach()
    : CoachAgent()
    , M_substitute_count( 0 )
{
    //
    // register audio memory & say message parsers
    //

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

    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 9 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 8 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 7 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 6 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 5 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 4 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 3 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 2 >( audio_memory ) ) );
    // addSayMessageParser( SayMessageParser::Ptr( new FreeMessageParser< 1 >( audio_memory ) ) );

    //
    //
    //

    for ( int i = 0; i < 11; ++i )
    {
        // init map values
        M_assigned_player_type_id[i] = rcsc::Hetero_Default;
    }

    for ( int i = 0; i < 11; ++i )
    {
        M_opponent_player_types[i] = rcsc::Hetero_Default;
    }

    for ( int i = 0; i < 11; ++i )
    {
        M_teammate_recovery[i] = rcsc::ServerParam::i().recoverInit();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
SampleCoach::~SampleCoach()
{

}

/*-------------------------------------------------------------------*/
/*!

 */
bool
SampleCoach::initImpl( rcsc::CmdLineParser & cmd_parser )
{
    bool result =CoachAgent::initImpl( cmd_parser );

#if 0
    ParamMap my_params;
    if ( cmd_parser.count( "help" ) )
    {
       my_params.printHelp( std::cout );
       return false;
    }
    cmd_parser.parse( my_params );
#endif

    if ( cmd_parser.failed() )
    {
        //std::cerr << "coach: ***WARNING*** detected unsupported options: ";
        //cmd_parser.print( std::cerr );
        //std::cerr << std::endl;
    }

    if ( ! result )
    {
        return false;
    }

    //////////////////////////////////////////////////////////////////
    // Add your code here.
    //////////////////////////////////////////////////////////////////

    if ( config().useTeamGraphic() )
    {
        if ( config().teamGraphicFile().empty() )
        {
            M_team_graphic.createXpmTiles( team_logo_xpm );
        }
        else
        {
            M_team_graphic.readXpmFile( config().teamGraphicFile().c_str() );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SampleCoach::actionImpl()
{
    if ( world().time().cycle() == 0
         && config().useTeamGraphic()
         && M_team_graphic.tiles().size() != teamGraphicOKSet().size() )
    {
        sendTeamGraphic();
    }

	updateRecoveryInfo();
    doSubstitute();
    sayPlayerTypes();
//     if ( world().time().cycle() > 0 )
//     {
//         M_client->setServerAlive( false );
//     }
	
    TeamStatistics();
    DangerousOpponents();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SampleCoach::updateRecoveryInfo()
{
    const int half_time = rcsc::ServerParam::i().halfTime() * 10;
    const int normal_time = half_time * rcsc::ServerParam::i().nrNormalHalfs();

    if ( world().time().cycle() < normal_time
         && world().gameMode().type() == rcsc::GameMode::BeforeKickOff )
    {
        for ( int i = 0; i < 11; ++i )
        {
            M_teammate_recovery[i] = rcsc::ServerParam::i().recoverInit();
        }

        return;
    }

    if ( world().audioMemory().recoveryTime() != world().time() )
    {
        return;
    }

#if 0
    //std::cerr << world().time() << ": heared recovery:\n";
    for ( std::vector< rcsc::AudioMemory::Recovery >::const_iterator it = world().audioMemory().recovery().begin();
          it != world().audioMemory().recovery().end();
          ++it )
    {
        double recovery
            = it->rate_ * ( ServerParam::i().recoverInit() - ServerParam::i().recoverMin() )
            + ServerParam::i().recoverMin();

            //std::cerr << "  sender=" << it->sender_
            //          << " rate=" << it->rate_
            //          << " value=" << recovery
            //          << '\n';

    }
    //std::cerr << std::flush;
#endif
    const std::vector< rcsc::AudioMemory::Recovery >::const_iterator end = world().audioMemory().recovery().end();
    for ( std::vector< rcsc::AudioMemory::Recovery >::const_iterator it = world().audioMemory().recovery().begin();
          it != end;
          ++it )
    {
        if ( 1 <= it->sender_
             && it->sender_ <= 11 )
        {
            double value
                = it->rate_ * ( rcsc::ServerParam::i().recoverInit() - rcsc::ServerParam::i().recoverMin() )
                + rcsc::ServerParam::i().recoverMin();
            // std::cerr << "coach: " << world().time() << " heared recovery: sender=" << it->sender_
            //           << " value=" << value << std::endl;
            M_teammate_recovery[ it->sender_ - 1 ] = value;
        }
    }
}

/*--------------------------------------------*/

void
SampleCoach::doSubstitute()
{
    static bool S_first_substituted = false;

    if ( ! S_first_substituted
         && world().time().cycle() == 0
         && world().time().stopped() > 10 )
    {
        doFirstSubstitute();
        S_first_substituted = true;

        return;
    }

    if ( world().time().cycle() > 0
         && world().gameMode().type() != rcsc::GameMode::PlayOn
         && ! world().gameMode().isPenaltyKickMode() )
    {
        doSubstituteTiredPlayers();

        return;
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SampleCoach::doFirstSubstitute()
{
    PlayerTypePtrCont candidates;

    for ( rcsc::PlayerTypeSet::PlayerTypeMap::const_iterator it = rcsc::PlayerTypeSet::i().playerTypeMap().begin();
          it != rcsc::PlayerTypeSet::i().playerTypeMap().end();
          ++it )
    {
        if ( it->second.id() == rcsc::Hetero_Default )
        {
            if ( rcsc::PlayerParam::i().allowMultDefaultType() )
            {
                for ( int i = 0; i <= rcsc::MAX_PLAYER; ++i )
                {
                    M_available_player_type_id.push_back( rcsc::Hetero_Default );
                    candidates.push_back( &(it->second) );
                }
            }
            else
            {
#ifdef USE_HETERO_GOALIE
                const int pt_max = ( config().version() < 14.0
                                     ? rcsc::PlayerParam::i().ptMax() - 1
                                     : rcsc::PlayerParam::i().ptMax() );
                for ( int i = 0; i < pt_max; ++i )
                {
                    M_available_player_type_id.push_back( it->second.id() );
                    candidates.push_back( &(it->second) );
                }
#endif
            }
        }
        else
        {
            for ( int i = 0; i < rcsc::PlayerParam::i().ptMax(); ++i )
            {
                M_available_player_type_id.push_back( it->second.id() );
                candidates.push_back( &(it->second) );
            }
        }
    }


    std::vector< int > unum_order;
    unum_order.reserve( 11 );

#ifdef USE_HETERO_GOALIE
    if ( config().version() >= 14.0 )
    {
        unum_order.push_back( 1 ); // goalie
    }
#endif

#if 0
    // side back has priority
    unum_order.push_back( 11 );
    unum_order.push_back( 2 );
    unum_order.push_back( 3 );
    unum_order.push_back( 4 );
    unum_order.push_back( 5 );
    unum_order.push_back( 10 );
    unum_order.push_back( 9 );
    unum_order.push_back( 6 );
    unum_order.push_back( 7 );
    unum_order.push_back( 8 );
#else
    // wing player has priority
    unum_order.push_back( 11 );
    unum_order.push_back( 2 );
    unum_order.push_back( 3 );
    unum_order.push_back( 10 );
    unum_order.push_back( 9 );
    unum_order.push_back( 6 );
    unum_order.push_back( 4 );
    unum_order.push_back( 5 );
    unum_order.push_back( 7 );
    unum_order.push_back( 8 );
#endif

    for ( std::vector< int >::iterator unum = unum_order.begin();
          unum != unum_order.end();
          ++unum )
    {
        int type = getFastestType( candidates );
        if ( type != rcsc::Hetero_Unknown )
        {
            substituteTo( *unum, type );
        }
    }
}

/*----------------------------------------*/
void
SampleCoach::doSubstituteTiredPlayers()
{
    if ( M_substitute_count >= rcsc::PlayerParam::i().subsMax() )
    {
        // over the maximum substitution
        return;
    }

    const rcsc::ServerParam & SP = rcsc::ServerParam::i();

    //
    // check game time
    //
    const int half_time = SP.halfTime() * 10;
    const int normal_time = half_time * SP.nrNormalHalfs();

    if ( world().time().cycle() < normal_time - 500
         || world().time().cycle() <= half_time + 1
         || world().gameMode().type() == rcsc::GameMode::KickOff_ )
    {
        return;
    }

    //
    // create candidate teamamte
    //
    std::vector< int > tired_teammate_unum;

    for ( int i = 0; i < 11; ++i )
    {
		/*
		const rcsc::GlobalPlayerObject * tm = world().teammate( i + 1 );
		if( ! tm )
			continue;
		double left = ( M_teammate_recovery[i] + tm->stamina() ) / ( 6000 - world().time().cycle() );
		double right = ( rcsc::ServerParam::i().recoverInit() - M_teammate_recovery[i] ) / world().time().cycle();
		if( left < right )
		*/
        if ( M_teammate_recovery[i] < rcsc::ServerParam::i().recoverInit() - 0.002 )
        {
            tired_teammate_unum.push_back( i + 1 );
        }
    }

    if ( tired_teammate_unum.empty() )
    {
        return;
    }

    //
    // create candidate player type
    //
    PlayerTypePtrCont candidates;

    for ( std::vector< int >::const_iterator id = M_available_player_type_id.begin();
          id !=  M_available_player_type_id.end();
          ++id )
    {
        const rcsc::PlayerType * ptype = rcsc::PlayerTypeSet::i().get( *id );
        if ( ! ptype )
        {
            continue;
        }

        candidates.push_back( ptype );
    }

    //
    // try substitution
    //
    for ( std::vector< int >::iterator unum = tired_teammate_unum.begin();
          unum != tired_teammate_unum.end();
          ++unum )
    {
        if ( M_substitute_count >= rcsc::PlayerParam::i().subsMax() )
        {
            // over the maximum substitution
            return;
        }

        int type = getFastestType( candidates );
        if ( type != rcsc::Hetero_Unknown )
        {
            substituteTo( *unum, type );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
SampleCoach::substituteTo( const int unum,
                           const int type )
{
    if ( world().time().cycle() > 0
         && M_substitute_count >= rcsc::PlayerParam::i().subsMax() )
    {
        std::cerr << "***Warning*** "
                  << config().teamName() << " coach: over the substitution max."
                  << " cannot change the player " << unum
                  << " to type " << type
                  << std::endl;
        return;
    }

    std::vector< int >::iterator it = std::find( M_available_player_type_id.begin(),
                                                 M_available_player_type_id.end(),
                                                 type );
    if ( it == M_available_player_type_id.end() )
    {
        std::cerr << "***ERROR*** "
                  << config().teamName() << " coach: "
                  << " cannot change the player " << unum
                  << " to type " << type
                  << std::endl;
        return;
    }

    M_available_player_type_id.erase( it );
    M_assigned_player_type_id.insert( std::make_pair( unum, type ) );
    if ( world().time().cycle() > 0 )
    {
        ++M_substitute_count;
    }

    doChangePlayerType( unum, type );

	if ( 1 <= unum && unum <= 11 )
    {
        M_teammate_recovery[unum-1] = rcsc::ServerParam::i().recoverInit();
    }
    
    std::cout << config().teamName() << " coach: "
              << "change player " << unum
              << " to type " << type
              << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

 */
int
SampleCoach::getFastestType( PlayerTypePtrCont & candidates )
{
    if ( candidates.empty() )
    {
        return rcsc::Hetero_Unknown;
    }

    // sort by max speed
    std::sort( candidates.begin(),
               candidates.end(),
               RealSpeedMaxCmp() );

    PlayerTypePtrCont::iterator best_type = candidates.begin();
    double max_speed = (*best_type)->realSpeedMax();
    int min_cycle = 100;
    for ( PlayerTypePtrCont::iterator it = candidates.begin();
          it != candidates.end();
          ++it )
    {
        if ( (*it)->realSpeedMax() < max_speed - 0.01 )
        {
            break;
        }

        if ( (*it)->cyclesToReachMaxSpeed() < min_cycle )
        {
            best_type = it;
            max_speed = (*best_type)->realSpeedMax();
            min_cycle = (*best_type)->cyclesToReachMaxSpeed();
            continue;
        }

        if ( (*it)->cyclesToReachMaxSpeed() == min_cycle )
        {
            if ( (*it)->getOneStepStaminaComsumption( rcsc::ServerParam::i() )
                 < (*best_type)->getOneStepStaminaComsumption( rcsc::ServerParam::i()) )
            {
                best_type = it;
                max_speed = (*best_type)->realSpeedMax();
            }
        }
    }

    int id = (*best_type)->id();
    candidates.erase( best_type );
    return id;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SampleCoach::sayPlayerTypes()
{
    /*
      format:
      "(player_types (1 0) (2 1) (3 2) (4 3) (5 4) (6 5) (7 6) (8 -1) (9 0) (10 1) (11 2))"
      ->
      (say (freeform "(player_type ...)"))
    */

    static rcsc::GameTime s_last_send_time( 0, 0 );

    if ( ! config().useFreeform() )
    {
        return;
    }

    if ( ! world().canSendFreeform() )
    {
        return;
    }

    int analyzed_count = 0;

    for ( int unum = 1; unum <= 11; ++unum )
    {
        const int id = world().heteroID( world().theirSide(), unum );

        if ( id != M_opponent_player_types[unum - 1] )
        {
            M_opponent_player_types[unum - 1] = id;

            if ( id != rcsc::Hetero_Unknown )
            {
                ++analyzed_count;
            }
        }
    }

    if ( analyzed_count == 0 )
    {
        return;
    }

    std::string msg;
    msg.reserve( 128 );

    msg = "(player_types ";

    for ( int unum = 1; unum <= 11; ++unum )
    {
        char buf[8];
        snprintf( buf, 8, "(%d %d)",
                  unum, M_opponent_player_types[unum - 1] );
        msg += buf;
    }

    msg += ")";

    doSayFreeform( msg );

    s_last_send_time = world().time();

    //std::cout << config().teamName()
    //          << " Coach: "
    //          << world().time()
    //          << " send freeform " << msg
    //          << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
SampleCoach::sendTeamGraphic()
{
    int count = 0;
    for ( rcsc::TeamGraphic::Map::const_reverse_iterator tile
              = M_team_graphic.tiles().rbegin();
          tile != M_team_graphic.tiles().rend();
          ++tile )
    {
        if ( teamGraphicOKSet().find( tile->first ) == teamGraphicOKSet().end() )
        {
            if ( ! doTeamGraphic( tile->first.first,
                                  tile->first.second,
                                  M_team_graphic ) )
            {
                break;
            }
            ++count;
        }
    }

    if ( count > 0 )
    {
        std::cout << config().teamName()
                  << " coach: "
                  << world().time()
                  << " send team_graphic " << count << " tiles"
                  << std::endl;
    }
}

/*--------------------------------------------------*/
void
SampleCoach::TeamStatistics()
{
	
	if( world().time().cycle() == 0 )
		return;
	
	static int our_kick = 0;
	static int opp_kick = 0;
	static int danger = 0;
	static int def_mid = 0;
	static int off_mid = 0;
	static int shootchance = 0;
	static int our_yellow[12];
	static int opp_yellow[12];
	static int our_red[12];
	static int opp_red[12];
	
	for( int i = 0 ; i < 12 ; i++ )
	{
		our_yellow[i] = 0;
		opp_yellow[i] = 0;
		our_red[i] = 0;
		opp_red[i] = 0;
	}
	
	if( world().gameMode().type() == rcsc::GameMode::PenaltySetup_
	 || world().gameMode().type() == rcsc::GameMode::PenaltyReady_
	 || world().gameMode().type() == rcsc::GameMode::PenaltyTaken_
	 || world().gameMode().type() == rcsc::GameMode::PenaltyMiss_
	 || world().gameMode().type() == rcsc::GameMode::PenaltyScore_ )
	{
		return;
	}
	
	if( world().gameMode().type() == rcsc::GameMode::PlayOn )
	{
		const std::vector< const rcsc::GlobalPlayerObject * >::const_iterator end = world().opponents().end();
		for( std::vector< const rcsc::GlobalPlayerObject * >::const_iterator it = world().opponents().begin(); it != end ; it++ )
		{
			if ( (*it)->pos().dist( world().ball().pos() ) < 0.7 )
				opp_kick++;
			if( world().isOpponentYellowCarded( (*it)->unum() ) )
				opp_yellow[ (*it)->unum() ] = 1;
			if( world().isOpponentRedCarded( (*it)->unum() ) )
				opp_red[ (*it)->unum() ] = 1;
		}
		
		const std::vector< const rcsc::GlobalPlayerObject * >::const_iterator end1 = world().teammates().end();
		for( std::vector< const rcsc::GlobalPlayerObject * >::const_iterator it = world().teammates().begin(); it != end1 ; it++ )
		{
			if ( (*it)->pos().dist( world().ball().pos() ) <= 1.0 )
				our_kick++;
			if( world().isTeammateYellowCarded( (*it)->unum() ) )
				our_yellow[ (*it)->unum() ] = 1;
			if( world().isTeammateRedCarded( (*it)->unum() ) )
				our_red[ (*it)->unum() ] = 1;
		}
		
		if( world().ball().pos().x > 36.0 )
			shootchance++;
		else
		if( world().ball().pos().x > 0.0 )
			off_mid++;
		else
		if( world().ball().pos().x > -36.0 )
			def_mid++;
		else
			danger++;	
	
	}
	
	
	int danger_per = 100 * danger / world().time().cycle();
	int def_per = 100 * def_mid / world().time().cycle();
	int off_mid_per = 100 * off_mid / world().time().cycle();
	int shootchance_per = 100 * shootchance / world().time().cycle();
	int diff = 100 - danger_per - def_per - off_mid_per - shootchance_per;
	if( diff != 0 )
	{
		if( diff % 2 == 0 )
		{
			off_mid_per += diff / 2;
			def_per += diff / 2;
		}
		else
		{
			off_mid_per += diff / 2;
			def_per += diff / 2 + 1;
		}
	}
	int our_kick_per = 100 * our_kick / world().time().cycle();
	int opp_kick_per = 100 * opp_kick / world().time().cycle();
	int no_per = ( 100 - our_kick_per - opp_kick_per ) / 2.0;
	our_kick_per += no_per;
	opp_kick_per += no_per;
	diff = 100 - our_kick_per - opp_kick_per;
	if( diff != 0 )
		our_kick_per += diff;
	
	int our_yellow_cards = 0;
	int opp_yellow_cards = 0;
	int our_red_cards = 0;
	int opp_red_cards = 0;
	
	for( int i = 0 ; i < 12 ; i++ )
	{
		if( our_yellow[i] == 1 )
			our_yellow_cards++;
		if( opp_yellow[i] == 1 )
			opp_yellow_cards++;
		if( our_red[i] == 1 )
			our_red_cards++;
		if( opp_red[i] == 1 )
			opp_red_cards++;
	}
	
	if( world().time().cycle() % 1499 == 0 )
	{
		std::cout<<std::endl;
		std::cout<<"---------------------"<<std::endl;
		std::cout<<"Our Danger   : "<<danger_per<<"%"<<std::endl;
		std::cout<<"Our Middle   : "<<def_per<<"%"<<std::endl;
		std::cout<<"Their Middle : "<<off_mid_per<<"%"<<std::endl;
		std::cout<<"Their Danger : "<<shootchance_per<<"%"<<std::endl;
		std::cout<<std::endl;
		std::cout<<"Our Ball     : "<<our_kick_per<<"%"<<std::endl;
		std::cout<<"Their Ball   : "<<opp_kick_per<<"%"<<std::endl;
		std::cout<<std::endl;
		std::cout<<"Our Yellow   : "<<our_yellow_cards<<std::endl;
		std::cout<<"Their Yellow : "<<opp_yellow_cards<<std::endl;
		std::cout<<"Our Red      : "<<our_red_cards<<std::endl;
		std::cout<<"Their Red    : "<<opp_red_cards<<std::endl;
		std::cout<<"---------------------"<<std::endl;
		std::cout<<std::endl;
	}

}

/*-----------------------------------------------------------*/

void
SampleCoach::DangerousOpponents()
{
	if ( world().gameMode().type() == rcsc::GameMode::PlayOn
	&& world().time().cycle() >= 40 && world().ball().pos().x < -30 )
	{
		std::vector< const rcsc::GlobalPlayerObject * >opp;
		std::vector< const rcsc::GlobalPlayerObject * >tmp;
		rcsc::Vector2D our_target( -52.5 , 0.0 );
		int our_line_x = -30;
		
		const std::vector< const rcsc::GlobalPlayerObject * >::const_iterator end = world().opponents().end();
		for( std::vector< const rcsc::GlobalPlayerObject * >::const_iterator it = world().opponents().begin(); it != end ; it++ )
		{
			if ( (*it)->pos().x < our_line_x )
				tmp.push_back( (*it) );
		}
		
		while ( tmp.size() > 0 )
		{
			double minDist = 1000;
			const rcsc::GlobalPlayerObject * p;
			std::vector< const rcsc::GlobalPlayerObject * >::iterator clear;
			const std::vector< const rcsc::GlobalPlayerObject * >::iterator end = tmp.end();
			for( std::vector< const rcsc::GlobalPlayerObject * >::iterator it = tmp.begin(); it != end ; it++)
			{
				if( (*it)->pos().dist( our_target ) < minDist )
				{
					clear = it;
					p = (*it);
					minDist = (*it)->pos().dist( our_target );
				}
			}
			opp.push_back( p );
			tmp.erase( clear );
		}

		std::vector< const rcsc::GlobalPlayerObject * >teammates;
		const std::vector< const rcsc::GlobalPlayerObject * >::const_iterator end1 = world().teammates().end();
		for( std::vector< const rcsc::GlobalPlayerObject * >::const_iterator it = world().teammates().begin(); it != end1 ; it++ )
		{
			if ( (*it)->unum() >= 2 && (*it)->unum() <= 8 )
				teammates.push_back( (*it) );
		}
		
		std::vector< std::pair < int, int > >mark;
		while( opp.size() > 0 && teammates.size() > 0 )
		{
			double minDist = 1000;
			std::vector< const rcsc::GlobalPlayerObject * >::iterator i = opp.begin();
			std::vector< const rcsc::GlobalPlayerObject * >::iterator clear;
			const std::vector< const rcsc::GlobalPlayerObject * >::iterator end = teammates.end();
			for( std::vector< const rcsc::GlobalPlayerObject * >::iterator j = teammates.begin(); j != end; j++)
			{
				if ( (*i)->pos().dist( (*j)->pos() ) < minDist )
				{
					clear = j;
					minDist = (*i)->pos().dist( (*j)->pos() );
				}
			}
			mark.push_back( std::make_pair( (*clear)->unum(), (*i)->unum() ) );
			teammates.erase( clear );
			opp.erase( i );
			i++;
		}
		
		teammates.clear();
		opp.clear();
		
		std::ostringstream o;
		while( mark.size() > 0 )
		{
			int minNum = 1000;
			std::vector< std::pair< int, int > >::iterator clear;
			const std::vector< std::pair< int, int > >::iterator end = mark.end();
			for( std::vector< std::pair< int, int > >::iterator it = mark.begin(); it != end; it++ )
			{
				if ( it->first < minNum )
				{
					clear = it;
					minNum = it->first;
				}
			}

			if ( clear->second == -1 )
				o << "C";
			else
			if( clear->second == 10 )
				o << "A";
			else
			if( clear->second == 11 )
				o << "B";
			else
				o << clear->second;

			mark.erase( clear );
		}

		static int last = -1;
		static int counter = 0;
		if( world().time().cycle() - last > 500 && counter < 6 )
		{
			//std::cout<<"(say (info (" << world().time().cycle()
			//		<< " (true) " << '"' << o.str() << '"' << ")))"<<std::endl;
			last = world().time().cycle();
			counter++;
			//M_client->sendMessage(s.str().c_str());
			std::string msg = "(say (info (";
			msg += world().time().cycle();
			msg += " (true) ";
			msg += '"';
			msg += o.str();
			msg += '"';
			msg += ")))";
			//doSayFreeform( msg );
		}
	}
}

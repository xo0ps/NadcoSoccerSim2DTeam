// -*-c++-*-

/*!
  \file view_synch.cpp
  \brief synchronize view frequency with server cycle.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "view_synch.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/see_state.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

*/
bool
View_Synch::execute( PlayerAgent * agent )
{
    if ( SeeState::synch_see_mode() )
    {
        return agent->doChangeView( ViewWidth::NARROW );
    }

    if ( ! agent->seeState().isSynch() )
    {

        if ( agent->world().gameMode().type() != GameMode::PlayOn )
        {
            return false;
        }

        return doTimerSynchView( agent );
    }

    if ( agent->seeState().lastTiming() == SeeState::TIME_0_00 )
    {
        return agent->doChangeView( ViewWidth::NORMAL );
    }
    else
    {
        return agent->doChangeView( ViewWidth::NARROW );
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
View_Synch::doTimerSynchView( PlayerAgent * agent )
{
    if ( ServerParam::i().synchMode() )
    {
        return agent->doChangeView( ViewWidth::NARROW );
    }

    if ( agent->bodyTimeStamp().sec() <= 0 )
    {
        // have not received sense_body.
        return agent->doChangeView( ViewWidth::NARROW );
    }

    if ( agent->world().seeTime() != agent->world().time() )
    {
        return agent->doChangeView( ViewWidth::NARROW );
    }

    if ( agent->world().self().viewWidth() != ViewWidth::NARROW )
    {
        return agent->doChangeView( ViewWidth::NARROW );
    }

    double msec_diff_real
        = agent->seeTimeStamp().getRealMSecDiffFrom( agent->bodyTimeStamp() );
    msec_diff_real
        /= static_cast< double >( ServerParam::i().slowDownFactor() );

    const long msec_diff
        = static_cast< long >( rint( msec_diff_real ) );

    // adjust see step depending on time diffrence between sense_body & see
    //if ( msec_diff < 15 ) // msec + 150 + 10 < 100 * 2 - 25
    //if ( msec_diff < 17 ) // msec + 150 -> 100 + 67
    //if ( msec_diff < 20 ) // msec + 150 -> 100 + 70
    if ( msec_diff < agent->config().normalViewTimeThr() )
    {
        // msec + Normal_High_Step + Server_Recv_Step < Server_Step * 2 - Think_Time
        return agent->doChangeView( ViewWidth::NORMAL );
    }

    // nothing to do
    return true;
}

}

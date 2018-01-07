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

#include <iostream>
#include <cstdlib> // exit
#include <cerrno> // errno
#include <cstring> // strerror
#include <csignal> // sigaction

#include <rcsc/common/basic_client.h>

#include "sample_player.h"
#include "sample_coach.h"

namespace {

SamplePlayer agent;
SampleCoach coach;

/*-------------------------------------------------------------------*/
void
sig_exit_handle( int )
{
    std::cerr << "Killed. Exiting..." << std::endl;
    agent.finalize();
    coach.finalize();
    std::exit( EXIT_FAILURE );
}

}

/*-------------------------------------------------------------------*/
int
main( int argc, char **argv )
{
    struct sigaction sig_action;
    sig_action.sa_handler = &sig_exit_handle;
    sig_action.sa_flags = 0;
    if ( sigaction( SIGINT, &sig_action , NULL ) != 0
         || sigaction( SIGTERM, &sig_action , NULL ) != 0
         || sigaction( SIGHUP, &sig_action , NULL ) != 0 )
    {
        std::exit( EXIT_FAILURE );
    }

    rcsc::BasicClient client;

	if( argc >= 2 && ( ! strcmp( argv[1] , "--coach" ) ) )
	{
		if ( ! coach.init( &client, argc, argv ) )
	    {
	        return EXIT_FAILURE;
	    }
	
	    client.run( &coach );
	}
	else
	{
		if ( ! agent.init( &client, argc, argv ) )
	    {
	        return EXIT_FAILURE;
	    }
	
	    client.run( &agent );
	}
	
    return EXIT_SUCCESS;
}

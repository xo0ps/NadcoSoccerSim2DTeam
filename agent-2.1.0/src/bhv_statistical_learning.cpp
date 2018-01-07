// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Mahdi SADEGHI
 Copyright (C) Samira KARIMZADEH
 Copyright (C) Elham IRAVANI

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

#include "bhv_statistical_learning.h"
#include "sample_player.h"
#include "strategy.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include <rcsc/player/player_agent.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/sector_2d.h>

/*-------------------------------------------------------------------*/
/*!

*/

#include<vector>
#include<string>
#include<rcsc/action/body_go_to_point.h>
#include<iostream>
#include<iomanip>
using std::setw;
using std::cout;
using std::vector;
using std::string;

vector<string> learn_data1;
vector<string> learn_data2;
vector<string> learn_data3;
vector<string> learn_data4;
vector<string> learn_data5;
vector<string> learn_data6;
vector<string> learn_data7;
vector<string> normalized;
static int data_counter= 1;

const int row = 17;
const int col = 35;
static int dataMatrix [ row ][ col ];


void Bhv_StatisticalLearning::resetDataMatrix ()
{

    for ( int  i=0; i< row; i++ )
	for ( int j=0; j < col; j++)
	    dataMatrix [i][j] = 0;

    return;
}
bool Bhv_StatisticalLearning::exist(vector<string> & a , string& value)
{
    if ( a.empty() ) return false;
    vector<string>::const_iterator it;
    for( it = a.begin(); it != a.end(); ++it )
	if( (*it) == value ) return true;
    return false;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void Bhv_StatisticalLearning::decode(string st, int &i, int &j)
{
    char ci[3],cj[3];
    ci[0] = st[0]; ci[1] = st[1]; ci[2] = '\0';
    cj[0] = st[2]; cj[1] = st[3]; cj[2] = '\0';
    i = atoi(ci);
    j = atoi(cj);
}

// not safe ha negah dari mishan
void Bhv_StatisticalLearning::normalize_learnDataToVector()
{
    vector<string>::const_iterator itbeg;
    vector<string>::const_iterator itend;
    int i = 0 ,j = 0;
    int count;
    switch( data_counter )
    {
	case 1:
	    itbeg = learn_data1.begin();
	    itend = learn_data1.end();
	    count = 0;
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data1.erase(learn_data1.begin()+count);
	    }
	break;
	case 2: 
	    itbeg = learn_data2.begin();
	    itend = learn_data2.end();
	    count = 0;
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data2.erase(learn_data2.begin()+count);
	    }
	break;
	case 3: 
	    itbeg = learn_data3.begin();
	    itend = learn_data3.end();
	    count = 0;
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data3.erase(learn_data3.begin()+count);
	    }
	break;
	case 4: 
	    itbeg = learn_data4.begin();
	    itend = learn_data4.end();
	    count = 0;
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data4.erase(learn_data4.begin()+count);
	    }	break;
	case 5: 
	    itbeg = learn_data5.begin();
	    itend = learn_data5.end();
	    count = 0;    
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data5.erase(learn_data5.begin()+count);
	    }
	break;
	case 6: 
	    itbeg = learn_data6.begin();
	    itend = learn_data6.end();
	    count = 0;    
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data6.erase(learn_data6.begin()+count);
	    }
	break;
	case 7:
	    itbeg = learn_data7.begin();
	    itend = learn_data7.end();
	    count = 0;
	    for( ; itbeg != itend ;++itbeg,count++)
	    {
		string st = *itbeg;
		decode(st, i,j);
    		if( dataMatrix[i][j] < 7)
    		    learn_data7.erase(learn_data7.begin()+count);
	    }
	break;
    }

}

// har 700 cycle yek bar, normalize hame ye vector ha
void Bhv_StatisticalLearning::setSecurematrix()
{
    int i,j;
    vector<string>::const_iterator it;
    resetDataMatrix();
    
    for(it = learn_data1.begin(); it != learn_data1.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
    }
    
    for(it = learn_data2.begin(); it != learn_data2.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
    }
    
    for(it = learn_data3.begin(); it != learn_data3.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
	if(!exist(normalized,st) && dataMatrix[i][j] > 3)
	    normalized.push_back(st);
    }
    
    for(it = learn_data4.begin(); it != learn_data4.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
	if(!exist(normalized,st) && dataMatrix[i][j] > 3)
	    normalized.push_back(st);
    }

    for(it = learn_data5.begin(); it != learn_data5.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
	if(!exist(normalized,st) && dataMatrix[i][j] > 3)
	    normalized.push_back(st);
    }
    
    for(it = learn_data6.begin(); it != learn_data6.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
	if(!exist(normalized,st) && dataMatrix[i][j] > 3)
	    normalized.push_back(st);
    }
    
    for(it = learn_data7.begin(); it != learn_data7.end(); ++it)
    {
	string st = *it;
	decode(st, i,j );
	dataMatrix[i][j] ++;
	if(!exist(normalized,st) && dataMatrix[i][j] > 3)
	    normalized.push_back(st);
    }
    
}

void Bhv_StatisticalLearning::invertVector(vector<string> & g, int j)
{
    vector<string> copy = g;
    g.clear();
    for(int i=0; i<row; i++)
	for(int k=j-9; k<j;k++)
	{
	    char s[5] = {char(i/10 +48) , char(i%10 +48) , char(k/10 +48) , char(k%10 +48),'\0'};
	    string st = s;
	    if(!exist(copy, st))
		g.push_back(st);
	}
}

void Bhv_StatisticalLearning::makeGraphs()
{
    
    int i,j;
    vector<string>::const_iterator it;
    
    for(it=normalized.begin(); it!=normalized.end(); ++it)
    {
    
    string st = *it;
	decode(st, i, j);
	if(j<8)
	    graph1.push_back(st);
	else
	if(j<17)
	    graph2.push_back(st);
	else
	if(j<26)
	    graph3.push_back(st);
	else
	    graph4.push_back(st);
    }
    invertVector(graph3, 26);
    invertVector(graph4, 35);

}
// main


void Bhv_StatisticalLearning::learn_main(rcsc::PlayerAgent *agent)
{
    cout<<"samira **************************************\n";
    int t = agent-> world().time().cycle();
    //static int counter = 750;

//    if ((!( t - 50) % 5))
	setMatrix(agent);
    
    //print matrix
//    if ((( t - 50) > 100) )
    
    cout<<"time = "<<t<<'\n';
    cout<<"player : "<<agent->world().self().unum();
    for(int i=0 ; i<row ; i++)
    {	
	cout<<'\n';
	for(int j=0 ; j<col ; j++)
	cout<<setw(3)<<dataMatrix[i][j];
    }
    cout<<'\n';
/*    
    if ((!( t - 50) % 100) ) 
	fillCurrentLearnData();

    if ( t == counter && ! findReachableGraph() )
	counter++;
    return;*/
}
#if 0
int  t = agent -> world().time().cycle();
static int counter = 750;

if ((!( t - 50) % 5))
    setMatrix(agent);
    
if ((!( t - 50) % 100) ) 
    fillCurrentLearnData();

if ( t == counter && ! findReachableGraph() )
    counter++;
#endif

//dar cycle 750 
bool Bhv_StatisticalLearning::findReachableGraph()
{
    setSecurematrix();
    makeGraphs();
    return true;
}

// dar main har 100 cycle yek bar seda zade shavad
void Bhv_StatisticalLearning::fillCurrentLearnData()
{
    normalize_learnDataToVector();
    resetDataMatrix ();
    data_counter++;

}


//dar main har 5 cycle seda zade shavad
void Bhv_StatisticalLearning::setMatrix( rcsc::PlayerAgent *agent ) 
{
    static bool firstCall  =  true;
    //int  t = agent->world().time().cycle();
    int i,j;
    if ( firstCall)
    {
	resetDataMatrix ();
	firstCall = false;
    }
    //if ( t <= 50  || t % 5) return;
    const rcsc::PlayerPtrCont & opps = agent->world().opponentsFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
    for ( rcsc::PlayerPtrCont::const_iterator it = opps.begin(); it != opps_end; ++it )
    {
        i = int( ( (*it)->pos().x + 52.5 ) / 3 );
        j = int (( (*it)->pos().y + 34 ) / 4 );
        cout<<"@@@@@@@@ op i = "<<i<<'\n';
        cout<<"@@@@@@@@ op j = "<<j<<'\n';
        if( (i >= 0) && (i < row) && (j >= 0) && (j < col))
    	    dataMatrix[ i ] [ j ] ++ ;
	
	char s[5] = {char(i/10 +48) , char(i%10 +48) , char(j/10 +48) , char(j%10 +48),'\0'};
	string position_code(s);
	cout<<"pos code = "<<s<<'\n';
	
	switch(data_counter)
	{
	    case 1:
		if(! learn_data1.empty() && !exist(learn_data1, position_code ))
		    learn_data1.push_back(position_code);
	    break;
	    case 2:
		if(! learn_data2.empty() && !exist(learn_data2, position_code ))
		    learn_data2.push_back(position_code);
	    break;
	    case 3:
		if(! learn_data3.empty() && !exist(learn_data3, position_code ))
		    learn_data3.push_back(position_code);
	    break;

	    case 4:
		if(! learn_data4.empty() && !exist(learn_data4, position_code ))
		    learn_data4.push_back(position_code);
	    break;
	    case 5:
		if(! learn_data5.empty() && !exist(learn_data5, position_code ))
		    learn_data5.push_back(position_code);
	    break;
	    case 6:
		if(! learn_data6.empty() && !exist(learn_data6, position_code ))
		    learn_data6.push_back(position_code);
	    break;
	    case 7:
		if(! learn_data7.empty() && !exist(learn_data7, position_code ))
		    learn_data7.push_back(position_code);
	    break;
	}
	
    }
    return ;
}
//check offensive_methods
bool Bhv_StatisticalLearning::check_offensive(rcsc::PlayerAgent * agent)
{
    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D opGoalie_pos;
    rcsc::Vector2D target_point;
    rcsc::AngleDeg target_ang;
    static bool isDribble_off = false;
    //bool isPass_off = false;
    static int cycle;
    
    if(my_pos.x <= 36 )
	return false;
    const rcsc::PlayerPtrCont opps = wm.opponentsFromSelf();
    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
    for ( rcsc::PlayerPtrCont::const_iterator itop = opps.begin(); itop != opps_end; ++itop )
	if( (*itop)->unum() == wm.opponentGoalieUnum() )
	    opGoalie_pos = (*itop)->pos();
    
    //shoot
//    if(my_pos.absY() < 7 && my_pos.x > 42)
	//if(rcsc::isShootLineExist(agent ,target_point))
	{
    	    cout<<"\n isShootlineExist\n";
    	    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    	    return true;
	}
    
    if( my_pos.x > 36 )
    {
	
	if( my_pos.y > 6 )
	{
	    target_ang = -70;
	    isDribble_off = true;
	}
	if( my_pos.y < -6  )
	{
	    target_ang = 70;
	    isDribble_off = true;
	}
	else if(my_pos.x < 40 )
	{
	    target_ang = 0;
	    isDribble_off = true;
	}
	
	else if( my_pos.x >= 40 && my_pos.y > 0)	
	{
	    target_ang = -130;
	    isDribble_off = true;

	}
	else if ( my_pos.x >= 40 && my_pos.y > 0)
	{
	    target_ang = +130;
	    isDribble_off = true;

	}
	else isDribble_off = false;
	if(isDribble_off && cycle < 40)
	{
	    cycle++;
	    //isDribble_off = false;
	    int max_dash_step;
	    target_point = target_point.setPolar(7,target_ang) + my_pos;
    	    max_dash_step = wm.self().playerType().cyclesToReachDistance( my_pos.dist( target_point ) );
	    //rcsc::Body_Dribble2008( target_point,1.0,rcsc::ServerParam::i().maxPower(),
                    //std::min( 5, max_dash_step )).execute( agent );
	    cout<<"\n isDribble_off...\n";
    	    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    	    return true;
	}
	cycle = 0;
	/*
	const rcsc::PlayerPtrCont tm = wm.teammatesFromSelf();
	const rcsc::PlayerPtrCont::const_iterator tm_end = tm.end();
	rcsc::Vector2D tm_pos , op_pos;
	for ( rcsc::PlayerPtrCont::const_iterator tm_it = tm.begin(); tm_it != tm_end; ++tm_it )
	{
	    tm_pos = (*tm_it)->pos();
	    if(tm_pos.absY() >= my_pos.absY() && tm_pos.absY() > 5)
		continue;
	    if(tm_pos.dist(my_pos) > 7)
		continue;
	    if(tm_pos.x < 35)
		continue;
	    rcsc::Line2D pass_line(my_pos , tm_pos);
	    
	    const rcsc::PlayerPtrCont opps = wm.opponentsFromSelf();
	    const rcsc::PlayerPtrCont::const_iterator opps_end = opps.end();
	    for ( rcsc::PlayerPtrCont::const_iterator itop = opps.begin(); itop != opps_end; ++itop )
	    {
		op_pos = (*itop)->pos();
		if(pass_line.dist(op_pos) < 0.5)
		{
		    isPass_off = false;
		    break;
		}
	    }
	    if(isPass_off)
	    {
		isPass_off = false;
		rcsc::Body_Pass().execute2(agent , tm_pos , (*tm_it)->unum());
	        cout<<"\n isPass_off...\n";
    		agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    		return true;
	    }
	}*/
	
    }
    return false;    
}

//decesion maker
bool Bhv_StatisticalLearning::decesion_maker(rcsc::PlayerAgent * agent)
{
    switch ( Strategy::get_ball_area( agent->world().ball().pos() ) ) 
    {/*
	case Strategy::BA_CrossBlock:
	    return BA_CrossBlock_execute(agent);
	break;
	
	case Strategy::BA_Stopper:
	    return BA_Stopper_execute(agent);
	break;
	
	case Strategy::BA_Danger:
	    return BA_Danger_execute(agent);
	break;
	
	case Strategy::BA_DribbleBlock:
	    return BA_DribbleBlock_execute(agent);
	break;
	
	case Strategy::BA_DefMidField:
	    return BA_DefMidField_execute(agent);
	break;
	
	case Strategy::BA_DribbleAttack:
	    return BA_DribbleAttack_execute(agent);
	break;
	
	case Strategy::BA_OffMidField:
	    return BA_OffMidField_execute(agent);
	break;
	
	case Strategy::BA_Cross:
	    return BA_Cross_execute(agent);
	break;
	
	case Strategy::BA_ShootChance:
	    return BA_ShootChance_execute(agent);
	break;
	*/
	default:
	    return default_execute(agent);
	break;
    }

}

bool Bhv_StatisticalLearning::default_execute(rcsc::PlayerAgent * agent)
{

    const rcsc::WorldModel & wm = agent->world();
    rcsc::Vector2D my_pos = wm.self().pos();
    rcsc::Vector2D tm_pos(0,0);
    
    if(check_offensive(agent))
	return true;
    //shoot
    rcsc::Vector2D tg_point;
    //if(rcsc::isShootLineExist(agent ,tg_point))
    {
    	cout<<"\n isShootlineExist\n";
    	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    	return true;
    }
/*
    //pass
    rcsc::Vector2D pass_point;
    int re_pass;
    if(rcsc::Body_Pass::check_pass(agent ,pass_point ,  re_pass))
    {
	if( my_pos.x > 25 )
	{
	    cout<<"\n check pass ---> "<<re_pass<<'\n';
	    rcsc::Body_Pass().execute2(agent , pass_point , re_pass);
	    agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	    return true;
	}
    }
*/
    //dribble
    //double x,y;
    //static int dmod = 0;
    rcsc::AngleDeg drib_ang;
    //double first_speed = agent->world().ball().vel().r();
    //if(rcsc::isDribble(agent,drib_ang,x,y,dmod) && dmod ==1 )
    {
	cout<<"\ndefault exe dribble\n";
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
    }
/*    if(rcsc::Body_Pass::emergency_pass(agent))
    {
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
    }
*/
/*    cout<<"\nPass To nearest Teammate!\n";
    
    rcsc::Vector2D target = wm.teammatesFromSelf().front()->pos();
    if( target.x > my_pos.x )
    {
	rcsc::Body_KickOneStep( target, first_speed).execute( agent );
	agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
	return true;
    }
*/
//    return true;
    return false;
}

//end

bool
Bhv_StatisticalLearning::execute( rcsc::PlayerAgent * agent )
{
	
	//if ( ! SamplePlayer::instance().static_learning() )
	if ( ! Strategy::i().static_learning() )
    {
		return false;
    }
    
	return false;
}

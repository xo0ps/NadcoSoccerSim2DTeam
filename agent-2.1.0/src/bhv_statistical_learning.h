
/////////////////////////////////////////////////////////////////////

#ifndef BHV_STATISTICAL_LEARNING_H
#define BHV_STATISTICAL_LEARNING_H

#include <rcsc/player/soccer_action.h>

#include<vector>
#include<string>

class Bhv_StatisticalLearning
    : public rcsc::SoccerBehavior {

public:

	Bhv_StatisticalLearning( )
		{ }
		
    bool execute( rcsc::PlayerAgent * agent );

private:

    std::vector<std::string> graph1,graph2,graph3,graph4;
    void  resetDataMatrix ();
    bool  exist(std::vector<std::string> & a , std::string& value);
    void  decode(std::string st, int &i, int &j);
    void  normalize_learnDataToVector();
    void  setSecurematrix();
    void  invertVector(std::vector<std::string> & g, int j);
    void  makeGraphs();
    bool  findReachableGraph();
    void  fillCurrentLearnData();
    void  setMatrix( rcsc::PlayerAgent *agent );
    void  learn_main(rcsc::PlayerAgent *agent);
    bool decesion_maker(rcsc::PlayerAgent * agent);
    bool default_execute(rcsc::PlayerAgent * agent);
    bool check_offensive(rcsc::PlayerAgent * agent);

};

#endif


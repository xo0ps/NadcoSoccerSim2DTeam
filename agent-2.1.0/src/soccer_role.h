
/////////////////////////////////////////////////////////////////////

#ifndef SOCCER_ROLE_H
#define SOCCER_ROLE_H

#include <rcsc/player/world_model.h>
#include <rcsc/factory.h>
#include <boost/shared_ptr.hpp>
#include <string>

namespace rcsc {
class PlayerAgent;
}

/*!
  \brief abstruct soccer role
*/
class SoccerRole {
public:

    typedef boost::shared_ptr< SoccerRole > Ptr;
    typedef Ptr (*Creator)();
    typedef rcss::Factory< Creator, std::string > Creators;

    static
    Creators & creators();

    static
    Ptr create( const std::string & name );

private:
    // not used
    SoccerRole( const SoccerRole & );
    SoccerRole & operator=( const SoccerRole & );
protected:

    SoccerRole()
      { }

public:
    //! destructor
    virtual
    ~SoccerRole()
      {
          //std::cerr << "delete SoccerRole" << std::endl;
      }

    //! decide action
    virtual
    void execute( rcsc::PlayerAgent * agent ) = 0;
    
    virtual
    bool acceptExecution( const rcsc::WorldModel & world );

};

#endif

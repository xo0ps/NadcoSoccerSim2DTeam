
/////////////////////////////////////////////////////////////////////

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <boost/shared_ptr.hpp>

namespace rcsc {
class PlayerAgent;
}

class Communication {
public:

    typedef boost::shared_ptr< Communication > Ptr;

protected:

    Communication()
      { }

public:

    virtual
    ~Communication()
      { }

    virtual
    bool execute( rcsc::PlayerAgent * agent ) = 0;
};

#endif

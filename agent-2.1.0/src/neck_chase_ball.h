
/////////////////////////////////////////////////////////////////////

#ifndef NECK_CHASE_BALL_H
#define NECK_CHASE_BALL_H

#include <rcsc/player/soccer_action.h>

namespace rcsc_ext {

/*!
  \class Neck_ChaseBall
  \brief turn neck with attention to ball only
*/
class Neck_ChaseBall
    : public rcsc::NeckAction {

public:
    /*!
      \brief constructor
     */
    Neck_ChaseBall();

    /*!
      \brief do turn neck
      \param agent agent pointer to the agent itself
      \return always true
     */
    bool execute( rcsc::PlayerAgent * agent );

    /*!
      \brief create cloned object
      \return create pointer to the cloned object
    */
    rcsc::NeckAction * clone() const;
};

}

#endif

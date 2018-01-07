
/////////////////////////////////////////////////////////////////////

#ifndef BODY_SMARTKICK_H
#define BODY_SMARTKICK_H

#include <rcsc/action/kick_table.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

/*!
  \class Body_SmartKick
  \brief smart kick action
 */
class Body_SmartKick
    : public rcsc::BodyAction {
private:
    //! target point where the ball should move to
    const rcsc::Vector2D M_target_point;
    //! desired ball first speed
    const double M_first_speed;
    //! threshold value for the ball first speed
    const double M_first_speed_thr;
    //! maximum number of kick steps
    const int M_max_step;

    //! result kick sequence holder
    rcsc::KickTable::Sequence M_sequence;

public:
    /*!
      \brief construct with all parameters
      \param target_point target point where the ball should move to
      \param first_speed desired ball first speed
      \param first_speed_thr threshold value for the ball first speed
      \param max_step maximum number of kick steps
    */
    Body_SmartKick( const rcsc::Vector2D & target_point,
                    const double & first_speed,
                    const double & first_speed_thr,
                    const int max_step )
        : M_target_point( target_point )
        , M_first_speed( first_speed )
        , M_first_speed_thr( first_speed_thr )
        , M_max_step( max_step )
      { }

    bool execute( rcsc::PlayerAgent * agent );

    /*!
      \brief get the result kick sequence
      \return kick sequence object
     */
    const
    rcsc::KickTable::Sequence & sequence() const
      {
          return M_sequence;
      }

};


#endif

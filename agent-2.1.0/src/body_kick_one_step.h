
/////////////////////////////////////////////////////////////////////

#ifndef BODY_KICK_ONE_STEP_H
#define BODY_KICK_ONE_STEP_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

/*!
  \class Body_KickOneStep
  \brief only one step kick action. this action will be used for emergency
  situation or setplay.
  NOTE: not consider about collision & opponents
*/
class Body_KickOneStep
    : public rcsc::BodyAction {
private:
    //! target point where ball should readh or pass through
    rcsc::Vector2D M_target_point;
    //! ball first speed when ball is released
    double M_first_speed;

    //! force mode flag
    bool M_force_mode;

    //! result ball position
    rcsc::Vector2D M_ball_result_pos;
    //! result ball velocity
    rcsc::Vector2D M_ball_result_vel;
    //! estimated kick step
    int M_kick_step;
public:
    /*!
      \brief construct with all parameters
      \param target_point global coordinate of target poisition
      \param first_speed ball first speed when ball is released
      \param force_mode enforce to kick out
    */
    Body_KickOneStep( const rcsc::Vector2D & target_point,
                      const double & first_speed,
                      const bool force_mode = false )
        : M_target_point( target_point )
        , M_first_speed( first_speed )
        , M_force_mode( force_mode )
        , M_ball_result_pos( rcsc::Vector2D::INVALIDATED )
        , M_ball_result_vel( rcsc::Vector2D::INVALIDATED )
        , M_kick_step( 0 )
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool execute( rcsc::PlayerAgent * agent );

    /*!
      \brief get the result ball position
      \return ball position after kick
     */
    const
    rcsc::Vector2D & ballResultPos() const
      {
          return M_ball_result_pos;
      }

    /*!
      \brief get the result ball velocity
      \return ball velocity after kick
     */
    const
    rcsc::Vector2D & ballResultVel() const
      {
          return M_ball_result_vel;
      }


    /*!
      \brief calculate possible velocity by one kick.
      NOTE: ball info may be future estimation result.
      \param target_angle kick target global angle
      \param kick_rate current kick rate
      \param ball_vel current ball velocity
    */
    static
    rcsc::Vector2D get_max_possible_vel( const rcsc::AngleDeg & target_angle,
                                   const double & kick_rate,
                                   const rcsc::Vector2D & ball_vel );
};


#endif

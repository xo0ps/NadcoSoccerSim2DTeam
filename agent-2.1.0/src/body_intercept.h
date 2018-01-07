
/////////////////////////////////////////////////////////////////////

#ifndef BODY_INTERCEPT_H
#define BODY_INTERCEPT_H

#include <rcsc/player/soccer_action.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/common/player_type.h>
#include <rcsc/geom/vector_2d.h>

/*!
  \class Body_Intercept2009
  \brief ball chasing action including smart planning.
*/
class Body_Intercept
    : public rcsc::BodyAction {
private:
    //! if true, agent must save recovery parameter
    const bool M_save_recovery;
    //! facing target target point. if specified this, plaeyr try to turn neck to this point.
    const rcsc::Vector2D M_face_point;
public:
    /*!
      \brief construct with all parameters
      \param save_recovery if true, agent must save recovery parameter
      \param face_point desired body facing point
    */
    explicit
    Body_Intercept( const bool save_recovery = true,
                        const rcsc::Vector2D & face_point = rcsc::Vector2D::INVALIDATED )
        : M_save_recovery( save_recovery )
        , M_face_point( face_point )
      { }

    /*!
      \brief execute action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool execute( rcsc::PlayerAgent * agent );


private:

    /*!
      \brief if kickable opponent exists, perform special action
      \param agent pointer to the agent itself
      \return true if action is performed
    */
    bool doKickableOpponentCheck( rcsc::PlayerAgent * agent );

    /*!
      \brief calculate best interception point using cached table
      \param wm const refefence to the WorldModel
      \param table const pointer to the cached table
      \return interception info object
    */
    rcsc::InterceptInfo getBestIntercept( const rcsc::WorldModel & wm,
                                    const rcsc::InterceptTable * table ) const;

    rcsc::InterceptInfo getBestIntercept_Test( const rcsc::WorldModel & wm,
                                         const rcsc::InterceptTable * table ) const;

    /*!
      \brief try to perform ball wait action
      \param agent pointer to the agent itself
      \param target_point intercept target ball point
      \param info interception info that is considered now
      \return true if action is performed
    */
    bool doWaitTurn( rcsc::PlayerAgent * agent,
                     const rcsc::Vector2D & target_point,
                     const rcsc::InterceptInfo & info );

    /*!
      \brief adjutment dash action. if possible try to perform turn action.
      \param agent pointer to agent itself
      \param target_point intercept target ball point
      \param info interception info that is considered now
      \return true if action is performed
    */
    bool doInertiaDash( rcsc::PlayerAgent * agent,
                        const rcsc::Vector2D & target_point,
                        const rcsc::InterceptInfo & info );

};

#endif

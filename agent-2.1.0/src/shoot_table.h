
/////////////////////////////////////////////////////////////////////

#ifndef SHOOT_TABLE_H
#define SHOOT_TABLE_H

#include <rcsc/geom/line_2d.h>
#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>

#include <functional>
#include <vector>

namespace rcsc{
class PlayerAgent;
class PlayerObject;
class WorldModel;
}

/*!
  \class ShootTable2008
  \brief shoot plan search and holder table
 */
class ShootTable {
public:
    /*!
      \struct Shot
      \brief shoot object
     */
    struct Shot {
        rcsc::Vector2D point_; //!< target point on the goal line
        rcsc::Vector2D vel_; //!< first ball velocity
        double speed_; //!< first ball speed
        rcsc::AngleDeg angle_; //!< shoot angle
        bool goalie_never_reach_; //!< true if goalie never reach the ball
        int score_; //!< evaluated value of this shoot

        /*!
          \brief constructor
          \param point shoot target point on the goal line
          \param speed first ball speed
          \param angle shoot angle
         */
        Shot( const rcsc::Vector2D & point,
              const double & speed,
              const rcsc::AngleDeg & angle )
            : point_( point )
            , vel_( rcsc::Vector2D::polar2vector( speed, angle ) )
            , speed_( speed )
            , angle_( angle )
            , goalie_never_reach_( true )
            , score_( 0 )
          { }
    };

    //! type of the Shot container
    typedef std::vector< Shot > ShotCont;

    /*!
      \struct ScoreCmp
      \brief function object to evaluate the shoot object
     */
    struct ScoreCmp
        : public std::binary_function< Shot,
                                       Shot,
                                       bool > {
        /*!
          \brief compare operator
          \param lhs left hand side argument
          \param rhs right hand side argument
         */
        result_type operator()( const first_argument_type & lhs,
                                const second_argument_type & rhs ) const
          {
              return lhs.score_ > rhs.score_;
          }
    };

private:

    //! search count
    int M_total_count;

    //! cached calculated shoot pathes
    ShotCont M_shots;

    // not used
    ShootTable( const ShootTable & );
    const ShootTable & operator=( const ShootTable & );
public:
    /*!
      \brief accessible from global.
     */
    ShootTable()
      { }

    /*!
      \brief calculate the shoot and return the container
      \param agent const pointer to the agent
      \return const reference to the shoot container
     */
    const
    ShotCont & getShots( const rcsc::PlayerAgent * agent )
      {
          search( agent );
          return M_shots;
      }

private:

    /*!
      \brief search shoot route patterns. goal mouth is divided to several
      segment and several ball speed for each segment are calculated.
      \param agent const pointer to the agent itself
     */
    void search( const rcsc::PlayerAgent * agent );

    void calculateShotPoint( const rcsc::WorldModel & wm,
                             const rcsc::Vector2D & shot_point,
                             const rcsc::PlayerObject * goalie );
    bool canScore( const rcsc::WorldModel & wm,
                   const bool one_step_kick,
                   Shot * shot );
    bool maybeGoalieCatch( const rcsc::WorldModel & wm,
                           const rcsc::PlayerObject * goalie,
                           Shot * shot );

    int predictOpponentReachStep( const rcsc::WorldModel & wm,
                                  const rcsc::Vector2D & target_point,
                                  const rcsc::PlayerObject * opponent,
                                  const rcsc::Vector2D & first_ball_pos,
                                  const rcsc::Vector2D & first_ball_vel,
                                  const bool one_step_kick,
                                  const int max_step );

};

#endif

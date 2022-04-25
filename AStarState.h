#ifndef A_STAR_STATE_H
#define A_STAR_STATE_H

#include "AStarSearch.h"

template< class S >
class AStarState
{

 public:

 /**
  * Return the estimated cost to goal from this node (pass reference to goal
  * node).
  * @param goal_node
  * @return
  */
 virtual float goal_distance_estimate( S & goal_node ) = 0;

 /**
  * Return true if this node is the goal.
  * @param goal_node
  * @return
  */
 virtual bool is_goal( S & goal_node ) = 0;

 /**
  * For each successor to this state call the a_star_search's add_successor call
  * to add each one to the current search - return false if you are out of
  * memory and the search will fail.
  * @param a_star_search
  * @param parent_node
  * @return
  */
 virtual bool get_successors( AStarSearch< S > * a_star_search ,
                              S * parent_node ) = 0;

 /**
  * Return the cost moving from this state to the state of successor.
  * @param successor
  * @return
  */
 virtual float get_cost( S & successor ) = 0;

 /**
  * Return true if the provided state is the same as this state.
  * @param rhs
  * @return
  */
 virtual bool is_same_state( S & rhs ) = 0;

};

#endif // A_STAR_STATE_H

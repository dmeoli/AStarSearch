#ifndef A_STAR_STATE_H
#define A_STAR_STATE_H

#include "AStarSearch.h"

template< class S >
class AStarState
{

 public:

 virtual float goalDistanceEstimate( S & nodeGoal ) = 0;

 virtual bool isGoal( S & nodeGoal ) = 0;

 virtual bool getSuccessors( AStarSearch< S > * aStarSearch ,
                             S * parentNode ) = 0;

 virtual float getCost( S & successor ) = 0;

 virtual bool isSameState( S & rhs ) = 0;
};

#endif

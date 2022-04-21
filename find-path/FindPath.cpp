/**
 * A* Search implementation to find a path on a simple grid maze.
 *
 * Usage: ./FindPath
 *
 * @author Donato Meoli
 */

#include "MapSearchNode.h"

int main( int argc , char * argv[] ) {
 AStarSearch< MapSearchNode > aStarSearch;
 MapSearchNode startState( rand() % MapSearchNode::MAP_HEIGHT ,
                            rand() % MapSearchNode::MAP_HEIGHT );
 MapSearchNode goalState( rand() % MapSearchNode::MAP_WIDTH ,
                           rand() % MapSearchNode::MAP_HEIGHT );
 aStarSearch.setStartAndGoalStates( startState , goalState );
 unsigned int searchState;
 unsigned int searchSteps = 0;
 do {
  searchState = aStarSearch.searchStep();
  searchSteps++;
 } while( searchState ==
          AStarSearch< MapSearchNode >::SEARCH_STATE_SEARCHING );
 if( searchState == AStarSearch< MapSearchNode >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  MapSearchNode * mapSearchState = aStarSearch.getSolutionStart();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  mapSearchState->printNodeInfo();
  for( ;; ) {
   mapSearchState = aStarSearch.getSolutionNext();
   if( !mapSearchState )
    break;
   mapSearchState->printNodeInfo();
   steps++;
  }
  cout << "Solution steps: " << steps << endl;
  aStarSearch.freeSolutionNodes();
 } else if( searchState ==
            AStarSearch< MapSearchNode >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( searchState ==
            AStarSearch< MapSearchNode >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << searchSteps << endl;
 return EXIT_SUCCESS;
}


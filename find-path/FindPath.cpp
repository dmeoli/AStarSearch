/**
 * A* Search implementation to find a path on a simple grid maze.
 *
 * Usage: ./FindPath start_x start_y goal_x goal_y
 *
 * Example: ./FindPath 3 6 17 15
 *
 * @author Donato Meoli
 */

#include <random>

#include "MapSearchNode.h"

int main( int argc , char * argv[] ) {
 AStarSearch< MapSearchNode > aStarSearch;
 if( argc == 5 ) {
  MapSearchNode startNode( atoi( argv[ 1 ] ) , atoi( argv[ 2 ] ) );
  MapSearchNode goalNode( atoi( argv[ 3 ] ) , atoi( argv[ 4 ] ) );
  aStarSearch.setStartAndGoalStates( startNode , goalNode );
 } else {
  std::random_device rd;
  std::mt19937 mt( rd() );
  MapSearchNode startNode( mt() % MapSearchNode::MAP_WIDTH ,
                           mt() % MapSearchNode::MAP_HEIGHT );
  MapSearchNode goalNode( mt() % MapSearchNode::MAP_WIDTH ,
                          mt() % MapSearchNode::MAP_HEIGHT );
  aStarSearch.setStartAndGoalStates( startNode , goalNode );
 }
 unsigned int searchState;
 unsigned int searchSteps = 0;
 do {
  searchState = aStarSearch.searchStep();
  searchSteps++;
 } while( searchState ==
          AStarSearch< MapSearchNode >::SEARCH_STATE_SEARCHING );
 if( searchState == AStarSearch< MapSearchNode >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  MapSearchNode * mapSearchNode = aStarSearch.getSolutionStart();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  mapSearchNode->printNodeInfo();
  for( ;; ) {
   mapSearchNode = aStarSearch.getSolutionNext();
   if( !mapSearchNode )
    break;
   mapSearchNode->printNodeInfo();
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


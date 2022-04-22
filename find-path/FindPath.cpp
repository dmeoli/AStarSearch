/**
 * A* Search implementation to find a path on a simple grid maze.
 * Our sample problem defines the world as a 2D array representing a terrain.
 * Each element contains an integer from 0 to 5 which indicates the cost of
 * travel across the terrain. Zero means the least possible difficulty in
 * travelling (think ice rink if you can skate) whilst 5 represents the most
 * difficult. 9 indicates that we cannot pass.
 *
 * Usage: ./FindPath start_col start_row goal_col goal_row
 *
 * Example: ./FindPath 3 6 17 15
 *
 *      00 01 02 03 04 05 06 07 08 09 10 11 12 13 1 15 16 17 18 19
 *
 * 00   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1
 * 01   1  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  1
 * 02   1  *  *  1  1  *  *  *  1  *  1  *  1  *  1  *  *  *  1  1
 * 03   1  *  *  1  1  *  *  *  1  *  1  *  1  *  1  *  *  *  1  1
 * 04   1  *  1  |  ―  ―  *  *  1  *  1  *  1  1  1  1  *  *  1  1
 * 05   1  *  1  |  *  |  ―  1  1  *  1  1  1  1  *  1  1  1  1  1
 * 06   1  *  *  S  *  1  |  ―  ―  ―  ―  *  *  *  *  1  1  1  1  1
 * 07   1  *  *  *  *  *  *  *  *  1  |  1  *  *  *  *  *  *  *  1
 * 08   1  *  1  1  1  1  1  1  1  1  |  *  1  1  1  1  1  1  1  1
 * 09   1  *  1  *  *  *  *  *  *  *  |  ―  *  *  *  *  *  *  *  1
 * 10   1  *  1  1  1  1  *  1  1  *  1  |  ―  ―  ―  ―  ―  ―  1  1
 * 11   1  *  *  *  *  *  1  *  1  *  1  *  *  *  *  *  1  |  ―  1
 * 12   1  *  1  *  1  *  *  *  1  *  1  *  1  *  1  *  *  *  |  1
 * 13   1  *  1  *  1  *  *  *  1  *  1  *  1  *  1  *  *  *  |  1
 * 14   1  *  1  1  1  1  *  *  1  *  1  *  1  1  1  1  *  *  |  1
 * 15   1  *  1  1  *  1  1  1  1  *  1  1  1  1  *  1  1  G  |  1
 * 16   1  *  *  *  *  1  1  1  1  1  1  *  *  *  *  1  1  1  1  1
 * 17   1  1  *  *  *  *  *  *  *  1  1  1  *  *  *  1  *  *  *  *
 * 18   1  *  1  1  1  1  1  1  1  1  1  *  1  1  1  1  1  1  1  1
 * 19   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1
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


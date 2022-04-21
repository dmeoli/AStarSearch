/**
 * A* Search implementation to solve 8 puzzle.
 *
 * Goal:        Easy:        Medium:      Hard:        Worst:
 *
 * 1 2 3        1 3 4        2 8 1        2 8 1        5 6 7
 * 8   4        8 6 2          4 3        4 6 3        4   8
 * 7 6 5        7   5        7 6 5          7 5        3 2 1
 *
 * Usage: ./8Puzzle {134862705|281043765|281463075|567408321|etc.}
 *
 * Example: ./8Puzzle 281463075
 *
 * @author Donato Meoli
 */

#include "PuzzleState.h"

int main( int argc , char * argv[] ) {
 if( argc > 1 ) {
  int i = 0 , c;
  while( ( c = argv[ 1 ][ i ] ) ) {
   if( isdigit( c ) ) {
    int n = ( c - '0' );
    PuzzleState::start[ i ] = static_cast<PuzzleState::TILE>(n);
   }
   i++;
  }
 }
 AStarSearch< PuzzleState > aStarSearch;
 PuzzleState startState( PuzzleState::start );
 PuzzleState goalState( PuzzleState::goal );
 aStarSearch.setStartAndGoalStates( startState , goalState );
 unsigned int searchState;
 unsigned int searchSteps = 0;
 do {
  searchState = aStarSearch.searchStep();
  searchSteps++;
 } while( searchState == AStarSearch< PuzzleState >::SEARCH_STATE_SEARCHING );
 if( searchState == AStarSearch< PuzzleState >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  PuzzleState * puzzleState = aStarSearch.getSolutionStart();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  puzzleState->printNodeInfo();
  cout << endl;
  for( ;; ) {
   puzzleState = aStarSearch.getSolutionNext();
   if( !puzzleState )
    break;
   puzzleState->printNodeInfo();
   cout << endl;
   steps++;
  }
  cout << "Solution steps: " << steps << endl;
  puzzleState = aStarSearch.getSolutionEnd();
  steps = 0;
  puzzleState->printNodeInfo();
  cout << endl;
  for( ;; ) {
   puzzleState = aStarSearch.getSolutionPrev();
   if( !puzzleState )
    break;
   steps++;
  }
  aStarSearch.freeSolutionNodes();
 } else if( searchState == AStarSearch< PuzzleState >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( searchState ==
            AStarSearch< PuzzleState >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << aStarSearch.getStepCount() << endl;
 return EXIT_SUCCESS;
}


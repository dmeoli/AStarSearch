/**
 * A* Search implementation to solve 15-puzzle.
 *
 *  Goal:
 *
 *     1  2  3
 *  4  5  6  7
 *  8  9 10 11
 * 12 13 14 15
 *
 * Usage:   ./15Puzzle {10 2 7 1 14 0 8 4 13 5 15 12 9 11 3 6|etc.}
 *
 * Example: ./15Puzzle 10 2 7 1 14 0 8 4 13 5 15 12 9 11 3 6
 */

#include "FifteenPuzzleState.h"

FifteenPuzzleState::TILE FifteenPuzzleState::start[] =
 {
  TL_10 ,
  TL_2 ,
  TL_7 ,
  TL_1 ,

  TL_14 ,
  TL_SPACE ,
  TL_8 ,
  TL_4 ,

  TL_13 ,
  TL_5 ,
  TL_15 ,
  TL_12 ,

  TL_9 ,
  TL_11 ,
  TL_3 ,
  TL_6
 };

int main( int argc , char * argv[] ) {
 if( argc > 1 ) {
  int c;
  for( int i = 1 ; i <= ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
   c = atoi( argv[ i ] );
   FifteenPuzzleState::start[ i - 1 ] =
    static_cast<FifteenPuzzleState::TILE>(c);
  }
 }
 AStarSearch< FifteenPuzzleState > a_star_search;
 FifteenPuzzleState startState( FifteenPuzzleState::start );
 FifteenPuzzleState goalState( FifteenPuzzleState::goal );
 a_star_search.set_start_goal_states( startState , goalState );
 unsigned int search_state;
 unsigned int search_steps = 0;
 do {
  search_state = a_star_search.search_step();
  search_steps++;
 } while( search_state ==
          AStarSearch< FifteenPuzzleState >::SEARCH_STATE_SEARCHING );
 if( search_state ==
     AStarSearch< FifteenPuzzleState >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  FifteenPuzzleState * puzzle_state = a_star_search.get_solution_start();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  puzzle_state->print_node_info();
  cout << endl;
  for( ;; ) {
   puzzle_state = a_star_search.get_solution_next();
   if( !puzzle_state )
    break;
   puzzle_state->print_node_info();
   cout << endl;
   steps++;
  }
  cout << "Solution steps: " << steps << endl;
  puzzle_state = a_star_search.get_solution_end();
  steps = 0;
  puzzle_state->print_node_info();
  cout << endl;
  for( ;; ) {
   puzzle_state = a_star_search.get_solution_prev();
   if( !puzzle_state )
    break;
   steps++;
  }
  a_star_search.free_solution_nodes();
 } else if( search_state ==
            AStarSearch< FifteenPuzzleState >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( search_state ==
            AStarSearch< FifteenPuzzleState >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << a_star_search.get_step_count() << endl;
 return EXIT_SUCCESS;
}
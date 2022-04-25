/**
 * A* Search implementation to solve 8-puzzle.
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
 */

#include "EightPuzzleState.h"

EightPuzzleState::TILE EightPuzzleState::start[] = {
 TL_1 ,
 TL_3 ,
 TL_4 ,
 TL_8 ,
 TL_SPACE ,
 TL_2 ,
 TL_7 ,
 TL_6 ,
 TL_5
};

int main( int argc , char * argv[] ) {
 if( argc > 1 ) {
  int i = 0 , c;
  while( ( c = argv[ 1 ][ i ] ) ) {
   if( isdigit( c ) ) {
    int n = ( c - '0' );
    EightPuzzleState::start[ i ] = static_cast<EightPuzzleState::TILE>(n);
   }
   i++;
  }
 }
 AStarSearch< EightPuzzleState > a_star_search;
 EightPuzzleState startState( EightPuzzleState::start );
 EightPuzzleState goalState( EightPuzzleState::goal );
 a_star_search.set_start_goal_states( startState , goalState );
 unsigned int search_state;
 unsigned int search_steps = 0;
 do {
  search_state = a_star_search.search_step();
  search_steps++;
 } while( search_state ==
          AStarSearch< EightPuzzleState >::SEARCH_STATE_SEARCHING );
 if( search_state == AStarSearch< EightPuzzleState >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  EightPuzzleState * puzzle_state = a_star_search.get_solution_start();
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
            AStarSearch< EightPuzzleState >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( search_state ==
            AStarSearch< EightPuzzleState >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << a_star_search.get_step_count() << endl;
 return EXIT_SUCCESS;
}


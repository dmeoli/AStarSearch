/*--------------------------------------------------------------------------*/
/*---------------------------- File 8Puzzle.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Solves an instance of the 8-puzzle with A*, towards the "snail" goal
 *
 *     1 2 3
 *     8   4
 *     7 6 5
 *
 * Usage:   8Puzzle [-n] [<start>]
 *
 * where <start> lists the nine tiles row by row, 0 for the blank, e.g.,
 * 134862705 (easy), 281043765 (medium), 281463075 (hard), 567408321
 * (worst); the default is the easy one. The heuristic is the Manhattan
 * distance, which gives an optimal solution; with -n it is the sequence
 * score of Nilsson, which expands fewer nodes but may return a longer
 * solution.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cctype>

#include <cstring>

#include <iostream>

#include "AStar.h"

#include "SequenceScore.h"

#include "SlidingTile.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// prints the solution of a search

template< class S >
static int report( const S & sol )
{
 if( ! sol.solved() ) {
  std::cout << "Search terminated. Did not find goal state!" << std::endl;
  return( 1 );
  }
 std::cout << "Search found goal state..." << std::endl
           << "Displaying solution:" << std::endl;
 for( auto s : sol.path ) {
  std::cout << std::endl;
  EightPuzzle::print( std::cout , s );
  }
 std::cout << std::endl << "Solution steps: " << sol.cost << std::endl
           << "Search steps: " << sol.stats.expanded << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 bool nilsson = false;
 const char * board = "134862705";
 for( int i = 1 ; i < argc ; ++i )
  if( ! std::strcmp( argv[ i ] , "-n" ) )
   nilsson = true;
  else
   board = argv[ i ];

 EightPuzzle::board_type start{};
 int k = 0;
 for( const char * c = board ; *c && ( k < 9 ) ; ++c )
  if( std::isdigit( static_cast< unsigned char >( *c ) ) )
   start[ k++ ] = std::uint8_t( *c - '0' );
 const EightPuzzle::board_type goal = { 1 , 2 , 3 , 8 , 0 , 4 , 7 , 6 , 5 };

 try {
  if( k != 9 )
   throw( std::invalid_argument( "the start needs nine tiles" ) );
  if( nilsson )
   return( report( astar( SequenceScore< 3 , 3 >( start , goal ) ) ) );
  return( report( astar( EightPuzzle( start , goal ) ) ) );
  }
 catch( std::exception & e ) {
  std::cerr << "8Puzzle: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File 8Puzzle.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

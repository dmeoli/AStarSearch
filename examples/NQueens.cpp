/*--------------------------------------------------------------------------*/
/*---------------------------- File NQueens.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The n-queens problem with A*, in the incremental formulation of section
 * 3.2.1 of Russell and Norvig, "Artificial Intelligence: A Modern
 * Approach" (3rd edition, 2010): a queen is added in the leftmost empty
 * column, in a row where no queen already on the board attacks it.
 *
 * Usage:   NQueens [<n>]
 *
 * The default is n = 8, and n is at most 15. Each queen costs 1, and the
 * heuristic, the number of the empty columns, is exact on the states that
 * lead to a solution; A* with the ties broken in favour of the larger g
 * then behaves as a depth-first search, and the counters tell how many
 * dead ends it meets.
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

#include <cstdint>

#include <cstdlib>

#include <iostream>

#include <stdexcept>

#include "AStarSearch/AStar.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS NQueens ------------------------------*/
/*--------------------------------------------------------------------------*/
/// the n-queens problem as a Domain
/** A state packs the number k of queens on the board in the 4 high bits and
 * the row of the queen of column c in the bits 4 c , ... , 4 c + 3. */

class NQueens {

 public:

 using state_type = std::uint64_t;               ///< a packed board
 using cost_type = int;                          ///< the cost type

 explicit NQueens( int n ) : f_n( n ) {
  if( ( n < 1 ) || ( n > 15 ) )
   throw( std::invalid_argument( "NQueens::NQueens: n is 1 to 15" ) );
  }

 [[nodiscard]] static int queens( std::uint64_t s ) noexcept {
  return( int( s >> 60 ) );
  }

 [[nodiscard]] static int row( std::uint64_t s , int c ) noexcept {
  return( int( ( s >> ( 4 * c ) ) & 15 ) );
  }

 [[nodiscard]] std::uint64_t initial_state( void ) const noexcept {
  return( 0 );
  }

 [[nodiscard]] bool is_goal( std::uint64_t s ) const noexcept {
  return( queens( s ) == f_n );
  }

 [[nodiscard]] std::uint64_t hash( std::uint64_t s ) const noexcept {
  return( mix64( s ) );
  }

 [[nodiscard]] int heuristic( std::uint64_t s ) const noexcept {
  return( f_n - queens( s ) );
  }

 template< class F >
 void for_each_successor( std::uint64_t s , int h , F && f ) const {
  const int k = queens( s );
  for( int r = 0 ; r < f_n ; ++r ) {
   bool free = true;
   for( int c = 0 ; free && ( c < k ) ; ++c ) {
    const int rc = row( s , c );
    free = ( rc != r ) && ( std::abs( rc - r ) != k - c );
    }
   if( free )
    f( ( ( s & ~( std::uint64_t( 15 ) << 60 ) ) |
         ( std::uint64_t( r ) << ( 4 * k ) ) ) |
       ( std::uint64_t( k + 1 ) << 60 ) , 1 , h - 1 );
   }
  }

 private:

 int f_n;                                        ///< the size of the board

 };  // end( class( NQueens ) )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 try {
  const int n = argc > 1 ? std::atoi( argv[ 1 ] ) : 8;
  const auto sol = astar( NQueens( n ) );
  if( ! sol.solved() ) {
   std::cout << "no placement of " << n << " queens" << std::endl;
   return( 1 );
   }
  const auto s = sol.path.back();
  for( int r = 0 ; r < n ; ++r ) {
   for( int c = 0 ; c < n ; ++c )
    std::cout << ( NQueens::row( s , c ) == r ? " Q" : " ." );
   std::cout << std::endl;
   }
  std::cout << "expanded:  " << sol.stats.expanded << std::endl
            << "generated: " << sol.stats.generated << std::endl
            << "seconds:   " << sol.stats.seconds << std::endl;
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "NQueens: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File NQueens.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

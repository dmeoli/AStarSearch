/*--------------------------------------------------------------------------*/
/*-------------------------- File VacuumWorld.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vacuum world with A*: the toy problem of section 3.2.1 of Russell and
 * Norvig, "Artificial Intelligence: A Modern Approach" (3rd edition, 2010),
 * on a row of n cells instead of 2.
 *
 * Usage:   VacuumWorld [<n> [<start> [<dirty cells>]]]
 *
 * Example: VacuumWorld 10 4 1011001101
 *
 * The robot starts in the cell <start> (0 by default), and the string of
 * 0s and 1s tells which cells are dirty (all by default); the actions Left,
 * Right and Suck cost 1 each, and the goal is a clean row. The default is
 * the world of the book, 2 cells both dirty with the robot on the left,
 * whose answer is Suck, Right, Suck. The heuristic, the number of dirty
 * cells, is consistent; with at most 32 cells.
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

#include <bit>

#include <cstdint>

#include <cstdlib>

#include <iostream>

#include <stdexcept>

#include <string>

#include "AStarSearch/AStar.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS VacuumWorld ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the vacuum world on a row of cells as a Domain
/** A state packs the dirty cells in the bits 0 , ... , 31 and the cell of
 * the robot in the bits 32 , ... , 37. */

class VacuumWorld {

 public:

 using state_type = std::uint64_t;               ///< the packed state
 using cost_type = int;                          ///< the cost type

 VacuumWorld( int n , int start , std::uint32_t dirty )
  : f_n( n ) , f_start( start ) , f_dirty( dirty ) {
  if( ( n < 1 ) || ( n > 32 ) || ( start < 0 ) || ( start >= n ) )
   throw( std::invalid_argument( "VacuumWorld::VacuumWorld: bad sizes" ) );
  }

 [[nodiscard]] static int robot( std::uint64_t s ) noexcept {
  return( int( s >> 32 ) );
  }

 [[nodiscard]] static std::uint32_t dirt( std::uint64_t s ) noexcept {
  return( std::uint32_t( s ) );
  }

 [[nodiscard]] std::uint64_t initial_state( void ) const noexcept {
  return( pack( f_start , f_dirty ) );
  }

 [[nodiscard]] bool is_goal( std::uint64_t s ) const noexcept {
  return( dirt( s ) == 0 );
  }

 [[nodiscard]] std::uint64_t hash( std::uint64_t s ) const noexcept {
  return( mix64( s ) );
  }

 [[nodiscard]] int heuristic( std::uint64_t s ) const noexcept {
  return( std::popcount( dirt( s ) ) );
  }

 template< class F >
 void for_each_successor( std::uint64_t s , int h , F && f ) const {
  const int r = robot( s );
  const std::uint32_t d = dirt( s );
  if( d & ( std::uint32_t( 1 ) << r ) )                       // Suck
   f( pack( r , d & ~( std::uint32_t( 1 ) << r ) ) , 1 , h - 1 );
  if( r > 0 )                                                 // Left
   f( pack( r - 1 , d ) , 1 , h );
  if( r + 1 < f_n )                                           // Right
   f( pack( r + 1 , d ) , 1 , h );
  }

 private:

 // the state with the robot in the cell r and the dirty cells d
 static std::uint64_t pack( int r , std::uint32_t d ) noexcept {
  return( ( std::uint64_t( r ) << 32 ) | d );
  }

 int f_n;                                        ///< the number of cells
 int f_start;                                    ///< the cell of the robot
 std::uint32_t f_dirty;                          ///< the dirty cells

 };  // end( class( VacuumWorld ) )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 try {
  const int n = argc > 1 ? std::atoi( argv[ 1 ] ) : 2;
  const int start = argc > 2 ? std::atoi( argv[ 2 ] ) : 0;
  std::uint32_t dirty = n >= 32 ? ~std::uint32_t( 0 )
                                : ( std::uint32_t( 1 ) << n ) - 1;
  if( argc > 3 ) {
   const std::string d = argv[ 3 ];
   if( int( d.size() ) != n )
    throw( std::invalid_argument( "one 0 or 1 per cell" ) );
   dirty = 0;
   for( int i = 0 ; i < n ; ++i )
    if( d[ i ] == '1' )
     dirty |= std::uint32_t( 1 ) << i;
   }
  const VacuumWorld w( n , start , dirty );
  const auto sol = astar( w );
  for( std::size_t k = 0 ; k < sol.path.size() ; ++k ) {
   const auto s = sol.path[ k ];
   for( int i = 0 ; i < n ; ++i )
    std::cout << ( VacuumWorld::robot( s ) == i ? '[' : ' ' )
              << ( ( VacuumWorld::dirt( s ) >> i ) & 1 ? '*' : '.' )
              << ( VacuumWorld::robot( s ) == i ? ']' : ' ' );
   std::cout << std::endl;
   }
  std::cout << "actions:  " << sol.cost << std::endl
            << "expanded: " << sol.stats.expanded << std::endl
            << "seconds:  " << sol.stats.seconds << std::endl;
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "VacuumWorld: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File VacuumWorld.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

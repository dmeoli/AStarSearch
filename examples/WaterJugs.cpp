/*--------------------------------------------------------------------------*/
/*--------------------------- File WaterJugs.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The water jugs problem with A*: exercise 3.6(d) of Russell and Norvig,
 * "Artificial Intelligence: A Modern Approach" (3rd edition, 2010), which
 * is the PourProblem of aima-python (github.com/aimacode/aima-python).
 *
 * Usage:   WaterJugs [<goal> <capacity> ...]
 *
 * Example: WaterJugs 4 5 3
 *
 * The jugs are empty at the start, and each move fills a jug from the
 * faucet, empties it on the ground, or pours it into another jug until
 * either the first is empty or the second is full; the goal is a jug
 * holding exactly <goal> units. The default is the exercise of the book:
 * jugs of 12, 8 and 3 gallons, and one gallon to measure. Every move costs
 * 1, and the heuristic is 0 at a goal and 1 elsewhere, which is
 * consistent; with up to 4 jugs of at most 65535 units.
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

#include <algorithm>

#include <cstdint>

#include <cstdlib>

#include <iostream>

#include <stdexcept>

#include <vector>

#include "AStar.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS WaterJugs -----------------------------*/
/*--------------------------------------------------------------------------*/
/// the water jugs problem as a Domain
/** A state packs the level of the jug j in the bits 16 j , ... , 16 j +
 * 15. */

class WaterJugs {

 public:

 using state_type = std::uint64_t;               ///< the packed levels
 using cost_type = int;                          ///< the cost type

 WaterJugs( std::vector< int > capacity , int goal )
  : v_cap( std::move( capacity ) ) , f_goal( goal ) {
  if( v_cap.empty() || ( v_cap.size() > 4 ) )
   throw( std::invalid_argument( "WaterJugs::WaterJugs: 1 to 4 jugs" ) );
  for( int c : v_cap )
   if( ( c <= 0 ) || ( c > 65535 ) )
    throw( std::invalid_argument( "WaterJugs::WaterJugs: bad capacity" ) );
  }

 [[nodiscard]] static int level( std::uint64_t s , std::size_t j ) noexcept {
  return( int( ( s >> ( 16 * j ) ) & 0xFFFF ) );
  }

 [[nodiscard]] std::size_t jugs( void ) const noexcept {
  return( v_cap.size() );
  }

 [[nodiscard]] std::uint64_t initial_state( void ) const noexcept {
  return( 0 );
  }

 [[nodiscard]] bool is_goal( std::uint64_t s ) const noexcept {
  for( std::size_t j = 0 ; j < v_cap.size() ; ++j )
   if( level( s , j ) == f_goal )
    return( true );
  return( false );
  }

 [[nodiscard]] std::uint64_t hash( std::uint64_t s ) const noexcept {
  return( mix64( s ) );
  }

 [[nodiscard]] int heuristic( std::uint64_t s ) const noexcept {
  return( is_goal( s ) ? 0 : 1 );
  }

 template< class F >
 void for_each_successor( std::uint64_t s , int , F && f ) const {
  const auto with = [ & ]( std::uint64_t t , std::size_t j , int l ) {
   return( ( t & ~( std::uint64_t( 0xFFFF ) << ( 16 * j ) ) ) |
           ( std::uint64_t( l ) << ( 16 * j ) ) );
   };
  const auto move = [ & ]( std::uint64_t t ) {
   if( t != s )
    f( t , 1 , heuristic( t ) );
   };
  for( std::size_t i = 0 ; i < v_cap.size() ; ++i ) {
   move( with( s , i , v_cap[ i ] ) );                       // fill i
   move( with( s , i , 0 ) );                                // empty i
   for( std::size_t j = 0 ; j < v_cap.size() ; ++j )         // i into j
    if( i != j ) {
     const int q = std::min( level( s , i ) , v_cap[ j ] - level( s , j ) );
     move( with( with( s , i , level( s , i ) - q ) , j ,
                 level( s , j ) + q ) );
     }
   }
  }

 private:

 std::vector< int > v_cap;                       ///< the capacities
 int f_goal;                                     ///< the level to reach

 };  // end( class( WaterJugs ) )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 try {
  int goal = 1;
  std::vector< int > cap = { 12 , 8 , 3 };
  if( argc > 2 ) {
   goal = std::atoi( argv[ 1 ] );
   cap.clear();
   for( int i = 2 ; i < argc ; ++i )
    cap.push_back( std::atoi( argv[ i ] ) );
   }
  const WaterJugs d( cap , goal );
  const auto sol = astar( d );
  if( ! sol.solved() ) {
   std::cout << "the jugs cannot measure " << goal << std::endl;
   return( 1 );
   }
  for( auto s : sol.path ) {
   for( std::size_t j = 0 ; j < d.jugs() ; ++j )
    std::cout << ' ' << WaterJugs::level( s , j ) << '/' << cap[ j ];
   std::cout << std::endl;
   }
  std::cout << "moves:    " << sol.cost << std::endl
            << "expanded: " << sol.stats.expanded << std::endl;
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "WaterJugs: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File WaterJugs.cpp -------------------------*/
/*--------------------------------------------------------------------------*/

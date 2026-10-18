/*--------------------------------------------------------------------------*/
/*--------------------- File MissionariesCannibals.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The missionaries and cannibals problem with A*: exercise 3.9 of Russell
 * and Norvig, "Artificial Intelligence: A Modern Approach" (3rd edition,
 * 2010), with m missionaries, c cannibals and a boat of b seats.
 *
 * Usage:   MissionariesCannibals [<m> <c> <b>]
 *
 * Example: MissionariesCannibals 4 4 2
 *
 * Everybody is on the left bank at the start, with the boat; a crossing
 * carries from 1 to b people, and on neither bank may the missionaries be
 * outnumbered by the cannibals, if any missionary is there. The default is
 * the exercise of the book, with 3 missionaries, 3 cannibals and 2 seats,
 * whose answer is 11 crossings; with 4, 4 and 2 there is none. Every
 * crossing costs 1, and the heuristic is the least number of crossings that
 * would carry the people left over the river with the boat always full,
 * which is admissible.
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

#include "PBeFS/astar/AStar.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace PBeFS_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS MissionariesCannibals -----------------------*/
/*--------------------------------------------------------------------------*/
/// the missionaries and cannibals problem as a Domain
/** A state packs the missionaries on the left bank in the bits 0 , ... ,
 * 15, the cannibals there in the bits 16 , ... , 31, and the bank of the
 * boat in the bit 32 (0 = left). */

class MissionariesCannibals {

 public:

 using state_type = std::uint64_t;               ///< the packed state
 using cost_type = int;                          ///< the cost type

 MissionariesCannibals( int m , int c , int b )
  : f_m( m ) , f_c( c ) , f_b( b ) {
  if( ( m < 0 ) || ( c < 0 ) || ( m > 65535 ) || ( c > 65535 ) ||
      ( b < 1 ) )
   throw( std::invalid_argument( "MissionariesCannibals::"
                                 "MissionariesCannibals: bad sizes" ) );
  }

 [[nodiscard]] static int missionaries( std::uint64_t s ) noexcept {
  return( int( s & 0xFFFF ) );
  }

 [[nodiscard]] static int cannibals( std::uint64_t s ) noexcept {
  return( int( ( s >> 16 ) & 0xFFFF ) );
  }

 [[nodiscard]] static bool boat_left( std::uint64_t s ) noexcept {
  return( ( ( s >> 32 ) & 1 ) == 0 );
  }

 [[nodiscard]] std::uint64_t initial_state( void ) const noexcept {
  return( pack( f_m , f_c , true ) );
  }

 [[nodiscard]] bool is_goal( std::uint64_t s ) const noexcept {
  return( ( missionaries( s ) == 0 ) && ( cannibals( s ) == 0 ) );
  }

 [[nodiscard]] std::uint64_t hash( std::uint64_t s ) const noexcept {
  return( mix64( s ) );
  }

 // with n people on the left bank, a full boat carries b of them over and
 // brings 1 back: at least 2 ceil( ( n - b ) / ( b - 1 ) ) + 1 crossings
 // if n > b, 1 if 0 < n <= b, one more if the boat is on the right bank
 [[nodiscard]] int heuristic( std::uint64_t s ) const noexcept {
  const int n = missionaries( s ) + cannibals( s );
  if( n == 0 )
   return( 0 );
  int h = 1;
  if( n > f_b )
   h = f_b > 1 ? 2 * ( ( n - f_b + ( f_b - 1 ) - 1 ) / ( f_b - 1 ) ) + 1
               : 2 * n - 1;
  return( boat_left( s ) ? h : h + 1 );
  }

 template< class F >
 void for_each_successor( std::uint64_t s , int , F && f ) const {
  const bool left = boat_left( s );
  const int m = missionaries( s ) , c = cannibals( s );
  const int am = left ? m : f_m - m , ac = left ? c : f_c - c;  // aboard
  for( int dm = 0 ; dm <= am ; ++dm )
   for( int dc = 0 ; dc <= ac ; ++dc ) {
    if( ( dm + dc == 0 ) || ( dm + dc > f_b ) )
     continue;
    const int nm = left ? m - dm : m + dm , nc = left ? c - dc : c + dc;
    if( safe( nm , nc ) && safe( f_m - nm , f_c - nc ) ) {
     const std::uint64_t t = pack( nm , nc , ! left );
     f( t , 1 , heuristic( t ) );
     }
    }
  }

 private:

 // the state with m missionaries and c cannibals on the left bank
 static std::uint64_t pack( int m , int c , bool left ) noexcept {
  return( std::uint64_t( m ) | ( std::uint64_t( c ) << 16 ) |
          ( std::uint64_t( left ? 0 : 1 ) << 32 ) );
  }

 // true if m missionaries and c cannibals on a bank are safe
 static bool safe( int m , int c ) noexcept {
  return( ( m == 0 ) || ( m >= c ) );
  }

 int f_m;                                        ///< the missionaries
 int f_c;                                        ///< the cannibals
 int f_b;                                        ///< the seats of the boat

 };  // end( class( MissionariesCannibals ) )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 try {
  int m = 3 , c = 3 , b = 2;
  if( argc > 3 ) {
   m = std::atoi( argv[ 1 ] );
   c = std::atoi( argv[ 2 ] );
   b = std::atoi( argv[ 3 ] );
   }
  const MissionariesCannibals d( m , c , b );
  const auto sol = astar( d );
  if( ! sol.solved() ) {
   std::cout << "no crossing of " << m << " missionaries and " << c
             << " cannibals with " << b << " seats" << std::endl;
   return( 1 );
   }
  for( auto s : sol.path )
   std::cout << ' ' << MissionariesCannibals::missionaries( s ) << 'M'
             << MissionariesCannibals::cannibals( s ) << 'C'
             << ( MissionariesCannibals::boat_left( s ) ? " <boat" : "" )
             << "  |  " << m - MissionariesCannibals::missionaries( s )
             << 'M' << c - MissionariesCannibals::cannibals( s ) << 'C'
             << ( MissionariesCannibals::boat_left( s ) ? "" : " boat>" )
             << std::endl;
  std::cout << "crossings: " << sol.cost << std::endl
            << "expanded:  " << sol.stats.expanded << std::endl
            << "seconds:   " << sol.stats.seconds << std::endl;
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "MissionariesCannibals: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File MissionariesCannibals.cpp -------------------*/
/*--------------------------------------------------------------------------*/

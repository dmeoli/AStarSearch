/*--------------------------------------------------------------------------*/
/*----------------------- File TravelingSalesman.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The traveling salesperson problem with A* and the minimum spanning tree
 * heuristic: exercise 3.30 of Russell and Norvig, "Artificial Intelligence:
 * A Modern Approach" (3rd edition, 2010).
 *
 * Usage:   TravelingSalesman [<cities> [<seed>]]
 *
 * The cities (default 10, at most 16) are random points of the unit square
 * drawn from the seed (default 1), and the tour starts and ends at the
 * first one. A state is the current city and the set of the visited ones;
 * the heuristic is the cost of a minimum spanning tree (by the algorithm of
 * Prim) of the unvisited cities together with the current and the first
 * one, which is admissible because the rest of the tour is a path through
 * all of them. The optimal tour is also computed by the dynamic program of
 * Held and Karp (1962), and the program fails if the two costs differ.
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

#include <cmath>

#include <cstdint>

#include <cstdlib>

#include <iostream>

#include <limits>

#include <random>

#include <stdexcept>

#include <vector>

#include "AStar.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS TravelingSalesman -------------------------*/
/*--------------------------------------------------------------------------*/
/// the traveling salesperson problem as a Domain
/** A state packs the set of the visited cities in the bits 0 , ... , 15,
 * the current city in the bits 16 , ... , 19 and, in the bit 20, whether
 * the tour has gone back to the first city. */

class TravelingSalesman {

 public:

 using state_type = std::uint32_t;               ///< a packed state
 using cost_type = double;                       ///< the cost type

 explicit TravelingSalesman( std::vector< std::vector< double > > dist )
  : v_d( std::move( dist ) ) , f_n( int( v_d.size() ) ) {
  if( ( f_n < 2 ) || ( f_n > 16 ) )
   throw( std::invalid_argument(
                   "TravelingSalesman::TravelingSalesman: 2 to 16 cities" ) );
  }

 [[nodiscard]] static int city( std::uint32_t s ) noexcept {
  return( int( ( s >> 16 ) & 15 ) );
  }

 [[nodiscard]] std::uint32_t initial_state( void ) const noexcept {
  return( 1 );
  }

 [[nodiscard]] bool is_goal( std::uint32_t s ) const noexcept {
  return( ( s >> 20 ) & 1 );
  }

 [[nodiscard]] std::uint64_t hash( std::uint32_t s ) const noexcept {
  return( mix64( s ) );
  }

 /// the cost of a minimum spanning tree of the cities still to be touched
 [[nodiscard]] double heuristic( std::uint32_t s ) const {
  if( is_goal( s ) )
   return( 0 );
  std::vector< int > u;
  for( int c = 0 ; c < f_n ; ++c )
   if( ! ( s & ( 1u << c ) ) || ( c == 0 ) || ( c == city( s ) ) )
    u.push_back( c );
  std::vector< double > key( u.size() ,
                             std::numeric_limits< double >::infinity() );
  std::vector< bool > in( u.size() , false );
  key[ 0 ] = 0;
  double tot = 0;
  for( std::size_t it = 0 ; it < u.size() ; ++it ) {
   std::size_t b = 0;
   double best = std::numeric_limits< double >::infinity();
   for( std::size_t k = 0 ; k < u.size() ; ++k )
    if( ! in[ k ] && ( key[ k ] < best ) )
     best = key[ b = k ];
   in[ b ] = true;
   tot += best;
   for( std::size_t k = 0 ; k < u.size() ; ++k )
    if( ! in[ k ] )
     key[ k ] = std::min( key[ k ] , v_d[ u[ b ] ][ u[ k ] ] );
   }
  return( tot );
  }

 template< class F >
 void for_each_successor( std::uint32_t s , double , F && f ) const {
  const int c = city( s );
  const std::uint32_t all = ( 1u << f_n ) - 1;
  if( ( s & all ) == all ) {                      // back to the first city
   const std::uint32_t t = ( s & all ) | ( 1u << 20 );
   f( t , v_d[ c ][ 0 ] , 0.0 );
   return;
   }
  for( int k = 1 ; k < f_n ; ++k )
   if( ! ( s & ( 1u << k ) ) ) {
    const std::uint32_t t = ( ( s & all ) | ( 1u << k ) ) |
                            ( std::uint32_t( k ) << 16 );
    f( t , v_d[ c ][ k ] , heuristic( t ) );
    }
  }

 private:

 std::vector< std::vector< double > > v_d;      ///< the distances
 int f_n;                                       ///< the number of cities

 };  // end( class( TravelingSalesman ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// the cost of an optimal tour by the dynamic program of Held and Karp

static double held_karp( const std::vector< std::vector< double > > & d )
{
 const int n = int( d.size() );
 const double inf = std::numeric_limits< double >::infinity();
 std::vector< std::vector< double > > c( std::size_t( 1 ) << n ,
                                         std::vector< double >( n , inf ) );
 c[ 1 ][ 0 ] = 0;
 for( std::uint32_t s = 1 ; s < ( 1u << n ) ; s += 2 )
  for( int j = 0 ; j < n ; ++j )
   if( ( s & ( 1u << j ) ) && ( c[ s ][ j ] < inf ) )
    for( int k = 1 ; k < n ; ++k )
     if( ! ( s & ( 1u << k ) ) ) {
      double & t = c[ s | ( 1u << k ) ][ k ];
      t = std::min( t , c[ s ][ j ] + d[ j ][ k ] );
      }
 double best = inf;
 for( int j = 1 ; j < n ; ++j )
  best = std::min( best , c[ ( 1u << n ) - 1 ][ j ] + d[ j ][ 0 ] );
 return( n == 1 ? 0 : best );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 try {
  const int n = argc > 1 ? std::atoi( argv[ 1 ] ) : 10;
  const unsigned long seed = argc > 2 ? std::strtoul( argv[ 2 ] , nullptr ,
                                                      10 ) : 1;
  if( ( n < 2 ) || ( n > 16 ) )
   throw( std::invalid_argument( "2 to 16 cities" ) );
  std::mt19937_64 gen( seed );
  std::vector< double > x( n ) , y( n );
  for( int c = 0 ; c < n ; ++c ) {
   x[ c ] = random_unit( gen );
   y[ c ] = random_unit( gen );
   }
  std::vector< std::vector< double > > d( n , std::vector< double >( n ) );
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b )
    d[ a ][ b ] = std::hypot( x[ a ] - x[ b ] , y[ a ] - y[ b ] );

  const auto sol = astar( TravelingSalesman( d ) );
  if( ! sol.solved() )
   throw( std::logic_error( "no tour found" ) );
  std::cout << "tour:";
  for( auto s : sol.path )
   std::cout << ' ' << TravelingSalesman::city( s );
  const double opt = held_karp( d );
  std::cout << std::endl << "cost:       " << sol.cost << std::endl
            << "Held-Karp:  " << opt << std::endl
            << "expanded:   " << sol.stats.expanded << std::endl;
  if( std::abs( sol.cost - opt ) > 1e-9 * ( 1 + opt ) ) {
   std::cerr << "TravelingSalesman: the tour is not optimal" << std::endl;
   return( 1 );
   }
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "TravelingSalesman: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File TravelingSalesman.cpp ---------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*----------------------------- File test.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tester of AStarSearch: each case, selected by the first argument, solves
 * instances of known optimal cost with one family of searches and checks
 * both the cost and the path, which must start at the initial state, end at
 * a goal, be made of edges of the Domain and cost what the Solution says.
 *
 * The references do not come from the library: a breadth-first search of
 * the whole state space of the 8-puzzle, the optimal costs of the 100
 * instances of Korf (1985), Dijkstra's algorithm on the grids, and the
 * distances on the map of Romania of Russell and Norvig.
 *
 *     test astar | grid | graph | limits | reopen
 *     test hda-thread | hda-openmp | hda-fastflow | farm-fastflow
 *     test spa-thread | spa-openmp
 *     test pbnf-thread | pbnf-openmp
 *     test pase-thread
 *     test bb | bb-shared-thread | bb-dist-thread
 *     test bb-shared-openmp | bb-dist-openmp
 *     test bb-shared-fastflow | bb-dist-fastflow | bb-farm-fastflow
 *     test mpi | bb-mpi             (under mpiexec)
 *     test registry <tests.txt>
 *
 * The cases are listed in test/tests.txt, which CMake and the makefile
 * read; the case registry checks that the list and the cases of the tester
 * are the same.
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

#include <cstring>

#include <fstream>

#include <iostream>

#include <queue>

#include <random>

#include <set>

#include <sstream>

#include <string>

#include <unordered_map>

#include <vector>

#include "AStarSearchConfig.h"

#include "AStarSearch/AStar.h"

#include "AStarSearch/BranchAndBound.h"

#include "AStarSearch/ParallelBranchAndBound.h"

#include "AStarSearch/Distribution.h"

#include "AStarSearch/ExplicitGraph.h"

#include "AStarSearch/Grid.h"

#include "AStarSearch/HDAStar.h"

#include "AStarSearch/SPAStar.h"

#include "AStarSearch/PASE.h"

#include "AStarSearch/SafePBNF.h"

#include "AStarSearch/SlowDomain.h"

#include "AStarSearch/SlidingTile.h"

#ifdef ASTAR_HAS_OPENMP
 #include "AStarSearch/OpenMPRuntime.h"
#endif

#ifdef ASTAR_HAS_FASTFLOW
 #include "AStarSearch/FarmAStar.h"
 #include "AStarSearch/FarmBranchAndBound.h"
 #include "AStarSearch/FastFlowRuntime.h"
#endif

#ifdef ASTAR_HAS_MPI
 #include "AStarSearch/HDAStarMPI.h"
 #include "AStarSearch/MPIBranchAndBound.h"
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------------- GLOBAL VARIABLES ----------------------------*/
/*--------------------------------------------------------------------------*/

static int failures = 0;              // the checks failed so far
static bool quiet = false;            // print only the failures

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// records the outcome of a check

static void check( bool ok , const std::string & what )
{
 if( ! ok ) {
  ++failures;
  std::cout << "FAILED: " << what << std::endl;
  }
 else
  if( ! quiet )
   std::cout << "ok: " << what << std::endl;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// true if the costs are equal, up to a relative 1e-9 for real costs

template< class Cost >
static bool same_cost( Cost a , Cost b )
{
 if constexpr( std::is_floating_point_v< Cost > )
  return( std::abs( a - b ) <= 1e-9 * std::max( Cost( 1 ) , std::abs( b ) ) );
 else
  return( a == b );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// checks that sol is solved with cost expected and a valid path

template< class D , class S >
static void check_solution( const D & d , const S & sol ,
                            cost_t< D > expected , const std::string & what )
{
 bool ok = sol.solved() && same_cost( sol.cost , expected ) &&
           ! sol.path.empty() && ( sol.path.front() == d.initial_state() ) &&
           d.is_goal( sol.path.back() );
 cost_t< D > sum = 0;
 for( std::size_t i = 1 ; ok && ( i < sol.path.size() ) ; ++i ) {
  bool edge = false;
  cost_t< D > best = infinite_cost< cost_t< D > >();
  d.for_each_successor( sol.path[ i - 1 ] ,
                        d.heuristic( sol.path[ i - 1 ] ) ,
                        [ & ]( const state_t< D > & t , cost_t< D > c ,
                               cost_t< D > ) {
   if( ( t == sol.path[ i ] ) && ( c < best ) ) {
    edge = true;
    best = c;
    }
   } );
  ok = edge;
  sum += best;
  }
 ok = ok && same_cost( sum , sol.cost );
 std::ostringstream os;
 os << what << " (cost " << sol.cost << ", expected " << expected
    << ", expanded " << sol.stats.expanded << ")";
 check( ok , os.str() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Blind -------------------------------*/
/*--------------------------------------------------------------------------*/
// a Domain with the heuristic of D replaced by 0: A* becomes Dijkstra

template< class D >
struct Blind {

 using state_type = state_t< D >;
 using cost_type = cost_t< D >;

 const D & d;

 state_type initial_state( void ) const { return( d.initial_state() ); }
 bool is_goal( const state_type & s ) const { return( d.is_goal( s ) ); }
 std::uint64_t hash( const state_type & s ) const { return( d.hash( s ) ); }
 cost_type heuristic( const state_type & ) const { return( 0 ); }

 template< class F >
 void for_each_successor( const state_type & s , cost_type , F && f ) const {
  d.for_each_successor( s , d.heuristic( s ) ,
                        [ & ]( const state_type & t , cost_type c ,
                               cost_type ) { f( t , c , cost_type( 0 ) ); } );
  }

 };

/*--------------------------------------------------------------------------*/
/*------------------------------- INSTANCES --------------------------------*/
/*--------------------------------------------------------------------------*/
// the instances of Korf (1985) with their optimal costs

struct KorfInstance {
 int id;
 FifteenPuzzle::board_type board;
 int cost;
 };

static std::vector< KorfInstance > korf( const std::string & dir )
{
 std::ifstream in( dir + "/15-puzzle/korf100.txt" );
 std::vector< KorfInstance > v;
 std::string line;
 while( std::getline( in , line ) ) {
  if( line.empty() || ( line[ 0 ] == '#' ) )
   continue;
  std::istringstream is( line );
  KorfInstance k;
  k.id = int( v.size() ) + 1;
  for( auto & t : k.board ) {
   int x;
   is >> x;
   t = std::uint8_t( x );
   }
  is >> k.cost;
  v.push_back( k );
  }
 if( v.size() != 100 )
  throw( std::runtime_error( "cannot read korf100.txt from " + dir ) );
 return( v );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the easiest instances of Korf, by number (cost 41 to 45)

static const std::vector< int > easy_korf = { 55 , 16 , 42 , 79 , 71 , 12 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the map of Romania of Russell and Norvig, from a city to Bucharest

enum City { Arad , Bucharest , Craiova , Drobeta , Eforie , Fagaras ,
            Giurgiu , Hirsova , Iasi , Lugoj , Mehadia , Neamt , Oradea ,
            Pitesti , RimnicuVilcea , Sibiu , Timisoara , Urziceni , Vaslui ,
            Zerind , n_cities };

static ExplicitGraph< int > romania( City from )
{
 ExplicitGraph< int > g( n_cities , from , Bucharest );
 const int e[][ 3 ] = {
  { Arad , Sibiu , 140 } , { Arad , Zerind , 75 } ,
  { Arad , Timisoara , 118 } ,
  { Bucharest , Giurgiu , 90 } , { Bucharest , Urziceni , 85 } ,
  { Bucharest , Fagaras , 211 } , { Bucharest , Pitesti , 101 } ,
  { Craiova , Drobeta , 120 } , { Craiova , RimnicuVilcea , 146 } ,
  { Craiova , Pitesti , 138 } , { Drobeta , Mehadia , 75 } ,
  { Eforie , Hirsova , 86 } , { Fagaras , Sibiu , 99 } ,
  { Hirsova , Urziceni , 98 } , { Iasi , Vaslui , 92 } ,
  { Iasi , Neamt , 87 } ,
  { Lugoj , Timisoara , 111 } , { Lugoj , Mehadia , 70 } ,
  { Oradea , Zerind , 71 } , { Oradea , Sibiu , 151 } ,
  { Pitesti , RimnicuVilcea , 97 } , { RimnicuVilcea , Sibiu , 80 } ,
  { Urziceni , Vaslui , 142 } };
 for( const auto & x : e )
  g.add_edge( std::uint32_t( x[ 0 ] ) , std::uint32_t( x[ 1 ] ) , x[ 2 ] );
 // the straight-line distances to Bucharest
 const int sld[ n_cities ] = { 366 , 0 , 160 , 242 , 161 , 176 , 77 , 151 ,
                               226 , 244 , 241 , 234 , 380 , 100 , 193 , 253 ,
                               329 , 80 , 199 , 374 };
 for( int c = 0 ; c < n_cities ; ++c )
  g.set_heuristic( std::uint32_t( c ) , sld[ c ] );
 return( g );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the distances to Bucharest, by Dijkstra on the map

static int romania_distance( City from )
{
 auto g = romania( from );
 return( astar( Blind< ExplicitGraph< int > >{ g } ).cost );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the distance from the goal of every state of the 8-puzzle, by a
// breadth-first search from the goal (the moves are reversible)

static std::unordered_map< std::uint64_t , int > eight_distances(
 const EightPuzzle & d )
{
 std::unordered_map< std::uint64_t , int > dist;
 std::uint64_t goal = EightPuzzle::pack( EightPuzzle::korf_goal() );
 std::queue< std::uint64_t > q;
 dist[ goal ] = 0;
 q.push( goal );
 while( ! q.empty() ) {
  const auto s = q.front();
  q.pop();
  d.for_each_successor( s , 0 , [ & ]( std::uint64_t t , int , int ) {
   if( dist.emplace( t , dist[ s ] + 1 ).second )
    q.push( t );
   } );
  }
 return( dist );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- CASES ----------------------------------*/
/*--------------------------------------------------------------------------*/
// sequential A*, both OPEN lists, on the 8-puzzle and the 15-puzzle

static void test_astar( const std::string & data )
{
 const EightPuzzle any( EightPuzzle::korf_goal() );
 const auto dist = eight_distances( any );
 check( dist.size() == 181440 , "the 8-puzzle has 9! / 2 states" );
 std::size_t k = 0;
 for( const auto & [ s , dst ] : dist ) {
  if( ( k++ % 907 ) != 0 )                     // 200 states, spread out
   continue;
  const EightPuzzle d( EightPuzzle::unpack( s ) );
  const auto sb = astar( d , OpenKind::buckets );
  const auto sh = astar( d , OpenKind::heap );
  if( ! ( sb.solved() && ( sb.cost == dst ) && sh.solved() &&
          ( sh.cost == dst ) ) ) {
   check_solution( d , sb , dst , "8-puzzle, buckets" );
   check_solution( d , sh , dst , "8-puzzle, heap" );
   }
  }
 check( failures == 0 , "A* on 200 states of the 8-puzzle" );

 const auto inst = korf( data );
 for( int id : easy_korf ) {
  const auto & ki = inst[ id - 1 ];
  const FifteenPuzzle d( ki.board );
  check_solution( d , astar( d , OpenKind::buckets ) , ki.cost ,
                  "A* buckets, Korf #" + std::to_string( id ) );
  check_solution( d , astar( d , OpenKind::heap ) , ki.cost ,
                  "A* heap, Korf #" + std::to_string( id ) );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// A* on random grids against Dijkstra, with 4 and 8 moves

static void test_grid( void )
{
 for( std::uint64_t seed = 1 ; seed <= 6 ; ++seed ) {
  const auto m = GridMap::random( 300 , 200 , 0.25 , seed );
  const Grid< 4 > g4( m , 0 , 199 , 299 , 199 );
  const auto ref4 = astar( Blind< Grid< 4 > >{ g4 } );
  if( ref4.solved() )
   check_solution( g4 , astar( g4 ) , ref4.cost ,
                   "grid 4, seed " + std::to_string( seed ) );
  else
   check( ! astar( g4 ).solved() , "grid 4 unsolvable, seed " +
                                    std::to_string( seed ) );
  const Grid< 8 > g8( m , 0 , 199 , 299 , 199 );
  const auto ref8 = astar( Blind< Grid< 8 > >{ g8 } );
  if( ref8.solved() )
   check_solution( g8 , astar( g8 ) , ref8.cost ,
                   "grid 8, seed " + std::to_string( seed ) );
  }

 // the weighted map of the old FindPath example: costs 1 to 5, 9 = wall
 GridMap w( 20 , 20 );
 for( int y = 0 ; y < 20 ; ++y )
  for( int x = 0 ; x < 20 ; ++x )
   w.set_cost( x , y , 1 + ( 7 * x + 3 * y ) % 5 );
 for( int y = 2 ; y < 18 ; ++y )
  w.set_cost( 10 , y , 0 );
 const Grid< 4 > gw( w , 3 , 6 , 17 , 15 );
 check_solution( gw , astar( gw ) , astar( Blind< Grid< 4 > >{ gw } ).cost ,
                 "weighted grid 4" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// A* on the map of Romania

static void test_graph( void )
{
 auto g = romania( Arad );
 const auto s = astar( g );
 check_solution( g , s , 418 , "Romania, Arad to Bucharest" );
 check( ( s.path.size() == 5 ) && ( s.path[ 1 ] == Sibiu ) &&
        ( s.path[ 2 ] == RimnicuVilcea ) && ( s.path[ 3 ] == Pitesti ) ,
        "Romania, through Sibiu, Rimnicu Vilcea and Pitesti" );
 for( int c = 0 ; c < n_cities ; ++c ) {
  auto gc = romania( City( c ) );
  check_solution( gc , astar( gc ) , romania_distance( City( c ) ) ,
                  "Romania, city " + std::to_string( c ) );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the node limit and the unsolvable instances

static void test_limits( const std::string & data )
{
 const auto inst = korf( data );
 const FifteenPuzzle d( inst[ 0 ].board );
 SearchOptions o;
 o.node_limit = 10000;
 const auto s = astar( d , OpenKind::automatic , o );
 check( ( s.status == Status::node_limit ) && ( s.stats.stored >= 10000 ) &&
        s.path.empty() , "A* stops at the node limit" );

 GridMap m( 50 , 50 );
 for( int y = 0 ; y < 50 ; ++y )
  m.set_cost( 25 , y , 0 );
 const Grid< 4 > g( m , 0 , 0 , 49 , 49 );
 const auto u = astar( g );
 check( ( u.status == Status::unsolvable ) && ( u.stats.expanded == 25 * 50 ) ,
        "A* proves the wall unsolvable after the 1250 cells it reaches" );

 ParallelOptions po;
 po.threads = 4;
 const StateHashDistribution< Grid< 4 > > hd( g );
 const auto hu = HDAStar< Grid< 4 > , decltype( hd ) >( g , hd , po ).solve();
 check( hu.status == Status::unsolvable , "HDA* proves the wall unsolvable" );
 const auto su = SPAStar< Grid< 4 > >( g , po ).solve();
 check( su.status == Status::unsolvable , "SPA* proves the wall unsolvable" );

 po.node_limit = 5000;
 const ZobristDistribution< FifteenPuzzle > z( d );
 const auto hl = HDAStar< FifteenPuzzle , decltype( z ) >( d , z , po )
                  .solve();
 check( hl.status == Status::node_limit , "HDA* stops at the node limit" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// an admissible but inconsistent heuristic, which forces a reopening: A*
// must still find the optimal cost

static void test_reopen( void )
{
 // s = 0, goal = 4; the cheap path 0 - 1 - 2 - 3 - 4 costs 4, while the
 // heuristic of 1 lures A* into 0 - 2 first
 ExplicitGraph< int > g( 5 , 0 , 4 );
 g.add_edge( 0 , 1 , 1 , true );
 g.add_edge( 1 , 2 , 1 , true );
 g.add_edge( 0 , 2 , 3 , true );
 g.add_edge( 2 , 3 , 1 , true );
 g.add_edge( 3 , 4 , 1 , true );
 const int h[ 5 ] = { 0 , 3 , 0 , 1 , 0 };   // admissible: h* = 4 3 2 1 0
 for( std::uint32_t u = 0 ; u < 5 ; ++u )
  g.set_heuristic( u , h[ u ] );
 const auto s = astar( g );
 check_solution( g , s , 4 , "A*, inconsistent heuristic" );
 check( s.stats.reexpanded > 0 , "A* reopens a closed node" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// HDA* with every distribution, 1 to 8 workers, on the given runtime

template< class Runtime >
static void test_hda( const std::string & data )
{
 const auto inst = korf( data );
 for( unsigned p : { 1u , 2u , 3u , 8u } ) {
  ParallelOptions o;
  o.threads = p;
  for( int id : { easy_korf[ 0 ] , easy_korf[ 1 ] } ) {
   const auto & ki = inst[ id - 1 ];
   const FifteenPuzzle d( ki.board );
   const std::string tag = " p " + std::to_string( p ) + ", Korf #" +
                           std::to_string( id );
   const ZobristDistribution< FifteenPuzzle > z( d );
   check_solution( d , HDAStar< FifteenPuzzle , decltype( z ) , Runtime >(
                        d , z , o ).solve() , ki.cost , "HDA*[Z]" + tag );
   const AbstractZobristDistribution< FifteenPuzzle > az( d );
   check_solution( d , HDAStar< FifteenPuzzle , decltype( az ) , Runtime >(
                        d , az , o ).solve() , ki.cost , "HDA*[AZ]" + tag );
   const AbstractStateDistribution< FifteenPuzzle > a( d );
   check_solution( d , HDAStar< FifteenPuzzle , decltype( a ) , Runtime >(
                        d , a , o ).solve() , ki.cost , "HDA*[A]" + tag );
   const PerfectHashDistribution< FifteenPuzzle > ph( d );
   check_solution( d , HDAStar< FifteenPuzzle , decltype( ph ) , Runtime >(
                        d , ph , o ).solve() , ki.cost , "HDA*[P]" + tag );
   const RandomDistribution< FifteenPuzzle > r( d );
   check_solution( d , HDAStar< FifteenPuzzle , decltype( r ) , Runtime >(
                        d , r , o ).solve() , ki.cost , "HDA*[R]" + tag );
   }

  const auto m = GridMap::random( 300 , 200 , 0.25 , 2 );
  const Grid< 8 > g( m , 0 , 199 , 299 , 199 , 20 );
  const auto ref = astar( Blind< Grid< 8 > >{ g } );
  const AbstractZobristDistribution< Grid< 8 > > gz( g );
  check_solution( g , HDAStar< Grid< 8 > , decltype( gz ) , Runtime >(
                       g , gz , o ).solve() , ref.cost ,
                  "HDA*[AZ] grid 8, p " + std::to_string( p ) );

  auto rg = romania( Arad );
  const ZobristDistribution< ExplicitGraph< int > > rz( rg );
  check_solution( rg , HDAStar< ExplicitGraph< int > , decltype( rz ) ,
                                Runtime >( rg , rz , o ).solve() , 418 ,
                  "HDA*[Z] Romania, p " + std::to_string( p ) );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// SPA* with 1 to 8 workers, on the given runtime

template< class Runtime >
static void test_spa( const std::string & data )
{
 const auto inst = korf( data );
 for( unsigned p : { 1u , 2u , 3u , 8u } ) {
  ParallelOptions o;
  o.threads = p;
  for( int id : { easy_korf[ 0 ] , easy_korf[ 1 ] } ) {
   const auto & ki = inst[ id - 1 ];
   const FifteenPuzzle d( ki.board );
   check_solution( d , SPAStar< FifteenPuzzle , Runtime >( d , o ).solve() ,
                   ki.cost , "SPA* p " + std::to_string( p ) + ", Korf #" +
                             std::to_string( id ) );
   }
  auto rg = romania( Arad );
  check_solution( rg , SPAStar< ExplicitGraph< int > , Runtime >( rg , o )
                        .solve() , 418 ,
                  "SPA* Romania, p " + std::to_string( p ) );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// SafePBNF with 1 to 8 workers, on the given runtime

template< class Runtime >
static void test_pbnf( const std::string & data )
{
 const auto inst = korf( data );
 for( unsigned p : { 1u , 2u , 3u , 8u } ) {
  PBNFOptions o;
  o.threads = p;
  for( int id : { easy_korf[ 0 ] , easy_korf[ 1 ] } ) {
   const auto & ki = inst[ id - 1 ];
   const FifteenPuzzle d( ki.board );
   check_solution( d , SafePBNF< FifteenPuzzle , Runtime >( d , o ).solve() ,
                   ki.cost , "SafePBNF p " + std::to_string( p ) +
                             ", Korf #" + std::to_string( id ) );
   }
  for( std::uint64_t seed : { 2u , 3u } ) {
   const auto m = GridMap::random( 300 , 200 , 0.25 , seed );
   const Grid< 4 > g4( m , 0 , 199 , 299 , 199 , 100 , 10 );
   const auto r4 = astar( Blind< Grid< 4 > >{ g4 } );
   if( r4.solved() )
    check_solution( g4 , SafePBNF< Grid< 4 > , Runtime >( g4 , o ).solve() ,
                    r4.cost , "SafePBNF grid 4, p " + std::to_string( p ) );
   const Grid< 8 > g8( m , 0 , 199 , 299 , 199 , 100 , 10 );
   const auto r8 = astar( Blind< Grid< 8 > >{ g8 } );
   if( r8.solved() )
    check_solution( g8 , SafePBNF< Grid< 8 > , Runtime >( g8 , o ).solve() ,
                    r8.cost , "SafePBNF grid 8, p " + std::to_string( p ) );
   }
  }

 GridMap m( 60 , 60 );
 for( int y = 0 ; y < 60 ; ++y )
  m.set_cost( 30 , y , 0 );
 const Grid< 4 > g( m , 0 , 0 , 59 , 59 , 100 , 7 );
 PBNFOptions o;
 o.threads = 4;
 check( SafePBNF< Grid< 4 > , Runtime >( g , o ).solve().status ==
        Status::unsolvable , "SafePBNF proves the wall unsolvable" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// PA*SE with 1 to 8 workers: every state is expanded at most once

template< class Runtime >
static void test_pase( const std::string & data )
{
 const auto inst = korf( data );
 const EightPuzzle any( EightPuzzle::korf_goal() );
 const auto dist = eight_distances( any );
 for( unsigned p : { 1u , 2u , 3u , 8u } ) {
  PASEOptions o;
  o.threads = p;
  std::size_t k = 0;
  for( const auto & [ s , dst ] : dist ) {
   if( ( k++ % 9007 ) != 0 )                   // 21 states, spread out
    continue;
   const EightPuzzle d( EightPuzzle::unpack( s ) );
   const auto sol = PASE< EightPuzzle , Runtime >( d , o ).solve();
   if( ! sol.solved() || ( sol.cost != dst ) )
    check_solution( d , sol , dst , "PA*SE 8-puzzle, p " +
                                    std::to_string( p ) );
   }
  check( failures == 0 , "PA*SE on 21 states of the 8-puzzle, p " +
                         std::to_string( p ) );

  const auto m = GridMap::random( 200 , 120 , 0.25 , 3 );
  const Grid< 8 > g8( m , 0 , 119 , 199 , 119 );
  const auto r8 = astar( Blind< Grid< 8 > >{ g8 } );
  const SlowDomain< Grid< 8 > > slow( g8 , 2000 );
  if( r8.solved() ) {
   check_solution( g8 , PASE< Grid< 8 > , Runtime >( g8 , o ).solve() ,
                   r8.cost , "PA*SE grid 8, p " + std::to_string( p ) );
   const auto ss = PASE< SlowDomain< Grid< 8 > > , Runtime >( slow , o )
                    .solve();
   check_solution( slow , ss , r8.cost , "PA*SE slow grid 8, p " +
                                          std::to_string( p ) );
   check( ss.stats.reexpanded == 0 , "PA*SE expands each state once" );
   }

  o.eps = 1.5;
  const auto & ki = inst[ easy_korf[ 0 ] - 1 ];
  const FifteenPuzzle d( ki.board );
  const auto se = PASE< FifteenPuzzle , Runtime >( d , o ).solve();
  check( se.solved() && ( se.cost >= ki.cost ) &&
         ( se.cost <= 1.5 * ki.cost ) ,
         "PA*SE eps 1.5 within the bound, p " + std::to_string( p ) );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifdef ASTAR_HAS_FASTFLOW

// the master-worker FarmAStar, 1 to 8 workers, 1 or 4 nodes per task

static void test_farm( const std::string & data )
{
 const auto inst = korf( data );
 for( unsigned p : { 1u , 2u , 3u , 8u } )
  for( std::size_t b : { std::size_t( 1 ) , std::size_t( 4 ) } ) {
   ParallelOptions o;
   o.threads = p;
   o.batch = b;
   const std::string tag = " p " + std::to_string( p ) + " batch " +
                           std::to_string( b );
   const auto & ki = inst[ easy_korf[ 0 ] - 1 ];
   const FifteenPuzzle d( ki.board );
   check_solution( d , FarmAStar< FifteenPuzzle >( d , o ).solve() ,
                   ki.cost , "FarmAStar Korf #" +
                             std::to_string( easy_korf[ 0 ] ) + tag );
   const auto m = GridMap::random( 200 , 120 , 0.25 , 3 );
   const Grid< 8 > g8( m , 0 , 119 , 199 , 119 );
   const auto r8 = astar( Blind< Grid< 8 > >{ g8 } );
   if( r8.solved() )
    check_solution( g8 , FarmAStar< Grid< 8 > >( g8 , o ).solve() ,
                    r8.cost , "FarmAStar grid 8" + tag );
   auto rg = romania( Arad );
   check_solution( rg , FarmAStar< ExplicitGraph< int > >( rg , o ).solve() ,
                   418 , "FarmAStar Romania" + tag );
   }
 GridMap m( 40 , 40 );
 for( int y = 0 ; y < 40 ; ++y )
  m.set_cost( 20 , y , 0 );
 const Grid< 4 > g( m , 0 , 0 , 39 , 39 );
 ParallelOptions o;
 o.threads = 3;
 check( FarmAStar< Grid< 4 > >( g , o ).solve().status ==
        Status::unsolvable , "FarmAStar proves the wall unsolvable" );
 o.node_limit = 300;
 check( FarmAStar< Grid< 4 > >( g , o ).solve().status ==
        Status::node_limit , "FarmAStar stops at the node limit" );
 }

#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef ASTAR_HAS_MPI

// HDA* over MPI with every deterministic distribution

static void test_mpi( const std::string & data )
{
 const auto inst = korf( data );
 for( int id : { easy_korf[ 0 ] , easy_korf[ 2 ] } ) {
  const auto & ki = inst[ id - 1 ];
  const FifteenPuzzle d( ki.board );
  const std::string tag = ", Korf #" + std::to_string( id );
  ParallelOptions o;
  o.batch = 32;
  const ZobristDistribution< FifteenPuzzle > z( d );
  check_solution( d , HDAStarMPI< FifteenPuzzle , decltype( z ) >( d , z , o )
                       .solve() , ki.cost , "MPI HDA*[Z]" + tag );
  const AbstractZobristDistribution< FifteenPuzzle > az( d );
  check_solution( d , HDAStarMPI< FifteenPuzzle , decltype( az ) >( d , az ,
                       o ).solve() , ki.cost , "MPI HDA*[AZ]" + tag );
  const AbstractStateDistribution< FifteenPuzzle > a( d );
  check_solution( d , HDAStarMPI< FifteenPuzzle , decltype( a ) >( d , a , o )
                       .solve() , ki.cost , "MPI HDA*[A]" + tag );
  const PerfectHashDistribution< FifteenPuzzle > ph( d );
  check_solution( d , HDAStarMPI< FifteenPuzzle , decltype( ph ) >( d , ph ,
                       o ).solve() , ki.cost , "MPI HDA*[P]" + tag );
  }
 auto rg = romania( Arad );
 const StateHashDistribution< ExplicitGraph< int > > rh( rg );
 check_solution( rg , HDAStarMPI< ExplicitGraph< int > , decltype( rh ) >(
                       rg , rh ).solve() , 418 , "MPI HDA*[H] Romania" );

 GridMap m( 40 , 40 );
 for( int y = 0 ; y < 40 ; ++y )
  m.set_cost( 20 , y , 0 );
 const Grid< 4 > g( m , 0 , 0 , 39 , 39 );
 const StateHashDistribution< Grid< 4 > > gh( g );
 check( HDAStarMPI< Grid< 4 > , decltype( gh ) >( g , gh ).solve().status ==
        Status::unsolvable , "MPI HDA* proves the wall unsolvable" );

 ParallelOptions lo;
 lo.node_limit = 2000;
 const FifteenPuzzle d( inst[ 0 ].board );
 const ZobristDistribution< FifteenPuzzle > z( d );
 check( HDAStarMPI< FifteenPuzzle , decltype( z ) >( d , z , lo ).solve()
         .status == Status::node_limit , "MPI HDA* stops at the node limit" );
 }

#endif

// a 0-1 knapsack, its items sorted by decreasing profit / weight: a node
// fixes the first k items, its bound is the one of the fractional
// relaxation of Dantzig, the greedy completion of the node is its feasible
// value, and it is branched on the item k

struct Knapsack {

 struct node_type {
  std::size_t k = 0;              // the items fixed
  long weight = 0;                // the weight of those taken
  long profit = 0;                // their profit
  std::vector< char > x;          // the items taken
  };

 static constexpr Sense sense = Sense::maximize;

 std::vector< long > p;           // the profits
 std::vector< long > w;           // the weights
 long cap = 0;                    // the capacity

 node_type root( void ) const {
  node_type n;
  n.x.assign( p.size() , 0 );
  return( n );
  }

 // the value of the greedy completion of n
 long greedy( const node_type & n ) const {
  long r = cap - n.weight , v = n.profit;
  for( std::size_t i = n.k ; i < p.size() ; ++i )
   if( w[ i ] <= r ) {
    r -= w[ i ];
    v += p[ i ];
    }
  return( v );
  }

 Evaluation evaluate( node_type & n ) const {
  long r = cap - n.weight;
  double b = double( n.profit );
  std::size_t j = n.k;
  for( ; ( j < p.size() ) && ( w[ j ] <= r ) ; ++j ) {
   r -= w[ j ];
   b += double( p[ j ] );
   }
  if( j < p.size() )
   b += double( p[ j ] ) * double( r ) / double( w[ j ] );
  Evaluation e;
  e.bound = b;
  e.feasible = double( greedy( n ) );
  return( e );
  }

 template< class F >
 void branch( const node_type & n , F && f ) const {
  if( n.k == p.size() )
   return;
  // the item k left out first and taken last, so that with ties a depth
  // first search goes on with the item taken, as the greedy order does
  constexpr double none = std::numeric_limits< double >::infinity();
  node_type c = n;
  ++c.k;
  const bool fits = n.weight + w[ n.k ] <= cap;
  node_type t = c;
  f( std::move( c ) , none );
  if( fits ) {
   t.weight += w[ n.k ];
   t.profit += p[ n.k ];
   t.x[ n.k ] = 1;
   f( std::move( t ) , none );
   }
  }

 std::uint64_t hash( const node_type & n ) const {
  return( mix64( ( std::uint64_t( n.k ) << 40 ) ^
                 ( std::uint64_t( n.weight ) << 20 ) ^
                 std::uint64_t( n.profit ) ) );
  }

 double estimate( const node_type & n ) const {
  return( double( greedy( n ) ) );
  }

 // the bytes of a node, for MPI
 void serialize( const node_type & n , std::vector< char > & out ) const {
  const auto put = [ & ]( const auto & v ) {
   const auto * b = reinterpret_cast< const char * >( & v );
   out.insert( out.end() , b , b + sizeof( v ) );
   };
  put( n.k );
  put( n.weight );
  put( n.profit );
  out.insert( out.end() , n.x.begin() , n.x.end() );
  }

 node_type deserialize( const char * & in ) const {
  node_type n;
  std::memcpy( & n.k , in , sizeof( n.k ) );
  in += sizeof( n.k );
  std::memcpy( & n.weight , in , sizeof( n.weight ) );
  in += sizeof( n.weight );
  std::memcpy( & n.profit , in , sizeof( n.profit ) );
  in += sizeof( n.profit );
  n.x.assign( in , in + p.size() );
  in += p.size();
  return( n );
  }

 // the optimal value, by the dynamic program on the capacity
 long optimum( void ) const {
  std::vector< long > best( std::size_t( cap ) + 1 , 0 );
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   for( long c = cap ; c >= w[ i ] ; --c )
    best[ c ] = std::max( best[ c ] , best[ c - w[ i ] ] + p[ i ] );
  return( best[ cap ] );
  }

 // a random instance of the class of Pisinger cls (0 uncorrelated, 1
 // weakly correlated, 2 strongly correlated), of n items of weight at most
 // r, the capacity half the total weight
 static Knapsack random( int cls , std::size_t n , long r ,
                         std::uint64_t seed ) {
  std::mt19937_64 gen( seed );
  std::vector< std::pair< long , long > > it( n );
  long tw = 0;
  for( auto & [ pi , wi ] : it ) {
   wi = 1 + long( random_below( gen , std::uint64_t( r ) ) );
   if( cls == 0 )
    pi = 1 + long( random_below( gen , std::uint64_t( r ) ) );
   else
    if( cls == 1 )
     pi = std::max( 1L , wi - r / 10 + long( random_below(
                                   gen , std::uint64_t( r / 5 + 1 ) ) ) );
    else
     pi = wi + r / 10;
   tw += wi;
   }
  std::sort( it.begin() , it.end() , []( const auto & a , const auto & b ) {
   return( a.first * b.second > b.first * a.second );
   } );
  Knapsack kp;
  for( const auto & [ pi , wi ] : it ) {
   kp.p.push_back( pi );
   kp.w.push_back( wi );
   }
  kp.cap = tw / 2;
  return( kp );
  }

 // true if n is a node of value v: its greedy completion fits and gives v
 bool check( const node_type & n , double v ) const {
  long wt = 0 , pt = 0;
  for( std::size_t i = 0 ; i < n.k ; ++i )
   if( n.x[ i ] ) {
    wt += w[ i ];
    pt += p[ i ];
    }
  return( ( wt == n.weight ) && ( pt == n.profit ) && ( wt <= cap ) &&
          ( double( greedy( n ) ) == v ) );
  }

 };  // end( struct( Knapsack ) )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the knapsack as a minimization of the opposite profit, which exercises
// the other sense

struct MinKnapsack {

 using node_type = Knapsack::node_type;

 static constexpr Sense sense = Sense::minimize;

 const Knapsack & kp;

 node_type root( void ) const { return( kp.root() ); }

 Evaluation evaluate( node_type & n ) const {
  Evaluation e = kp.evaluate( n );
  e.bound = - e.bound;
  e.feasible = - *e.feasible;
  return( e );
  }

 template< class F >
 void branch( const node_type & n , F && f ) const {
  kp.branch( n , [ & ]( node_type && c , double e ) {
   f( std::move( c ) , - e );
   } );
  }

 void serialize( const node_type & n , std::vector< char > & out ) const {
  kp.serialize( n , out );
  }

 node_type deserialize( const char * & in ) const {
  return( kp.deserialize( in ) );
  }

 };  // end( struct( MinKnapsack ) )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the instances of the knapsack of the tests: 2 of each class of Pisinger

static std::vector< Knapsack > knapsacks( void )
{
 std::vector< Knapsack > v;
 for( int cls : { 0 , 1 , 2 } )
  for( std::uint64_t seed : { 1 , 2 } )
   v.push_back( Knapsack::random( cls , cls == 2 ? 30 : 50 , 1000 ,
                                  seed ) );
 return( v );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the sequential B&B on the knapsack against the dynamic program, with
// every selection, in both senses, with a relative tolerance, with an
// initial incumbent and with a node limit

static void test_bb( const std::string & )
{
 int i = 0;
 for( const Knapsack & kp : knapsacks() ) {
  const double opt = double( kp.optimum() );
  const std::string tag = " knapsack " + std::to_string( i++ );
  for( Selection sel : { Selection::best_first , Selection::depth_first ,
                         Selection::breadth_first ,
                         Selection::best_estimate , Selection::plunging } ) {
   BBOptions o;
   o.selection = sel;
   const auto s = branch_and_bound( kp , o );
   const std::string t = tag + " selection " +
                         std::to_string( int( sel ) );
   check( ( s.status == BBStatus::optimal ) && ( s.value == opt ) &&
          s.node && kp.check( *s.node , s.value ) && ( s.gap <= 1e-9 ) ,
          "B&B optimum" + t );
   const auto m = branch_and_bound( MinKnapsack{ kp } , o );
   check( ( m.status == BBStatus::optimal ) && ( m.value == - opt ) &&
          m.node && kp.check( *m.node , - m.value ) ,
          "B&B minimization" + t );
   }

  BBOptions o;
  o.rel_gap = 0.05;
  const auto r = branch_and_bound( kp , o );
  check( ( r.status == BBStatus::optimal ) &&
         ( r.value >= opt * 0.95 - 1e-9 ) && ( r.bound >= opt - 1e-9 ) &&
         ( r.gap <= 0.05 * r.value + 1e-9 ) ,
         "B&B within a relative gap of 5%" + tag );

  o = BBOptions();
  o.incumbent = opt;
  double seen = 0;
  o.on_incumbent = [ & ]( double v ) { seen = v; };
  const auto z = branch_and_bound( kp , o );
  check( ( z.status == BBStatus::optimal ) && ( z.value == opt ) &&
         ! z.node && ( seen == 0 ) , "B&B with the optimum given" + tag );

  o = BBOptions();
  o.node_limit = 3;
  const auto l = branch_and_bound( kp , o );
  check( ( l.status == BBStatus::node_limit ) && ( l.bound >= opt - 1e-9 ) &&
         ( l.stats.evaluated == 3 ) , "B&B node limit" + tag );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// a parallel B&B of the problem q: K = 0 the shared queue, K = 1 the
// distributed queues, K = 2 the farm (the runtime is then FastFlow)

template< int K , class Runtime , class Q >
static auto bb_parallel( const Q & q , const ParallelBBOptions & o )
{
 if constexpr( K == 0 )
  return( SharedBranchAndBound< Q , Runtime >( q , o ).solve() );
 else
  if constexpr( K == 1 )
   return( DistributedBranchAndBound< Q , Runtime >( q , o ).solve() );
  #ifdef ASTAR_HAS_FASTFLOW
  else
   return( FarmBranchAndBound< Q >( q , o ).solve() );
  #endif
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// a parallel B&B on the knapsack against the dynamic program, with 1 to 8
// workers and every selection, in both senses (the minimization has no
// hash( n ), and hence random owners), with a relative tolerance and with
// a node limit

template< int K , class Runtime >
static void test_bb_parallel( const std::string & )
{
 const std::string name = std::string( K == 0 ? "shared" :
                                       K == 1 ? "distributed" : "farm" ) +
                          " B&B on " + Runtime::name();
 int i = 0;
 for( const Knapsack & kp : knapsacks() ) {
  const double opt = double( kp.optimum() );
  const std::string tag = " knapsack " + std::to_string( i++ );
  for( unsigned p : { 1u , 2u , 3u , 8u } )
   for( Selection sel : { Selection::best_first , Selection::depth_first ,
                          Selection::breadth_first ,
                          Selection::best_estimate , Selection::plunging } ) {
    ParallelBBOptions o;
    o.threads = p;
    o.selection = sel;
    const auto s = bb_parallel< K , Runtime >( kp , o );
    check( ( s.status == BBStatus::optimal ) && ( s.value == opt ) &&
           s.node && kp.check( *s.node , s.value ) && ( s.gap <= 1e-9 ) ,
           name + tag + " p " + std::to_string( p ) + " selection " +
           std::to_string( int( sel ) ) );
    }

  ParallelBBOptions o;
  o.threads = 4;
  const auto m = bb_parallel< K , Runtime >( MinKnapsack{ kp } , o );
  check( ( m.status == BBStatus::optimal ) && ( m.value == - opt ) &&
         m.node && kp.check( *m.node , - m.value ) ,
         name + " minimization" + tag );

  o.rel_gap = 0.05;
  const auto r = bb_parallel< K , Runtime >( kp , o );
  check( ( r.status == BBStatus::optimal ) &&
         ( r.value >= opt * 0.95 - 1e-9 ) && ( r.bound >= opt - 1e-9 ) ,
         name + " within a relative gap of 5%" + tag );

  o = ParallelBBOptions();
  o.threads = 4;
  o.node_limit = 3;
  const auto l = bb_parallel< K , Runtime >( kp , o );
  check( ( l.status == BBStatus::node_limit ) && ( l.bound >= opt - 1e-9 ) ,
         name + " node limit" + tag );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifdef ASTAR_HAS_MPI

// the B&B over MPI on the knapsack against the dynamic program, with every
// selection, in both senses (the minimization has no hash( n ), and hence
// random owners), with a relative tolerance and with a node limit

static void test_bb_mpi( const std::string & )
{
 int i = 0;
 for( const Knapsack & kp : knapsacks() ) {
  const double opt = double( kp.optimum() );
  const std::string tag = " knapsack " + std::to_string( i++ );
  for( Selection sel : { Selection::best_first , Selection::depth_first ,
                         Selection::breadth_first ,
                         Selection::best_estimate , Selection::plunging } ) {
   ParallelBBOptions o;
   o.selection = sel;
   o.batch = 4;
   const auto s = MPIBranchAndBoundSearch< Knapsack >( kp , o ).solve();
   check( ( s.status == BBStatus::optimal ) && ( s.value == opt ) &&
          s.node && kp.check( *s.node , s.value ) && ( s.gap <= 1e-9 ) ,
          "MPI B&B" + tag + " selection " + std::to_string( int( sel ) ) );
   }

  ParallelBBOptions o;
  const MinKnapsack mk{ kp };
  const auto m = MPIBranchAndBoundSearch< MinKnapsack >( mk , o ).solve();
  check( ( m.status == BBStatus::optimal ) && ( m.value == - opt ) &&
         m.node && kp.check( *m.node , - m.value ) ,
         "MPI B&B minimization" + tag );

  o.rel_gap = 0.05;
  const auto r = MPIBranchAndBoundSearch< Knapsack >( kp , o ).solve();
  check( ( r.status == BBStatus::optimal ) &&
         ( r.value >= opt * 0.95 - 1e-9 ) && ( r.bound >= opt - 1e-9 ) ,
         "MPI B&B within a relative gap of 5%" + tag );

  o = ParallelBBOptions();
  o.node_limit = 3;
  const auto l = MPIBranchAndBoundSearch< Knapsack >( kp , o ).solve();
  check( ( l.status == BBStatus::node_limit ) && ( l.bound >= opt - 1e-9 ) ,
         "MPI B&B node limit" + tag );
  }
 }

#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the cases of the tester, each with the runtime it needs ("-" if none):
// test/tests.txt lists the same ones [see test_registry()]

struct Case {
 const char * name;                          // the name on the command line
 const char * needs;                         // the runtime it needs
 void ( * run )( const std::string & );      // the test, given the data
 };

static const std::vector< Case > cases = {
 { "astar" , "-" , test_astar } ,
 { "grid" , "-" , []( const std::string & ) { test_grid(); } } ,
 { "graph" , "-" , []( const std::string & ) { test_graph(); } } ,
 { "limits" , "-" , test_limits } ,
 { "reopen" , "-" , []( const std::string & ) { test_reopen(); } } ,
 { "hda-thread" , "-" , test_hda< ThreadRuntime > } ,
 { "spa-thread" , "-" , test_spa< ThreadRuntime > } ,
 { "pbnf-thread" , "-" , test_pbnf< ThreadRuntime > } ,
 { "pase-thread" , "-" , test_pase< ThreadRuntime > } ,
 { "bb" , "-" , test_bb } ,
 { "bb-shared-thread" , "-" , test_bb_parallel< 0 , ThreadRuntime > } ,
 { "bb-dist-thread" , "-" , test_bb_parallel< 1 , ThreadRuntime > } ,
 #ifdef ASTAR_HAS_OPENMP
  { "hda-openmp" , "openmp" , test_hda< OpenMPRuntime > } ,
  { "spa-openmp" , "openmp" , test_spa< OpenMPRuntime > } ,
  { "pbnf-openmp" , "openmp" , test_pbnf< OpenMPRuntime > } ,
  { "bb-shared-openmp" , "openmp" ,
    test_bb_parallel< 0 , OpenMPRuntime > } ,
  { "bb-dist-openmp" , "openmp" , test_bb_parallel< 1 , OpenMPRuntime > } ,
 #endif
 #ifdef ASTAR_HAS_FASTFLOW
  { "hda-fastflow" , "fastflow" , test_hda< FastFlowRuntime > } ,
  { "farm-fastflow" , "fastflow" , test_farm } ,
  { "bb-shared-fastflow" , "fastflow" ,
    test_bb_parallel< 0 , FastFlowRuntime > } ,
  { "bb-dist-fastflow" , "fastflow" ,
    test_bb_parallel< 1 , FastFlowRuntime > } ,
  { "bb-farm-fastflow" , "fastflow" ,
    test_bb_parallel< 2 , FastFlowRuntime > } ,
 #endif
 #ifdef ASTAR_HAS_MPI
  { "mpi" , "mpi" , test_mpi } ,
  { "bb-mpi" , "mpi" , test_bb_mpi } ,
 #endif
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// the cases of the list (test/tests.txt) whose runtime is built are those
// of the tester, and vice versa

static void test_registry( const std::string & list )
{
 const auto built = []( const std::string & needs ) {
  if( needs == "-" )
   return( true );
  #ifdef ASTAR_HAS_OPENMP
   if( needs == "openmp" )
    return( true );
  #endif
  #ifdef ASTAR_HAS_FASTFLOW
   if( needs == "fastflow" )
    return( true );
  #endif
  #ifdef ASTAR_HAS_MPI
   if( needs == "mpi" )
    return( true );
  #endif
  return( false );
  };

 std::ifstream in( list );
 check( bool( in ) , "the list " + list + " is read" );
 std::set< std::string > listed;
 for( std::string line ; std::getline( in , line ) ; ) {
  std::istringstream is( line );
  std::string name , needs;
  if( ( is >> name >> needs ) && ( name[ 0 ] != '#' ) && built( needs ) )
   listed.insert( name );
  }
 std::set< std::string > tester;
 for( const Case & c : cases )
  tester.insert( c.name );
 for( const auto & n : listed )
  check( tester.count( n ) == 1 , "the case " + n + " is in the tester" );
 for( const auto & n : tester )
  check( listed.count( n ) == 1 , "the case " + n + " is in the list" );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 if( argc < 2 ) {
  std::cerr << "usage: " << argv[ 0 ] << " <case> [<data dir>]" << std::endl;
  return( 2 );
  }
 const std::string what = argv[ 1 ];
 const std::string data = argc > 2 ? argv[ 2 ] : "../data";
 const auto c = std::find_if( cases.begin() , cases.end() ,
                              [ & ]( const Case & x ) {
                               return( what == x.name );
                               } );
 if( ( what != "registry" ) && ( c == cases.end() ) ) {
  std::cerr << "unknown case " << what << std::endl;
  return( 2 );
  }
 [[maybe_unused]] const bool mpi = ( c != cases.end() ) &&
                                   ( std::string( c->needs ) == "mpi" );

 #ifdef ASTAR_HAS_MPI
  int rank = 0;
  if( mpi ) {
   MPI_Init( & argc , & argv );
   MPI_Comm_rank( MPI_COMM_WORLD , & rank );
   quiet = rank != 0;
   }
 #endif

 try {
  if( what == "registry" )
   test_registry( data );
  else
   c->run( data );
  }
 catch( std::exception & e ) {
  std::cout << "FAILED: exception " << e.what() << std::endl;
  ++failures;
  }

 #ifdef ASTAR_HAS_MPI
  if( mpi ) {
   int all = 0;
   MPI_Allreduce( & failures , & all , 1 , MPI_INT , MPI_SUM ,
                  MPI_COMM_WORLD );
   failures = all;
   MPI_Finalize();
   }
 #endif

 if( ! quiet )
  std::cout << ( failures ? "FAILED: " : "passed: " ) << what << " ("
            << failures << " failures)" << std::endl;
 return( failures ? 1 : 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File test.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/

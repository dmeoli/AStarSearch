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
 *     test mpi                      (under mpiexec)
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

#include <cmath>

#include <fstream>

#include <iostream>

#include <queue>

#include <sstream>

#include <string>

#include <unordered_map>

#include <vector>

#include "AStarSearchConfig.h"

#include "AStar.h"

#include "Distribution.h"

#include "ExplicitGraph.h"

#include "Grid.h"

#include "HDAStar.h"

#include "SPAStar.h"

#include "PASE.h"

#include "SafePBNF.h"

#include "SlowDomain.h"

#include "SlidingTile.h"

#ifdef ASTAR_HAS_OPENMP
 #include "OpenMPRuntime.h"
#endif

#ifdef ASTAR_HAS_FASTFLOW
 #include "FarmAStar.h"
 #include "FastFlowRuntime.h"
#endif

#ifdef ASTAR_HAS_MPI
 #include "HDAStarMPI.h"
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

 #ifdef ASTAR_HAS_MPI
  int rank = 0;
  if( what == "mpi" ) {
   MPI_Init( & argc , & argv );
   MPI_Comm_rank( MPI_COMM_WORLD , & rank );
   quiet = rank != 0;
   }
 #endif

 try {
  if( what == "astar" )
   test_astar( data );
  else
   if( what == "grid" )
    test_grid();
   else
    if( what == "graph" )
     test_graph();
    else
     if( what == "limits" )
      test_limits( data );
     else
      if( what == "reopen" )
       test_reopen();
      else
       if( what == "hda-thread" )
        test_hda< ThreadRuntime >( data );
       else
        if( what == "spa-thread" )
         test_spa< ThreadRuntime >( data );
        else
         if( what == "pbnf-thread" )
          test_pbnf< ThreadRuntime >( data );
         else
          if( what == "pase-thread" )
           test_pase< ThreadRuntime >( data );
        #ifdef ASTAR_HAS_OPENMP
        else
         if( what == "hda-openmp" )
          test_hda< OpenMPRuntime >( data );
         else
          if( what == "spa-openmp" )
           test_spa< OpenMPRuntime >( data );
          else
           if( what == "pbnf-openmp" )
            test_pbnf< OpenMPRuntime >( data );
        #endif
        #ifdef ASTAR_HAS_FASTFLOW
        else
         if( what == "hda-fastflow" )
          test_hda< FastFlowRuntime >( data );
         else
          if( what == "farm-fastflow" )
           test_farm( data );
        #endif
        #ifdef ASTAR_HAS_MPI
        else
         if( what == "mpi" )
          test_mpi( data );
        #endif
        else {
         std::cerr << "unknown case " << what << std::endl;
         return( 2 );
         }
  }
 catch( std::exception & e ) {
  std::cout << "FAILED: exception " << e.what() << std::endl;
  ++failures;
  }

 #ifdef ASTAR_HAS_MPI
  if( what == "mpi" ) {
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

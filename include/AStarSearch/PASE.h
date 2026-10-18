/*--------------------------------------------------------------------------*/
/*------------------------------ File PASE.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template PASE, Parallel A* for Slow Expansions
 * of Phillips, Likhachev and Koenig (ICAPS 2014).
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __PASE
 #define __PASE
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>

#include <cmath>

#include <condition_variable>

#include <memory>

#include <mutex>

#include <set>

#include <tuple>

#include <type_traits>

#include <vector>

#include "AStarSearch/HDAStar.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PASEOptions ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the options of PASE

struct PASEOptions : public ParallelOptions {

 /// the inflation eps_p >= 1 of the independence rule: with eps_p = 1 the
 /// search is optimal, otherwise the cost is within eps_p of the optimum
 double eps = 1;

 };  // end( struct( PASEOptions ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASS PASE -------------------------------*/
/*--------------------------------------------------------------------------*/
/// Parallel A* for Slow Expansions
/** A state s is independent of a state s' if g( s ) - g( s' ) <= eps_p
 * h( s' , s ), h( s' , s ) being the forward-backward consistent pairwise
 * heuristic of the Domain: then no path through s' can lower g( s ) below
 * its value divided by eps_p. A state is safe to expand if it is
 * independent of all the states being expanded (the set BE) and of all
 * the states of OPEN with a smaller f [see Theorem 2 of the paper]. Each
 * worker takes from OPEN the safe state of smallest f, expands it outside
 * the lock and puts back the successors whose g has improved; a state is
 * expanded at most once, and when it is expanded its g is optimal (eps_p =
 * 1). The search ends when the goal is safe to expand, or when OPEN and BE
 * are both empty. This is Algorithm 1 of the paper; OPEN is an ordered set
 * scanned in increasing f, and a worker that finds no safe state waits
 * until OPEN or BE change.
 *
 * Finding a safe state costs a scan of the states of OPEN with a smaller f
 * and of BE, all under one lock: PASE pays only when the expansions are
 * slow, as in motion planning with collision checks. */

template< PairwiseDomain D , class Runtime = ThreadRuntime >
class PASE {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using State = state_t< D >;                       ///< the state type
 using Cost = cost_t< D >;                         ///< the cost type
 using NodeT = Node< State , Cost >;               ///< the node type
 using SolutionT = Solution< State , Cost >;       ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the Domain (which must outlive the PASE)

 explicit PASE( const D & domain , const PASEOptions & options = {} )
  : f_domain( domain ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  if( f_options.eps < 1 )
   f_options.eps = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the Domain, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  const unsigned p = f_options.threads;
  Stopwatch clock;
  Shared sh;
  std::vector< std::unique_ptr< Worker > > w( p );
  for( unsigned i = 0 ; i < p ; ++i )
   w[ i ] = std::make_unique< Worker >( *this , sh );

  {
   const State s0 = f_domain.initial_state();
   NodeT * root = w[ 0 ]->f_pool.allocate();
   *root = NodeT{ s0 , Cost( 0 ) , Cost( f_domain.heuristic( s0 ) ) ,
                  nullptr , false , false };
   sh.table.find_or_insert( s0 , f_domain.hash( s0 ) ,
                            [ & ] { return( root ); } );
   sh.open.emplace( root->f() , - root->g , root );
   }

  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run(); } );

  SolutionT sol;
  SearchStats & st = sol.stats;
  for( auto & x : w ) {
   st.expanded += x->f_st.expanded;
   st.generated += x->f_st.generated;
   st.duplicates += x->f_st.duplicates;
   st.stored += x->f_pool.size();
   st.expanded_by.push_back( x->f_st.expanded );
   st.stored_by.push_back( x->f_pool.size() );
   }
  if( sh.limit_hit )
   sol.status = Status::node_limit;
  else
   if( sh.goal ) {
    sol.status = Status::solved;
    sol.cost = sh.goal->g;
    sol.path = trace_path( sh.goal );
    }
  st.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 using Lock = typename Runtime::lock_type;

 // OPEN: by f, then by larger g, then by address
 using Key = std::tuple< Cost , Cost , NodeT * >;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers, all protected by lock

 struct Shared {
  Lock lock;                                    // protects everything
  std::condition_variable_any changed;          // OPEN or BE changed
  std::set< Key > open;                         // OPEN
  std::vector< NodeT * > be;                    // being expanded
  NodeTable< NodeT > table;                     // all the states
  bool done = false;                            // the search is over
  bool limit_hit = false;                       // the node limit is hit
  NodeT * goal = nullptr;                       // the goal, once safe
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: its pool of nodes and its counters

 struct alignas( cache_line ) Worker {

  Worker( const PASE & a , Shared & s ) : f_a( a ) , f_sh( s ) {}

  // the states of OPEN that a worker examines for a safe one
  static constexpr std::size_t c_candidates = 64;

  // true if s is independent of t: g( s ) - g( t ) <= eps h( t , s ), up
  // to a relative 1e-9 for real costs, whose rounding would otherwise make
  // the goal dependent on the states with f = f( goal )
  bool independent( const NodeT * s , const NodeT * t ) const {
   const double lhs = double( s->g - t->g );
   const double rhs = f_a.f_options.eps *
    double( f_a.f_domain.pairwise_heuristic( t->state , s->state ) );
   if constexpr( std::is_floating_point_v< Cost > )
    return( lhs <= rhs + 1e-9 * std::max( 1.0 ,
                                          std::abs( double( s->g ) ) ) );
   else
    return( lhs <= rhs );
   }

  // true if s is safe to expand (lock held): independent of BE and of the
  // states of OPEN with a smaller f, which come first in OPEN, as in the
  // definition of safety of the paper; the states of equal f are not
  // checked, so that a layer of equal f costs a scan of BE per state
  // instead of a scan of the layer
  bool safe( const NodeT * s ) const {
   for( const NodeT * t : f_sh.be )
    if( ! independent( s , t ) )
     return( false );
   const Cost fs = s->f();
   for( const auto & k : f_sh.open ) {
    const NodeT * t = std::get< 2 >( k );
    if( ! ( t->f() < fs ) )
     break;
    if( ! independent( s , t ) )
     return( false );
    }
   return( true );
   }

  // Algorithm 1 of the paper
  void run( void ) {
   std::vector< std::tuple< State , Cost , Cost > > succ;
   std::unique_lock< Lock > guard( f_sh.lock );
   while( ! f_sh.done ) {
    // the safe state of smallest f among the first c_candidates of OPEN,
    // if any: further down, each candidate would have to be checked
    // against the larger and larger part of OPEN before it, under the lock;
    // with BE empty the first state is safe, and the search goes on
    NodeT * s = nullptr;
    std::size_t n = 0;
    for( const auto & k : f_sh.open ) {
     if( n++ == c_candidates )
      break;
     if( safe( std::get< 2 >( k ) ) ) {
      s = std::get< 2 >( k );
      break;
      }
     }
    if( ! s ) {
     if( f_sh.open.empty() && f_sh.be.empty() ) {
      f_sh.done = true;
      f_sh.changed.notify_all();
      break;
      }
     f_sh.changed.wait( guard );
     continue;
     }
    f_sh.open.erase( Key( s->f() , - s->g , s ) );
    if( f_a.f_domain.is_goal( s->state ) ) {
     f_sh.goal = s;               // safe, hence of optimal g
     f_sh.done = true;
     f_sh.changed.notify_all();
     break;
     }
    f_sh.be.push_back( s );
    s->closed = s->expanded = true;
    const State st = s->state;
    const Cost g = s->g , h = s->h;
    guard.unlock();

    ++f_st.expanded;
    succ.clear();
    f_a.f_domain.for_each_successor( st , h , [ & ]( const State & t ,
                                                      Cost c , Cost ht ) {
     succ.emplace_back( t , g + c , ht );
     } );

    guard.lock();
    for( const auto & sc : succ ) {    // no structured binding: OpenMP
     const State & t = std::get< 0 >( sc );     // cannot capture one
     const Cost gt = std::get< 1 >( sc ) , ht = std::get< 2 >( sc );
     ++f_st.generated;
     auto [ x , fresh ] = f_sh.table.find_or_insert( t ,
                                                     f_a.f_domain.hash( t ) ,
                                                     [ & ] {
      NodeT * y = f_pool.allocate();
      *y = NodeT{ t , gt , ht , s , false , false };
      return( y );
      } );
     if( fresh )
      f_sh.open.emplace( x->f() , - x->g , x );
     else
      if( ! x->closed && ( gt < x->g ) ) {
       f_sh.open.erase( Key( x->f() , - x->g , x ) );
       x->g = gt;
       x->parent = s;
       f_sh.open.emplace( x->f() , - x->g , x );
       }
      else
       ++f_st.duplicates;
     }
    f_sh.be.erase( std::find( f_sh.be.begin() , f_sh.be.end() , s ) );
    if( f_a.f_options.node_limit &&
        ( f_pool.size() >= f_a.f_options.node_limit ) ) {
     f_sh.limit_hit = true;
     f_sh.done = true;
     }
    f_sh.changed.notify_all();
    }
   }

  const PASE & f_a;                             // the search
  Shared & f_sh;                                // the shared data
  NodePool< NodeT > f_pool;                     // the nodes it allocated
  SearchStats f_st;                             // its counters
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const D & f_domain;                                   ///< the Domain
 PASEOptions f_options;                                ///< the options

 };  // end( class( PASE ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PASE */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File PASE.h -----------------------------*/
/*--------------------------------------------------------------------------*/

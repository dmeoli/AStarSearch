/*--------------------------------------------------------------------------*/
/*----------------------------- File SPAStar.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template SPAStar, Simple Parallel A*: all the
 * workers share one OPEN and one CLOSED list.
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

#ifndef __PBeFS_SPAStar
 #define __PBeFS_SPAStar
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <mutex>

#include <type_traits>

#include <vector>

#include "PBeFS/astar/HDAStar.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Parallel Best-First Search library
namespace PBeFS_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS SPAStar ------------------------------*/
/*--------------------------------------------------------------------------*/
/// Simple Parallel A*: one OPEN and one CLOSED list shared by the workers
/** This is Algorithm 2 of Fukunaga et al. (2017), due to Irani and Shih
 * (1986): each worker takes the best node from the shared OPEN list,
 * expands it, and puts the successors back; the incumbent prunes the nodes
 * with f not smaller, and the search ends when OPEN has no node with f
 * smaller than the incumbent and no worker is expanding. A node may be
 * expanded before its g is optimal, and then re-expanded, as with most
 * parallel A* variants.
 *
 * OPEN is protected by one lock. CLOSED is split into 256 shards by the
 * hash of the state, each with its own lock, which also protects the nodes
 * of its states: the stale test of an entry popped from OPEN is done under
 * the lock of the shard of its node. The expansion itself runs with no
 * lock held. The survey reports that, when the expansions are as fast as on
 * the sliding-tile puzzle, the lock of OPEN makes SPA* slower than A*; it
 * pays when the expansions are slow. */

template< Domain D , class Runtime = ThreadRuntime ,
          class Open = std::conditional_t<
           std::is_integral_v< cost_t< D > > ,
           BucketOpenList< Node< state_t< D > , cost_t< D > > > ,
           HeapOpenList< Node< state_t< D > , cost_t< D > > > > >
class SPAStar {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using State = state_t< D >;                       ///< the state type
 using Cost = cost_t< D >;                         ///< the cost type
 using NodeT = Node< State , Cost >;               ///< the node type
 using SolutionT = Solution< State , Cost >;       ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the Domain (which must outlive the SPAStar)

 explicit SPAStar( const D & domain , const ParallelOptions & options = {} )
  : f_domain( domain ) , f_options( options ) {
  f_options.normalize();
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

  {  // the root, inserted by the first worker before the start
   const State s0 = f_domain.initial_state();
   NodeT * root = w[ 0 ]->update( s0 , Cost( 0 ) ,
                                  Cost( f_domain.heuristic( s0 ) ) ,
                                  nullptr );
   sh.open.push( root );
   }

  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run(); } );

  SolutionT sol;
  SearchStats & st = sol.stats;
  for( auto & x : w ) {
   st.expanded += x->f_st.expanded;
   st.reexpanded += x->f_st.reexpanded;
   st.generated += x->f_st.generated;
   st.duplicates += x->f_st.duplicates;
   st.stored += x->f_pool.size();
   st.expanded_by.push_back( x->f_st.expanded );
   st.stored_by.push_back( x->f_pool.size() );
   }
  if( sh.limit_hit.load() )
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

 static constexpr unsigned c_shards = 256;         ///< shards of CLOSED

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a shard of CLOSED and its lock

 struct alignas( cache_line ) Shard {
  Lock lock;
  NodeTable< NodeT > table;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers

 struct Shared {
  alignas( cache_line ) Lock open_lock;         // protects open and busy
  Open open;                                    // the OPEN list
  unsigned busy = 0;                            // workers expanding
  std::unique_ptr< Shard[] > shards{ new Shard[ c_shards ] };
  alignas( cache_line ) std::atomic< Cost > incumbent{
                                                   infinite_cost< Cost >() };
  std::atomic< bool > done{ false };
  std::atomic< bool > limit_hit{ false };
  std::mutex goal_mutex;                        // protects goal
  NodeT * goal = nullptr;                       // the best goal found
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: its pool of nodes and its counters

 struct alignas( cache_line ) Worker {

  Worker( const SPAStar & a , Shared & s ) : f_a( a ) , f_sh( s ) {}

  // the shard of a hash value
  Shard & shard( std::uint64_t h ) {
   return( f_sh.shards[ h >> 56 ] );
   }

  // the main loop
  void run( void ) {
   Backoff backoff;
   std::vector< OpenEntry< NodeT > > fresh;
   while( ! f_sh.done.load( std::memory_order_acquire ) ) {
    OpenEntry< NodeT > e{ nullptr , Cost( 0 ) };
    {
     std::lock_guard< Lock > guard( f_sh.open_lock );
     if( ! f_sh.open.empty() && ( f_sh.open.top_f() <
                                  f_sh.incumbent.load() ) ) {
      e = f_sh.open.pop();
      ++f_sh.busy;
      }
     else
      if( f_sh.busy == 0 ) {
       f_sh.done.store( true , std::memory_order_release );
       break;
       }
     }
    if( ! e.node ) {
     backoff.pause();
     continue;
     }
    backoff.reset();
    fresh.clear();
    expand( e , fresh );
    std::lock_guard< Lock > guard( f_sh.open_lock );
    for( const auto & x : fresh )
     f_sh.open.push( x.node , x.g );
    --f_sh.busy;
    }
   }

  // expands the node of e unless the entry is stale; the nodes to put in
  // OPEN, with the g they had under the lock of their shard, are appended
  // to fresh
  void expand( const OpenEntry< NodeT > & e ,
               std::vector< OpenEntry< NodeT > > & fresh ) {
   NodeT * n = e.node;
   State s;
   Cost g , h;
   {
    Shard & sd = shard( f_a.f_domain.hash( n->state ) );
    std::lock_guard< Lock > guard( sd.lock );
    if( e.stale() )
     return;
    n->closed = true;
    if( n->expanded )
     ++f_st.reexpanded;
    n->expanded = true;
    s = n->state;
    g = n->g;
    h = n->h;
    }
   ++f_st.expanded;

   if( f_a.f_domain.is_goal( s ) ) {
    std::lock_guard< std::mutex > guard( f_sh.goal_mutex );
    if( g < f_sh.incumbent.load() ) {
     f_sh.incumbent.store( g );
     f_sh.goal = n;
     }
    return;
    }

   f_a.f_domain.for_each_successor( s , h , [ & ]( const State & t , Cost c ,
                                                   Cost ht ) {
    ++f_st.generated;
    const Cost gt = g + c;
    if( gt + ht >= f_sh.incumbent.load( std::memory_order_relaxed ) )
     return;
    if( NodeT * x = update( t , gt , ht , n ) )
     fresh.push_back( OpenEntry< NodeT >{ x , gt } );
    } );

   if( f_a.f_options.node_limit &&
       ( f_pool.size() >= f_a.f_options.node_limit ) ) {
    f_sh.limit_hit.store( true );
    f_sh.done.store( true , std::memory_order_release );
    }
   }

  // records the path to t of cost g through parent; returns the node of t
  // if it has to go into OPEN, nullptr if it is a duplicate
  NodeT * update( const State & t , Cost g , Cost h , NodeT * parent ) {
   const std::uint64_t hash = f_a.f_domain.hash( t );
   Shard & sd = shard( hash );
   std::lock_guard< Lock > guard( sd.lock );
   auto [ x , fresh ] = sd.table.find_or_insert( t , hash , [ & ] {
    NodeT * y = f_pool.allocate();
    *y = NodeT{ t , g , h , parent , false , false };
    return( y );
    } );
   if( fresh )
    return( x );
   if( g < x->g ) {
    x->g = g;
    x->parent = parent;
    x->closed = false;
    return( x );
    }
   ++f_st.duplicates;
   return( nullptr );
   }

  const SPAStar & f_a;                          // the search
  Shared & f_sh;                                // the shared data
  NodePool< NodeT > f_pool;                     // the nodes it allocated
  SearchStats f_st;                             // its counters
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const D & f_domain;                                   ///< the Domain
 ParallelOptions f_options;                            ///< the options

 };  // end( class( SPAStar ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace PBeFS_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_SPAStar */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File SPAStar.h ---------------------------*/
/*--------------------------------------------------------------------------*/

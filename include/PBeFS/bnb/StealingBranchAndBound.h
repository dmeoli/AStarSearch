/*--------------------------------------------------------------------------*/
/*--------------------- File StealingBranchAndBound.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for StealingBranchAndBound, the parallel Branch-and-Bound
 * (B&B) whose workers keep the children they generate in a queue of their
 * own and take a node from the queue of another worker only when theirs is
 * empty, i.e., by work stealing. The problem, the options and the Solution
 * are those of BranchAndBound.h and ParallelBranchAndBound.h, and the
 * design is discussed in docs/branch-and-bound.md.
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

#ifndef __PBeFS_StealingBranchAndBound
 #define __PBeFS_StealingBranchAndBound
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <cstdint>

#include <memory>

#include <mutex>

#include <optional>

#include <random>

#include <vector>

#include "PBeFS/bnb/ParallelBranchAndBound.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Parallel Best-First Search library
namespace PBeFS_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS StealingBranchAndBound ----------------------*/
/*--------------------------------------------------------------------------*/
/// the parallel B&B by work stealing
/** Each worker extracts the nodes from its own queue and pushes there the
 * children it generates (with depth-first or plunging, the child that dives
 * is kept), so that a worker goes on in the part of the tree where it is,
 * which is what matters when moving a worker from a node to a distant one
 * costs (e.g., when the evaluator of a worker keeps the changes along the
 * path from the root to its last node). A worker whose queue is empty takes
 * the best node of the queue of another one, the victims being tried from a
 * random one on. The selection is then best-first within each queue only,
 * and the order of extraction may be farther from the sequential one than
 * that of SharedBranchAndBound. The incumbent is shared, and the search
 * ends when no node is open nor held by a worker, which a counter of those
 * nodes tells. */

template< BranchAndBound P , class Runtime = ThreadRuntime >
class StealingBranchAndBound {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using Node = typename P::node_type;            ///< the node type
 using SolutionT = BBSolution< Node >;          ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the problem (which must outlive the search)

 explicit StealingBranchAndBound( const P & problem ,
                                  const ParallelBBOptions & options = {} )
  : f_p( problem ) , f_options( options ) {
  f_options.normalize();
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the tree from the root, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const {
  return( run( std::nullopt ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// searches the subtrees of the given nodes, returning the Solution [see
 /// BranchAndBoundSearch::solve()]; the node i goes to the worker i mod p

 [[nodiscard]] SolutionT solve( std::vector< BBLeaf< Node > > seeds ) const {
  return( run( std::move( seeds ) ) );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 SolutionT run( std::optional< std::vector< BBLeaf< Node > > > seeds ) const
 {
  constexpr double sgn = detail::bb_sign< P >();
  constexpr double inf = std::numeric_limits< double >::infinity();
  const unsigned p = f_options.threads;
  Stopwatch clock;
  Shared sh( p , f_options.incumbent ? sgn * *f_options.incumbent : inf );

  std::vector< std::unique_ptr< Worker > > w( p );
  for( unsigned i = 0 ; i < p ; ++i )
   w[ i ] = std::make_unique< Worker >( *this , sh , w , i , clock );
  std::int64_t n = 0;
  for( auto & l : detail::bb_starts( f_p , std::move( seeds ) ) )
   w[ std::size_t( n++ ) % p ]->push( Item{ std::move( l.node ) , l.bound ,
                                            l.depth } );
  sh.pending.store( n );
  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run(); } );

  // the nodes left open by a limit bound the optimum too
  SolutionT sol;
  detail::BBFrontier< Node > fr( f_options.keep_frontier );
  for( auto & x : w ) {
   sol.stats.add( x->f_st );
   fr.merge( std::move( x->f_fr ) );
   fr.add( x->f_q , BBLeafKind::open );
   if( x->f_dive )
    fr.add( std::move( x->f_dive->node ) , x->f_dive->bound ,
            x->f_dive->depth , BBLeafKind::open );
   }
  detail::bb_finish< P >( sol , sh.inc , fr , sh.limit.load() ,
                          sh.time.load() );
  sol.stats.seconds = clock.seconds();
  return( sol );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 using Lock = typename Runtime::lock_type;
 using Item = typename BBQueue< Node >::Item;
 struct Worker;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers

 struct Shared {

  Shared( unsigned np , double z ) : p( np ) , inc( z ) {}

  unsigned p;                                   // the number of workers
  BBIncumbent< Node > inc;                      // the incumbent
  // the nodes open in a queue, held by a worker or in its dive
  alignas( cache_line ) std::atomic< std::int64_t > pending{ 0 };
  alignas( cache_line ) std::atomic< std::uint64_t > evaluated{ 0 };
  alignas( cache_line ) std::atomic< bool > done{ false };
  std::atomic< bool > limit{ false };           // stopped by a limit
  std::atomic< bool > time{ false };            // ... the time limit
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: its queue, under its lock, and its counters

 struct alignas( cache_line ) Worker {

  Worker( const StealingBranchAndBound & a , Shared & s ,
          std::vector< std::unique_ptr< Worker > > & all , unsigned i ,
          const Stopwatch & c )
   : f_a( a ) , f_sh( s ) , f_all( all ) , f_id( i ) , f_clock( c ) ,
     f_q( a.f_options.selection ) , f_gen( a.f_options.seed + i ) ,
     f_fr( a.f_options.keep_frontier ) {}

  // the main loop
  void run( void ) {
   auto w = detail::bb_worker( f_a.f_p , f_id );
   const BBOptions & o = f_a.f_options;
   const auto inc = [ & ] { return( f_sh.inc.value() ); };
   const auto improve = [ & ]( double v , const Node & n ) {
    return( f_sh.inc.improve( v , n , o , detail::bb_sign< P >() ) );
    };

   for( ;; ) {
    std::optional< Item > it = next();
    if( ! it )
     return;
    if( stop() ) {                     // the node stays open
     f_fr.add( std::move( it->node ) , it->bound , it->depth ,
               BBLeafKind::open );
     return;
     }

    v_kids.clear();
    detail::bb_process< P >( it->node , it->bound , it->depth , w , inc ,
                             improve , [ & ]( Node && c , double b ,
                                              unsigned d ) {
                              v_kids.push_back( Item{ std::move( c ) , b ,
                                                      d } );
                              } , o , f_st , f_fr );
    f_sh.evaluated.fetch_add( 1 );

    // the children count before the node stops counting, in one step, so
    // that no other worker sees no node pending while some are
    std::lock_guard< Lock > guard( f_lock );
    const std::size_t k = detail::bb_dive( v_kids , f_q.best_bound() ,
                                           inc() , o );
    for( std::size_t j = 0 ; j < v_kids.size() ; ++j )
     if( j == k )
      f_dive.emplace( std::move( v_kids[ j ] ) );
     else
      push( std::move( v_kids[ j ] ) );
    f_sh.pending.fetch_add( std::int64_t( v_kids.size() ) - 1 );
    }
   }

  // pushes a node in the queue, under the lock unless the search has not
  // started yet
  void push( Item && it ) {
   const double key = detail::bb_key( f_a.f_p , it.node , it.bound ,
                                      it.depth , f_a.f_options.selection );
   f_q.push( std::move( it.node ) , it.bound , it.depth , key );
   f_size.store( f_q.size() , std::memory_order_relaxed );
   f_st.max_open = std::max( f_st.max_open , f_q.size() );
   }

  // the next node: the dive, the best of the queue, or one stolen; none
  // when the search is over, i.e., when no node is pending or a limit is
  // reached; the queue is dropped when none of its nodes can improve the
  // incumbent
  std::optional< Item > next( void ) {
   if( f_dive ) {
    std::optional< Item > it( std::move( f_dive ) );
    f_dive.reset();
    return( it );
    }
   Backoff backoff;
   for( ;; ) {
    if( f_sh.done.load( std::memory_order_acquire ) )
     return( std::nullopt );
    {
     std::lock_guard< Lock > guard( f_lock );
     if( ! f_q.empty() ) {
      if( detail::bb_prunable( f_q.best_bound() , f_sh.inc.value() ,
                               f_a.f_options ) ) {
       const std::size_t n = f_fr.add( f_q , BBLeafKind::pruned );
       f_st.pruned_extracted += n;
       f_size.store( 0 , std::memory_order_relaxed );
       f_sh.pending.fetch_sub( std::int64_t( n ) );
       continue;
       }
      std::optional< Item > it( f_q.pop() );
      f_size.store( f_q.size() , std::memory_order_relaxed );
      return( it );
      }
     }
    if( std::optional< Item > it = steal() )
     return( it );
    if( f_sh.pending.load() == 0 ) {
     f_sh.done.store( true , std::memory_order_release );
     return( std::nullopt );
     }
    backoff.pause();
    }
   }

  // the best node of the queue of another worker, the victims being tried
  // from a random one on; none if all their queues are empty
  std::optional< Item > steal( void ) {
   const unsigned p = f_sh.p;
   if( p < 2 )
    return( std::nullopt );
   const unsigned first = unsigned( random_below( f_gen , p ) );
   for( unsigned t = 0 ; t < p ; ++t ) {
    const unsigned v = ( first + t ) % p;
    if( v == f_id )
     continue;
    Worker & x = *f_all[ v ];
    if( x.f_size.load( std::memory_order_relaxed ) == 0 )
     continue;
    std::lock_guard< Lock > guard( x.f_lock );
    if( x.f_q.empty() )
     continue;
    std::optional< Item > it( x.f_q.pop() );
    x.f_size.store( x.f_q.size() , std::memory_order_relaxed );
    ++f_st.sent;
    return( it );
    }
   return( std::nullopt );
   }

  // true if a limit is reached, which ends the search
  bool stop( void ) {
   if( ! detail::bb_limit( f_a.f_options , f_sh.evaluated.load() , f_clock ,
                           f_sh.limit , f_sh.time ) )
    return( false );
   f_sh.done.store( true , std::memory_order_release );
   return( true );
   }

  const StealingBranchAndBound & f_a;           // the search
  Shared & f_sh;                                // the shared data
  std::vector< std::unique_ptr< Worker > > & f_all;  // all the workers
  unsigned f_id;                                // the index of the worker
  const Stopwatch & f_clock;                    // the clock of the search
  alignas( cache_line ) Lock f_lock;            // protects f_q
  BBQueue< Node > f_q;                          // its open nodes
  std::atomic< std::size_t > f_size{ 0 };       // f_q.size(), read unlocked
  std::optional< Item > f_dive;                 // the node of its dive
  std::mt19937_64 f_gen;                        // the random victims
  std::vector< Item > v_kids;                   // the children of a node
  BBStats f_st;                                 // its counters
  detail::BBFrontier< Node > f_fr;              // its leaves
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const P & f_p;                                       ///< the problem
 ParallelBBOptions f_options;                         ///< the options

 };  // end( class( StealingBranchAndBound ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace PBeFS_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_StealingBranchAndBound */

/*--------------------------------------------------------------------------*/
/*------------------- End File StealingBranchAndBound.h --------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------- File ParallelBranchAndBound.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the parallel best-first searches with an incumbent, i.e.,
 * the parallel Branch-and-Bound (B&B) on a runtime (the C++ threads,
 * OpenMP or FastFlow): SharedBranchAndBound, whose workers extract the
 * nodes from one queue as the workers of SPA* do, and
 * DistributedBranchAndBound, whose workers have a queue each and send the
 * children to their owners as the workers of HDA* do. The problem, the
 * options and the Solution are those of BranchAndBound.h, and the design
 * is discussed in docs/branch-and-bound.md.
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

#ifndef __ParallelBranchAndBound
 #define __ParallelBranchAndBound
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <mutex>

#include <random>

#include "AStarSearch/BranchAndBound.h"

#include "AStarSearch/HDAStar.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ParallelBBOptions -------------------------*/
/*--------------------------------------------------------------------------*/
/// the options of the parallel B&B
/** node_limit bounds the evaluations of all the workers together; batch and
 * flush_period are used by DistributedBranchAndBound only. */

struct ParallelBBOptions : public BBOptions {

 unsigned threads = 1;          ///< the number of workers
 std::size_t batch = 16;        ///< nodes packed in a message
 unsigned flush_period = 1;     ///< evaluations between flushes of outboxes
 std::uint64_t seed = 1;        ///< seed of the owners, without hash( n )

 };  // end( struct( ParallelBBOptions ) )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BBIncumbent ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the incumbent shared by the workers of a parallel B&B
/** The value (in the minimization) is atomic, so that the workers read it
 * without a lock when they prune; an improvement takes the lock, checks the
 * value again, and stores the node and calls the callback under it. */

template< class Node >
class BBIncumbent {

 public:

 /// constructor, taking the initial value (+infinity if none)

 explicit BBIncumbent( double v ) : f_value( v ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the current value

 [[nodiscard]] double value( void ) const noexcept {
  return( f_value.load( std::memory_order_relaxed ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// offers the value v of the node n; true if it is a new incumbent

 bool improve( double v , const Node & n , const BBOptions & o , double sgn )
 {
  if( v >= value() )
   return( false );
  std::lock_guard< std::mutex > guard( f_m );
  if( v >= value() )
   return( false );
  f_value.store( v , std::memory_order_relaxed );
  f_node = n;
  if( o.on_incumbent )
   o.on_incumbent( sgn * v );
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves out the node of the incumbent, if any

 std::optional< Node > take_node( void ) { return( std::move( f_node ) ); }

 private:

 alignas( cache_line ) std::atomic< double > f_value;   ///< the value
 std::mutex f_m;                                ///< protects the rest
 std::optional< Node > f_node;                  ///< the node

 };  // end( class( BBIncumbent ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// fills the Solution of a parallel B&B from the incumbent and the least
/// bound low of the nodes pruned by the tolerance or left open

template< BranchAndBound P >
void bb_finish( BBSolution< typename P::node_type > & sol ,
                BBIncumbent< typename P::node_type > & inc , double low ,
                bool limit , bool time )
{
 constexpr double sgn = bb_sign< P >();
 constexpr double inf = std::numeric_limits< double >::infinity();
 const double z = inc.value();
 if( limit )
  sol.status = time ? BBStatus::time_limit : BBStatus::node_limit;
 else
  sol.status = std::isfinite( z ) ? BBStatus::optimal
                                  : BBStatus::infeasible;
 sol.node = inc.take_node();
 if( std::isfinite( z ) )
  sol.value = sgn * z;
 const double lb = std::min( z , low );
 sol.bound = sgn * lb;
 sol.gap = std::isfinite( z ) ? z - lb : inf;
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the index of the child that the worker goes on with, or kids.size() if
/// none: with depth-first the last child, which the queue would extract
/// next anyway, and with plunging the child of least bound, if it is close
/// enough to the best bound

template< class Item >
std::size_t bb_dive( const std::vector< Item > & kids , double best ,
                     double inc , const BBOptions & o )
{
 if( ( o.selection == Selection::depth_first ) && ! kids.empty() )
  return( kids.size() - 1 );
 if( ( o.selection != Selection::plunging ) || kids.empty() )
  return( kids.size() );
 std::size_t c = 0;
 for( std::size_t j = 1 ; j < kids.size() ; ++j )
  if( kids[ j ].bound <= kids[ c ].bound )
   c = j;
 const double b0 = std::min( best , kids[ c ].bound );
 if( ! std::isfinite( inc ) ||
     ( kids[ c ].bound <= b0 + o.plunge_gap * ( inc - b0 ) ) )
  return( c );
 return( kids.size() );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS SharedBranchAndBound -----------------------*/
/*--------------------------------------------------------------------------*/
/// the parallel B&B with one queue shared by the workers
/** The workers extract the nodes from one queue under a lock, evaluate and
 * branch them outside of it, and push the children back under it (with
 * depth-first or plunging, the child that dives is kept by the worker). The
 * search ends when the queue is empty and no worker holds a node; the open
 * nodes are dropped at once when none of them can improve the incumbent. The
 * order of extraction is close to the sequential one, and the lock is cheap
 * when the evaluation dominates, which is the usual case of a B&B. */

template< BranchAndBound P , class Runtime = ThreadRuntime >
class SharedBranchAndBound {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using Node = typename P::node_type;            ///< the node type
 using SolutionT = BBSolution< Node >;          ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the problem (which must outlive the search)

 explicit SharedBranchAndBound( const P & problem ,
                                const ParallelBBOptions & options = {} )
  : f_p( problem ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the tree, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  constexpr double sgn = detail::bb_sign< P >();
  constexpr double inf = std::numeric_limits< double >::infinity();
  const unsigned p = f_options.threads;
  const Selection sel = f_options.selection;
  Stopwatch clock;
  Shared sh( f_options.incumbent ? sgn * *f_options.incumbent : inf ,
             ( sel == Selection::depth_first ) ||
             ( sel == Selection::best_estimate ) );
  {
   Node r = f_p.root();
   const double key = detail::bb_key( f_p , r , -inf , 0 , sel );
   sh.q.push( std::move( r ) , -inf , 0 , key );
   sh.open.store( 1 );
   }

  std::vector< std::unique_ptr< Worker > > w( p );
  for( unsigned i = 0 ; i < p ; ++i )
   w[ i ] = std::make_unique< Worker >( *this , sh , clock );
  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run( i ); } );

  SolutionT sol;
  double low = sh.q.best_bound();             // left open by a limit
  for( auto & x : w ) {
   sol.stats.add( x->f_st );
   low = std::min( low , x->f_low );
   }
  sol.stats.max_open = sh.max_open;
  detail::bb_finish< P >( sol , sh.inc , low , sh.limit.load() ,
                          sh.time.load() );
  sol.stats.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 using Lock = typename Runtime::lock_type;
 using Item = typename BBQueue< Node >::Item;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers

 struct Shared {

  Shared( double z , bool track ) : inc( z ) , q( track ) {}

  BBIncumbent< Node > inc;                     // the incumbent
  alignas( cache_line ) Lock lock;              // protects q and max_open
  BBQueue< Node > q;                           // the open nodes
  std::size_t max_open = 0;                    // the most open nodes
  alignas( cache_line ) std::atomic< std::size_t > open{ 0 };  // q.size()
  alignas( cache_line ) std::atomic< unsigned > busy{ 0 };  // holding a node
  alignas( cache_line ) std::atomic< std::uint64_t > evaluated{ 0 };
  alignas( cache_line ) std::atomic< bool > done{ false };
  std::atomic< bool > limit{ false };          // stopped by a limit
  std::atomic< bool > time{ false };           // ... the time limit
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker

 struct alignas( cache_line ) Worker {

  Worker( const SharedBranchAndBound & a , Shared & s , const Stopwatch & c )
   : f_a( a ) , f_sh( s ) , f_clock( c ) {}

  // the main loop
  void run( unsigned i ) {
   auto w = detail::bb_worker( f_a.f_p , i );
   const BBOptions & o = f_a.f_options;
   const Selection sel = o.selection;
   std::optional< Item > dive;               // the node of the dive
   std::vector< Item > kids;                 // the children of a node
   const auto inc = [ & ] { return( f_sh.inc.value() ); };
   const auto improve = [ & ]( double v , const Node & n ) {
    return( f_sh.inc.improve( v , n , o , detail::bb_sign< P >() ) );
    };

   for( ;; ) {
    std::optional< Item > it;
    if( dive ) {
     it.emplace( std::move( *dive ) );
     dive.reset();
     }
    else
     if( ! ( it = next() ) )
      return;

    if( stop() ) {                     // the node stays open
     f_low = std::min( f_low , it->bound );
     f_sh.busy.fetch_sub( 1 );
     return;
     }

    kids.clear();
    detail::bb_process< P >( it->node , it->bound , it->depth , w , inc ,
                             improve , [ & ]( Node && c , double b ,
                                              unsigned d ) {
                              kids.push_back( Item{ std::move( c ) , b , d } );
                              } , o , f_st , f_low );
    f_sh.evaluated.fetch_add( 1 );

    std::lock_guard< Lock > guard( f_sh.lock );
    const std::size_t k = detail::bb_dive( kids , f_sh.q.best_bound() ,
                                           inc() , o );
    for( std::size_t j = 0 ; j < kids.size() ; ++j )
     if( j == k )
      dive.emplace( std::move( kids[ j ] ) );
     else {
      const double key = detail::bb_key( f_a.f_p , kids[ j ].node ,
                                         kids[ j ].bound , kids[ j ].depth ,
                                         sel );
      f_sh.q.push( std::move( kids[ j ].node ) , kids[ j ].bound ,
                   kids[ j ].depth , key );
      }
    f_sh.open.store( f_sh.q.size() );
    f_sh.max_open = std::max( f_sh.max_open , f_sh.q.size() );
    if( ! dive )
     f_sh.busy.fetch_sub( 1 );
    }
   }

  // the next node of the queue, counting the worker as busy; none when the
  // search is over
  std::optional< Item > next( void ) {
   Backoff backoff;
   for( ;; ) {
    if( f_sh.done.load( std::memory_order_acquire ) )
     return( std::nullopt );
    if( f_sh.open.load() == 0 ) {      // wait without the lock
     if( f_sh.busy.load() == 0 ) {
      std::lock_guard< Lock > guard( f_sh.lock );
      if( f_sh.q.empty() && ( f_sh.busy.load() == 0 ) )
       f_sh.done.store( true , std::memory_order_release );
      continue;
      }
     backoff.pause();
     continue;
     }
    std::lock_guard< Lock > guard( f_sh.lock );
    if( f_sh.q.empty() )
     continue;
    const double best = f_sh.q.best_bound();
    if( detail::bb_prunable( best , f_sh.inc.value() , f_a.f_options ) ) {
     f_st.pruned_extracted += f_sh.q.clear();     // nothing can improve
     f_low = std::min( f_low , best );
     f_sh.open.store( 0 );
     continue;
     }
    f_sh.busy.fetch_add( 1 );
    Item it = f_sh.q.pop();
    f_sh.open.store( f_sh.q.size() );
    return( it );
    }
   }

  // true if a limit is reached, which ends the search
  bool stop( void ) {
   const BBOptions & o = f_a.f_options;
   const bool nodes = o.node_limit &&
                      ( f_sh.evaluated.load() >= o.node_limit );
   const bool time = ( o.time_limit > 0 ) &&
                     ( f_clock.seconds() >= o.time_limit );
   if( ! ( nodes || time ) )
    return( false );
   if( time )
    f_sh.time.store( true );
   f_sh.limit.store( true );
   f_sh.done.store( true , std::memory_order_release );
   return( true );
   }

  const SharedBranchAndBound & f_a;             // the search
  Shared & f_sh;                                // the shared data
  const Stopwatch & f_clock;                    // the clock of the search
  BBStats f_st;                                 // its counters
  double f_low = std::numeric_limits< double >::infinity();  // pruned
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const P & f_p;                                       ///< the problem
 ParallelBBOptions f_options;                         ///< the options

 };  // end( class( SharedBranchAndBound ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS DistributedBranchAndBound ---------------------*/
/*--------------------------------------------------------------------------*/
/// the parallel B&B with a queue per worker and the children sent to
/// their owners
/** Each worker extracts the nodes from its own queue, and sends each child,
 * in batches, to the worker that owns it: hash( n ) mod p if the problem
 * gives hash( n ), a random worker otherwise (with depth-first or plunging,
 * the child that dives is kept). Since a tree has no duplicates, the owners
 * only balance the load. The incumbent is shared, and the termination is
 * detected as in HDAStar, by a counter of the nodes in flight, a counter of
 * the idle workers and an epoch [see HDAStar.h]. */

template< BranchAndBound P , class Runtime = ThreadRuntime >
class DistributedBranchAndBound {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using Node = typename P::node_type;            ///< the node type
 using SolutionT = BBSolution< Node >;          ///< the solution type

 /// a node travelling to its owner
 struct Msg {
  Node node;
  double bound;
  unsigned depth;
  };

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the problem (which must outlive the search)

 explicit DistributedBranchAndBound( const P & problem ,
                                     const ParallelBBOptions & options = {} )
  : f_p( problem ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  if( f_options.batch == 0 )
   f_options.batch = 1;
  if( f_options.flush_period == 0 )
   f_options.flush_period = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the tree, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  constexpr double sgn = detail::bb_sign< P >();
  constexpr double inf = std::numeric_limits< double >::infinity();
  const unsigned p = f_options.threads;
  Stopwatch clock;
  Shared sh( p , f_options.incumbent ? sgn * *f_options.incumbent : inf );
  {
   std::vector< Msg > b;
   b.push_back( Msg{ f_p.root() , -inf , 0 } );
   sh.pending.store( 1 );
   sh.channels.send( 0 , 0 , b );
   }

  std::vector< std::unique_ptr< Worker > > w( p );
  for( unsigned i = 0 ; i < p ; ++i )
   w[ i ] = std::make_unique< Worker >( *this , sh , i , clock );
  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run(); } );

  // the nodes left open by a limit bound the optimum too
  SolutionT sol;
  double low = inf;
  std::vector< Msg > in;
  for( unsigned i = 0 ; i < p ; ++i ) {
   Worker & x = *w[ i ];
   sol.stats.add( x.f_st );
   low = std::min( { low , x.f_low , x.f_q.best_bound() } );
   for( const auto & o : x.v_out )
    for( const Msg & m : o )
     low = std::min( low , m.bound );
   sh.channels.receive( i , in , true );
   for( const Msg & m : in )
    low = std::min( low , m.bound );
   }
  detail::bb_finish< P >( sol , sh.inc , low , sh.limit.load() ,
                          sh.time.load() );
  sol.stats.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 using Channels = typename Runtime::template channels_type< Msg >;
 using Item = typename BBQueue< Node >::Item;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers

 struct Shared {

  Shared( unsigned np , double z ) : p( np ) , channels( np ) , inc( z ) {}

  unsigned p;                                   // the number of workers
  Channels channels;                            // the inboxes
  BBIncumbent< Node > inc;                      // the incumbent
  alignas( cache_line ) std::atomic< std::int64_t > pending{ 0 };
  alignas( cache_line ) std::atomic< unsigned > idle{ 0 };
  alignas( cache_line ) std::atomic< std::uint64_t > epoch{ 0 };
  alignas( cache_line ) std::atomic< std::uint64_t > evaluated{ 0 };
  alignas( cache_line ) std::atomic< bool > done{ false };
  std::atomic< bool > limit{ false };           // stopped by a limit
  std::atomic< bool > time{ false };            // ... the time limit
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: its queue, its outboxes and its counters

 struct alignas( cache_line ) Worker {

  Worker( const DistributedBranchAndBound & a , Shared & s , unsigned i ,
          const Stopwatch & c )
   : f_a( a ) , f_sh( s ) , f_id( i ) , f_clock( c ) ,
     f_q( ( a.f_options.selection == Selection::depth_first ) ||
          ( a.f_options.selection == Selection::best_estimate ) ) ,
     f_gen( a.f_options.seed + i ) , v_out( s.p ) {}

  // the main loop
  void run( void ) {
   auto w = detail::bb_worker( f_a.f_p , f_id );
   const BBOptions & o = f_a.f_options;
   const auto inc = [ & ] { return( f_sh.inc.value() ); };
   const auto improve = [ & ]( double v , const Node & n ) {
    return( f_sh.inc.improve( v , n , o , detail::bb_sign< P >() ) );
    };
   unsigned since_flush = 0;

   while( ! f_sh.done.load( std::memory_order_acquire ) ) {
    if( std::size_t n = f_sh.channels.receive( f_id , v_in ,
                                               f_q.empty() && ! f_dive ) ) {
     for( Msg & m : v_in )
      push( Item{ std::move( m.node ) , m.bound , m.depth } );
     f_sh.pending.fetch_sub( std::int64_t( n ) );
     }

    if( std::optional< Item > it = next() ) {
     if( stop() ) {                   // the node stays open
      f_low = std::min( f_low , it->bound );
      return;
      }
     v_kids.clear();
     detail::bb_process< P >( it->node , it->bound , it->depth , w , inc ,
                              improve , [ & ]( Node && c , double b ,
                                               unsigned d ) {
                               v_kids.push_back( Item{ std::move( c ) , b ,
                                                       d } );
                               } , o , f_st , f_low );
     f_sh.evaluated.fetch_add( 1 );
     const std::size_t k = detail::bb_dive( v_kids , f_q.best_bound() ,
                                            inc() , o );
     for( std::size_t j = 0 ; j < v_kids.size() ; ++j )
      if( j == k )
       f_dive.emplace( std::move( v_kids[ j ] ) );
      else
       send( std::move( v_kids[ j ] ) );
     if( ++since_flush >= f_a.f_options.flush_period ) {
      since_flush = 0;
      flush( false );
      }
     continue;
     }

    flush( true );           // nothing to evaluate: send everything, then
    wait();                  // wait for mail or for the end
    }
   }

  // pushes a node in the queue
  void push( Item && it ) {
   const double key = detail::bb_key( f_a.f_p , it.node , it.bound ,
                                      it.depth , f_a.f_options.selection );
   f_q.push( std::move( it.node ) , it.bound , it.depth , key );
   f_st.max_open = std::max( f_st.max_open , f_q.size() );
   }

  // the next node: the dive, or the best of the queue; none if there is
  // none, or if none can improve the incumbent (which empties the queue)
  std::optional< Item > next( void ) {
   if( f_dive ) {
    std::optional< Item > it( std::move( f_dive ) );
    f_dive.reset();
    return( it );
    }
   if( f_q.empty() )
    return( std::nullopt );
   const double best = f_q.best_bound();
   if( detail::bb_prunable( best , f_sh.inc.value() , f_a.f_options ) ) {
    f_st.pruned_extracted += f_q.clear();
    f_low = std::min( f_low , best );
    return( std::nullopt );
    }
   return( f_q.pop() );
   }

  // sends a child to its owner, or keeps it if the worker owns it
  void send( Item && it ) {
   const unsigned p = f_sh.p;
   unsigned dst;
   if constexpr( HashedBranchAndBound< P > )
    dst = unsigned( std::uint64_t( f_a.f_p.hash( it.node ) ) % p );
   else
    dst = unsigned( random_below( f_gen , p ) );
   if( dst == f_id ) {
    push( std::move( it ) );
    return;
    }
   auto & o = v_out[ dst ];
   o.push_back( Msg{ std::move( it.node ) , it.bound , it.depth } );
   ++f_st.sent;
   if( o.size() >= f_a.f_options.batch )
    post( dst , false );
   }

  // sends the outbox of dst, waiting for the lock if block; true if sent
  bool post( unsigned dst , bool block ) {
   auto & o = v_out[ dst ];
   const auto k = std::int64_t( o.size() );
   f_sh.pending.fetch_add( k );
   if( block )
    f_sh.channels.send( f_id , dst , o );
   else
    if( ! f_sh.channels.try_send( f_id , dst , o ) ) {
     f_sh.pending.fetch_sub( k );
     return( false );
     }
   ++f_st.messages;
   return( true );
   }

  // sends the non-empty outboxes
  void flush( bool block ) {
   for( unsigned d = 0 ; d < f_sh.p ; ++d )
    if( ! v_out[ d ].empty() )
     post( d , block );
   }

  // waits until mail arrives or the search ends
  void wait( void ) {
   f_sh.idle.fetch_add( 1 );
   Backoff backoff;
   for( ;; ) {
    if( f_sh.done.load( std::memory_order_acquire ) )
     return;
    if( f_sh.channels.has_mail( f_id ) ) {
     f_sh.idle.fetch_sub( 1 );     // first leave idle, then change epoch
     f_sh.epoch.fetch_add( 1 );
     return;
     }
    const auto e = f_sh.epoch.load();
    if( ( f_sh.idle.load() == f_sh.p ) && ( f_sh.pending.load() == 0 ) &&
        ( f_sh.epoch.load() == e ) ) {
     f_sh.done.store( true , std::memory_order_release );
     return;
     }
    backoff.pause();
    }
   }

  // true if a limit is reached, which ends the search
  bool stop( void ) {
   const BBOptions & o = f_a.f_options;
   const bool nodes = o.node_limit &&
                      ( f_sh.evaluated.load() >= o.node_limit );
   const bool time = ( o.time_limit > 0 ) &&
                     ( f_clock.seconds() >= o.time_limit );
   if( ! ( nodes || time ) )
    return( false );
   if( time )
    f_sh.time.store( true );
   f_sh.limit.store( true );
   f_sh.done.store( true , std::memory_order_release );
   return( true );
   }

  const DistributedBranchAndBound & f_a;        // the search
  Shared & f_sh;                                // the shared data
  unsigned f_id;                                // the index of the worker
  const Stopwatch & f_clock;                    // the clock of the search
  BBQueue< Node > f_q;                          // its open nodes
  std::optional< Item > f_dive;                 // the node of its dive
  std::mt19937_64 f_gen;                        // the random owners
  std::vector< std::vector< Msg > > v_out;      // its outboxes
  std::vector< Msg > v_in;                      // the last inbox received
  std::vector< Item > v_kids;                   // the children of a node
  BBStats f_st;                                 // its counters
  double f_low = std::numeric_limits< double >::infinity();  // pruned
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const P & f_p;                                       ///< the problem
 ParallelBBOptions f_options;                         ///< the options

 };  // end( class( DistributedBranchAndBound ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __ParallelBranchAndBound */

/*--------------------------------------------------------------------------*/
/*------------------- End File ParallelBranchAndBound.h --------------------*/
/*--------------------------------------------------------------------------*/

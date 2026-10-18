/*--------------------------------------------------------------------------*/
/*----------------------- File FarmBranchAndBound.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for FarmBranchAndBound, the master-worker Branch-and-Bound
 * (B&B) on a farm of FastFlow with feedback: the master holds the queue of
 * the open nodes, and the workers evaluate and branch the nodes it sends.
 * The problem, the options and the Solution are those of BranchAndBound.h
 * and ParallelBranchAndBound.h, and the design is discussed in
 * docs/branch-and-bound.md.
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

#ifndef __FarmBranchAndBound
 #define __FarmBranchAndBound
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <stdexcept>

#include <vector>

#include <ff/ff.hpp>

#include "AStarSearch/ParallelBranchAndBound.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS FarmBranchAndBound ------------------------*/
/*--------------------------------------------------------------------------*/
/// the master-worker B&B on a FastFlow farm with feedback
/** The master extracts the nodes from its queue by the Selection and sends
 * them, batch at a time, to the workers that are free; a worker evaluates
 * and branches each node, offering its feasible value to the shared
 * incumbent, and sends back the children that survive the pruning (with
 * depth-first or plunging, it goes on with the child that dives in the same
 * task). The search ends when the queue is empty, or no node of it can
 * improve the incumbent, and no task is out at a worker. Since the master
 * only moves nodes, the order of extraction is the one of the selection, and
 * the scheme suits an evaluation that is expensive w.r.t. the transfer of a
 * node.
 *
 * The farm runs threads + 1 threads: with ASTAR_PIN the master takes the
 * first CPU of the affinity mask and the i-th worker the ( i + 1 )-th [see
 * pin_worker()], and with fewer CPUs than threads + 1 the threads block on
 * their queues instead of spinning. */

template< BranchAndBound P >
class FarmBranchAndBound {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using Node = typename P::node_type;            ///< the node type
 using SolutionT = BBSolution< Node >;          ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the problem (which must outlive the search);
 /// threads is the number of workers, batch the nodes per task

 explicit FarmBranchAndBound( const P & problem ,
                              const ParallelBBOptions & options = {} )
  : f_p( problem ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  if( f_options.batch == 0 )
   f_options.batch = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the tree, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  constexpr double sgn = detail::bb_sign< P >();
  constexpr double inf = std::numeric_limits< double >::infinity();
  Stopwatch clock;
  Shared sh( f_options.incumbent ? sgn * *f_options.incumbent : inf ,
             clock );
  Master master( *this , sh );
  std::vector< std::unique_ptr< ff::ff_node > > workers;
  std::vector< Worker * > w;
  for( unsigned i = 0 ; i < f_options.threads ; ++i ) {
   auto x = std::make_unique< Worker >( *this , sh , i );
   w.push_back( x.get() );
   workers.push_back( std::move( x ) );
   }
  ff::ff_Farm< Task > farm( std::move( workers ) , master );
  farm.remove_collector();
  farm.wrap_around();
  farm.set_scheduling_ondemand();
  // with fewer CPUs than master + workers, a spinning thread may hold the
  // CPU that the one it waits for needs: the threads have to block instead
  const unsigned cpus = available_cpus();
  if( cpus && ( cpus < f_options.threads + 1 ) )
   farm.blocking_mode( true );
  if( farm.run_and_wait_end() < 0 )
   throw( std::runtime_error( "FarmBranchAndBound::solve: the farm "
                              "failed" ) );

  SolutionT sol;
  double low = std::min( master.f_low , master.f_q.best_bound() );
  for( Worker * x : w ) {
   sol.stats.add( x->f_st );
   low = std::min( low , x->f_low );
   }
  sol.stats.pruned_extracted += master.f_st.pruned_extracted;
  sol.stats.max_open = master.f_st.max_open;
  sol.stats.messages = master.f_st.messages;
  detail::bb_finish< P >( sol , sh.inc , low , sh.limit.load() ,
                          sh.time.load() );
  sol.stats.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 using Item = typename BBQueue< Node >::Item;

 // a task: the nodes to evaluate, and then the children that survive
 struct Task {
  std::vector< Item > nodes;                    // the nodes
  std::vector< Item > kids;                     // their children
  double best = 0;                              // the best open bound
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the master and the workers

 struct Shared {

  Shared( double z , const Stopwatch & c ) : inc( z ) , clock( c ) {}

  BBIncumbent< Node > inc;                      // the incumbent
  const Stopwatch & clock;                      // the clock of the search
  alignas( cache_line ) std::atomic< std::uint64_t > evaluated{ 0 };
  std::atomic< bool > limit{ false };           // stopped by a limit
  std::atomic< bool > time{ false };            // ... the time limit
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the master: the queue of the open nodes

 struct Master : ff::ff_monode_t< Task > {

  Master( const FarmBranchAndBound & a , Shared & s )
   : f_a( a ) , f_sh( s ) ,
     f_q( ( a.f_options.selection == Selection::depth_first ) ||
          ( a.f_options.selection == Selection::best_estimate ) ) {}

  int svc_init( void ) override {
   pin_worker( 0 );
   return( 0 );
   }

  Task * svc( Task * t ) override {
   if( ! t )                                     // the start: the root
    push( Item{ f_a.f_p.root() , - std::numeric_limits< double >::infinity() ,
                0 } );
   else {                                        // children back
    --f_out;
    for( Item & it : t->kids )
     push( std::move( it ) );
    t->kids.clear();
    v_free.push_back( t );
    }
   check_limits();
   dispatch();
   if( ( f_out == 0 ) && ( f_sh.limit.load() || f_q.empty() ) )
    return( this->EOS );            // nothing out, nothing left to send
   return( this->GO_ON );
   }

  void svc_end( void ) override {
   for( Task * t : v_free )
    delete t;
   }

  // pushes a node in the queue
  void push( Item && it ) {
   const double key = detail::bb_key( f_a.f_p , it.node , it.bound ,
                                      it.depth , f_a.f_options.selection );
   f_q.push( std::move( it.node ) , it.bound , it.depth , key );
   f_st.max_open = std::max( f_st.max_open , f_q.size() );
   }

  // stops the dispatch if a limit is reached
  void check_limits( void ) {
   const BBOptions & o = f_a.f_options;
   const bool nodes = o.node_limit &&
                      ( f_sh.evaluated.load() >= o.node_limit );
   const bool time = ( o.time_limit > 0 ) &&
                     ( f_sh.clock.seconds() >= o.time_limit );
   if( time )
    f_sh.time.store( true );
   if( nodes || time )
    f_sh.limit.store( true );
   }

  // sends tasks while there are nodes and free workers
  void dispatch( void ) {
   while( ! f_sh.limit.load() && ( f_out < f_a.f_options.threads ) &&
          ! f_q.empty() ) {
    const double best = f_q.best_bound();
    if( detail::bb_prunable( best , f_sh.inc.value() , f_a.f_options ) ) {
     f_st.pruned_extracted += f_q.clear();      // nothing can improve
     f_low = std::min( f_low , best );
     return;
     }
    Task * t;
    if( v_free.empty() )
     t = new Task;
    else {
     t = v_free.back();
     v_free.pop_back();
     }
    t->nodes.clear();
    while( ! f_q.empty() && ( t->nodes.size() < f_a.f_options.batch ) )
     t->nodes.push_back( f_q.pop() );
    t->best = std::min( best , f_q.best_bound() );
    ++f_out;
    ++f_st.messages;
    this->ff_send_out( t );
    }
   }

  const FarmBranchAndBound & f_a;               // the search
  Shared & f_sh;                                // the shared data
  BBQueue< Node > f_q;                          // the open nodes
  std::vector< Task * > v_free;                 // tasks to reuse
  unsigned f_out = 0;                           // tasks at the workers
  BBStats f_st;                                 // its counters
  double f_low = std::numeric_limits< double >::infinity();  // pruned
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: evaluates and branches the nodes of a task

 struct Worker : ff::ff_node_t< Task > {

  Worker( const FarmBranchAndBound & a , Shared & s , unsigned i )
   : f_a( a ) , f_sh( s ) , f_id( i ) {}

  int svc_init( void ) override {
   pin_worker( f_id + 1 );
   return( 0 );
   }

  Task * svc( Task * t ) override {
   if( ! f_w )
    f_w.emplace( detail::bb_worker( f_a.f_p , f_id ) );
   const BBOptions & o = f_a.f_options;
   const auto inc = [ & ] { return( f_sh.inc.value() ); };
   const auto improve = [ & ]( double v , const Node & n ) {
    return( f_sh.inc.improve( v , n , o , detail::bb_sign< P >() ) );
    };
   for( Item & first : t->nodes ) {
    std::optional< Item > it( std::move( first ) );
    while( it ) {                  // the node, then the child that dives
     v_kids.clear();
     detail::bb_process< P >( it->node , it->bound , it->depth , *f_w , inc ,
                              improve , [ & ]( Node && c , double b ,
                                               unsigned d ) {
                               v_kids.push_back( Item{ std::move( c ) , b ,
                                                       d } );
                               } , o , f_st , f_low );
     f_sh.evaluated.fetch_add( 1 );
     it.reset();
     const std::size_t k = detail::bb_dive( v_kids , t->best , inc() , o );
     for( std::size_t j = 0 ; j < v_kids.size() ; ++j )
      if( j == k )
       it.emplace( std::move( v_kids[ j ] ) );
      else
       t->kids.push_back( std::move( v_kids[ j ] ) );
     if( it && o.node_limit &&
         ( f_sh.evaluated.load() >= o.node_limit ) ) {
      t->kids.push_back( std::move( *it ) );     // the dive stops here
      it.reset();
      }
     }
    }
   t->nodes.clear();
   return( t );
   }

  using Evaluator = decltype( detail::bb_worker( std::declval< const P & >() ,
                                                 0u ) );

  const FarmBranchAndBound & f_a;               // the search
  Shared & f_sh;                                // the shared data
  unsigned f_id;                                // the index of the worker
  std::optional< Evaluator > f_w;               // its evaluator
  std::vector< Item > v_kids;                   // the children of a node
  BBStats f_st;                                 // its counters
  double f_low = std::numeric_limits< double >::infinity();  // pruned
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const P & f_p;                                       ///< the problem
 ParallelBBOptions f_options;                         ///< the options

 };  // end( class( FarmBranchAndBound ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __FarmBranchAndBound */

/*--------------------------------------------------------------------------*/
/*--------------------- End File FarmBranchAndBound.h ----------------------*/
/*--------------------------------------------------------------------------*/

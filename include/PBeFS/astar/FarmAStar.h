/*--------------------------------------------------------------------------*/
/*---------------------------- File FarmAStar.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template FarmAStar, a centralized parallel A*
 * written as a FastFlow farm with feedback: a master holds OPEN and CLOSED,
 * and the workers expand the nodes it sends them.
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

#ifndef __PBeFS_FarmAStar
 #define __PBeFS_FarmAStar
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <memory>

#include <stdexcept>

#include <tuple>

#include <type_traits>

#include <vector>

#include <ff/ff.hpp>

#include "PBeFS/astar/HDAStar.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Parallel Best-First Search library
namespace PBeFS_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS FarmAStar -----------------------------*/
/*--------------------------------------------------------------------------*/
/// a master-worker parallel A* on a FastFlow farm with feedback
/** The emitter of the farm is the master: it alone owns OPEN, CLOSED and
 * the nodes, so it needs no lock. It sends the best nodes of OPEN, batch at
 * a time, to the workers that are free (the farm schedules on demand); a
 * worker generates the successors of its nodes with their costs and
 * heuristic values and sends them back on the feedback channel, and the
 * master checks them against CLOSED and puts the new and improved ones in
 * OPEN. It is the centralized approach of Kumar, Ramesh and Rao (1988), the
 * same as SPAStar but with the master in place of the locks, close to the
 * K-best-first search of Felner, Kraus and Korf (2003) without its
 * synchronization at each round. A node may be expanded before its g is
 * optimal, and then re-expanded; the incumbent prunes, and the search ends
 * when no node of OPEN has f below the incumbent and no node is out at a
 * worker. Since the master does all the bookkeeping, FarmAStar pays only
 * when the expansions are slow.
 *
 * The farm runs threads + 1 threads: with PBEFS_PIN the master takes the
 * first CPU of the affinity mask and the i-th worker the ( i + 1 )-th [see
 * pin_worker()], and with fewer CPUs than threads + 1 the threads block on
 * their queues instead of spinning. */

template< Domain D ,
          class Open = std::conditional_t<
           std::is_integral_v< cost_t< D > > ,
           BucketOpenList< Node< state_t< D > , cost_t< D > > > ,
           HeapOpenList< Node< state_t< D > , cost_t< D > > > > >
class FarmAStar {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using State = state_t< D >;                       ///< the state type
 using Cost = cost_t< D >;                         ///< the cost type
 using NodeT = Node< State , Cost >;               ///< the node type
 using SolutionT = Solution< State , Cost >;       ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the Domain (which must outlive the FarmAStar);
 /// threads is the number of workers, batch the nodes per task

 explicit FarmAStar( const D & domain , const ParallelOptions & options = {} )
  : f_domain( domain ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  if( f_options.batch == 0 )
   f_options.batch = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the Domain, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  Stopwatch clock;
  Master master( *this );
  std::vector< std::unique_ptr< ff::ff_node > > workers;
  for( unsigned i = 0 ; i < f_options.threads ; ++i )
   workers.push_back( std::make_unique< Worker >( f_domain , i + 1 ) );
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
   throw( std::runtime_error( "FarmAStar::solve: the farm failed" ) );

  SolutionT sol;
  sol.stats = master.f_st;
  sol.stats.stored = master.f_pool.size();
  sol.stats.expanded_by = { sol.stats.expanded };
  sol.stats.stored_by = { sol.stats.stored };
  if( master.f_limit_hit )
   sol.status = Status::node_limit;
  else
   if( master.f_goal ) {
    sol.status = Status::solved;
    sol.cost = master.f_goal->g;
    sol.path = trace_path( master.f_goal );
    }
  sol.stats.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 // a task: nodes to expand, and then their successors
 struct Task {
  std::vector< NodeT * > nodes;                           // the nodes
  std::vector< std::tuple< std::size_t , State , Cost , Cost > > succ;
                         // per successor: its node, state, edge cost, h
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the master: OPEN, CLOSED, the nodes and the counters

 struct Master : ff::ff_monode_t< Task > {

  explicit Master( const FarmAStar & a ) : f_a( a ) {}

  int svc_init( void ) override {
   pin_worker( 0 );
   return( 0 );
   }

  Task * svc( Task * t ) override {
   if( ! t ) {                                   // the start: the root
    const State s0 = f_a.f_domain.initial_state();
    insert( s0 , Cost( 0 ) , Cost( f_a.f_domain.heuristic( s0 ) ) ,
            nullptr , f_a.f_domain.hash( s0 ) );
    }
   else {                                        // successors back
    --f_out;
    v_hash.resize( t->succ.size() );       // prefetch the batch first
    for( std::size_t i = 0 ; i < t->succ.size() ; ++i ) {
     v_hash[ i ] = f_a.f_domain.hash( std::get< 1 >( t->succ[ i ] ) );
     f_table.prefetch( v_hash[ i ] );
     }
    for( std::size_t i = 0 ; i < t->succ.size() ; ++i ) {
     const auto & [ k , s , c , h ] = t->succ[ i ];
     ++f_st.generated;
     NodeT * parent = t->nodes[ k ];
     const Cost g = parent->g + c;
     if( g + h < f_incumbent )
      insert( s , g , h , parent , v_hash[ i ] );
     }
    if( f_a.f_options.node_limit &&
        ( f_pool.size() >= f_a.f_options.node_limit ) )
     f_limit_hit = true;
    v_free.push_back( t );
    }
   dispatch();
   if( ( f_out == 0 ) && ( f_limit_hit || ! has_work() ) )
    return( this->EOS );            // nothing out, nothing left to send
   return( this->GO_ON );
   }

  void svc_end( void ) override {
   for( Task * t : v_free )
    delete t;
   }

  // sends tasks while there are nodes and free workers
  void dispatch( void ) {
   while( ! f_limit_hit && ( f_out < f_a.f_options.threads ) ) {
    Task * t = nullptr;
    while( NodeT * n = next() ) {
     n->closed = true;
     ++f_st.expanded;
     if( n->expanded )
      ++f_st.reexpanded;
     n->expanded = true;
     if( f_a.f_domain.is_goal( n->state ) ) {
      if( n->g < f_incumbent ) {
       f_incumbent = n->g;
       f_goal = n;
       }
      continue;
      }
     if( ! t ) {
      if( v_free.empty() )
       t = new Task;
      else {
       t = v_free.back();
       v_free.pop_back();
       }
      t->nodes.clear();
      t->succ.clear();
      }
     t->nodes.push_back( n );
     if( t->nodes.size() >= f_a.f_options.batch )
      break;
     }
    if( ! t )
     return;
    ++f_out;
    this->ff_send_out( t );
    }
   }

  // true if OPEN has a node with f below the incumbent
  bool has_work( void ) {
   while( ! f_open.empty() && f_open.top().stale() )
    f_open.pop();
   return( ! f_open.empty() && ( f_open.top_f() < f_incumbent ) );
   }

  // the best node of OPEN with f below the incumbent, removed, or nullptr
  NodeT * next( void ) {
   return( has_work() ? f_open.pop().node : nullptr );
   }

  // the path to s, whose hash is hash, of cost g through parent
  void insert( const State & s , Cost g , Cost h , NodeT * parent ,
               std::uint64_t hash ) {
   auto [ x , fresh ] = f_table.find_or_insert( s , hash , [ & ] {
    NodeT * y = f_pool.allocate();
    *y = NodeT{ s , g , h , parent , false , false };
    return( y );
    } );
   if( fresh )
    f_open.push( x );
   else
    if( g < x->g ) {
     x->g = g;
     x->parent = parent;
     x->closed = false;
     f_open.push( x );
     }
    else
     ++f_st.duplicates;
   }

  const FarmAStar & f_a;                        // the search
  NodePool< NodeT > f_pool;                     // the nodes
  NodeTable< NodeT > f_table;                   // CLOSED
  Open f_open;                                  // OPEN
  std::vector< Task * > v_free;                 // tasks to reuse
  std::vector< std::uint64_t > v_hash;          // hashes of a task back
  unsigned f_out = 0;                           // tasks at the workers
  Cost f_incumbent = infinite_cost< Cost >();   // the best cost found
  NodeT * f_goal = nullptr;                     // the best goal found
  bool f_limit_hit = false;                     // the node limit is hit
  SearchStats f_st;                             // the counters
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: expands the nodes of a task

 struct Worker : ff::ff_node_t< Task > {

  Worker( const D & d , unsigned cpu ) : f_d( d ) , f_cpu( cpu ) {}

  int svc_init( void ) override {
   pin_worker( f_cpu );
   return( 0 );
   }

  Task * svc( Task * t ) override {
   for( std::size_t k = 0 ; k < t->nodes.size() ; ++k ) {
    const NodeT * n = t->nodes[ k ];
    f_d.for_each_successor( n->state , n->h ,
                            [ & ]( const State & s , Cost c , Cost h ) {
     t->succ.emplace_back( k , s , c , h );
     } );
    }
   return( t );
   }

  const D & f_d;                                // the Domain
  unsigned f_cpu;                               // its CPU for pin_worker()
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const D & f_domain;                                   ///< the Domain
 ParallelOptions f_options;                            ///< the options

 };  // end( class( FarmAStar ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace PBeFS_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_FarmAStar */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File FarmAStar.h --------------------------*/
/*--------------------------------------------------------------------------*/

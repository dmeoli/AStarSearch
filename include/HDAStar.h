/*--------------------------------------------------------------------------*/
/*----------------------------- File HDAStar.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template HDAStar, Hash Distributed A* on a
 * shared-memory machine, and for ThreadRuntime, the runtime that runs its
 * workers as C++ threads exchanging nodes through mutex-protected inboxes.
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

#ifndef __HDAStar
 #define __HDAStar
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <mutex>

#include <string>

#include <thread>

#include <type_traits>

#include <vector>

#include "AStar.h"

#include "Distribution.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MutexChannels ---------------------------*/
/*--------------------------------------------------------------------------*/
/// one inbox per worker, protected by a mutex
/** The shared-memory implementation of hash-based work distribution of
 * Burns et al. (JAIR 2010): a sender tries to lock the inbox of the owner
 * and, if the lock is taken, keeps the nodes in its own outbox and tries
 * again later, so that no sender ever waits; the receiver swaps the whole
 * inbox with an empty vector, and waits for the lock only when it has
 * nothing else to do. */

template< class Msg >
class MutexChannels {

 public:

 /// constructor, for p workers

 explicit MutexChannels( unsigned p ) : v_in( new Inbox[ p ] ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves b into the inbox of dst if its lock is free; true if done

 bool try_send( unsigned , unsigned dst , std::vector< Msg > & b ) {
  Inbox & in = v_in[ dst ];
  if( ! in.m.try_lock() )
   return( false );
  put( in , b );
  in.m.unlock();
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves b into the inbox of dst, waiting for its lock

 void send( unsigned , unsigned dst , std::vector< Msg > & b ) {
  Inbox & in = v_in[ dst ];
  in.m.lock();
  put( in , b );
  in.m.unlock();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// true if the inbox of dst is not empty

 [[nodiscard]] bool has_mail( unsigned dst ) const noexcept {
  return( v_in[ dst ].mail.load( std::memory_order_acquire ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves the inbox of dst into out (cleared), waiting for the lock if block

 std::size_t receive( unsigned dst , std::vector< Msg > & out , bool block )
 {
  out.clear();
  Inbox & in = v_in[ dst ];
  if( ! in.mail.load( std::memory_order_acquire ) )
   return( 0 );
  if( block )
   in.m.lock();
  else
   if( ! in.m.try_lock() )
    return( 0 );
  out.swap( in.q );
  in.mail.store( false , std::memory_order_relaxed );
  in.m.unlock();
  return( out.size() );
  }

 private:

 /// an inbox, alone in its cache lines
 struct alignas( cache_line ) Inbox {
  std::mutex m;
  std::vector< Msg > q;
  std::atomic< bool > mail{ false };
  };

 // appends b to the inbox (locked by the caller) and empties b
 static void put( Inbox & in , std::vector< Msg > & b ) {
  if( in.q.empty() )
   in.q.swap( b );
  else
   in.q.insert( in.q.end() , b.begin() , b.end() );
  b.clear();
  in.mail.store( true , std::memory_order_release );
  }

 std::unique_ptr< Inbox[] > v_in;                      ///< the inboxes

 };  // end( class( MutexChannels ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ThreadRuntime ---------------------------*/
/*--------------------------------------------------------------------------*/
/// the runtime of the C++ threads
/** A runtime provides the channels between the workers, a lock type with
 * lock(), try_lock() and unlock(), and run( p , f ), which calls f( i ) for
 * i = 0 , ... , p - 1 concurrently and returns when all the calls have
 * returned. */

struct ThreadRuntime {

 /// the channels of this runtime
 template< class Msg >
 using channels_type = MutexChannels< Msg >;

 /// the lock of this runtime
 using lock_type = std::mutex;

 /// runs f( 0 ) , ... , f( p - 1 ), each on a new thread
 template< class F >
 static void run( unsigned p , F && f ) {
  std::vector< std::thread > t;          // the caller is not pinned
  t.reserve( p );
  for( unsigned i = 0 ; i < p ; ++i )
   t.emplace_back( [ & f ]( unsigned k ) {
    pin_worker( k );
    f( k );
    } , i );
  for( auto & x : t )
   x.join();
  }

 /// the name of the runtime
 [[nodiscard]] static std::string name( void ) { return( "thread" ); }

 };  // end( struct( ThreadRuntime ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ParallelOptions --------------------------*/
/*--------------------------------------------------------------------------*/
/// the options of the parallel searches

struct ParallelOptions : public SearchOptions {

 unsigned threads = 1;          ///< the number of workers
 std::size_t batch = 64;        ///< nodes packed in a message
 unsigned flush_period = 64;    ///< expansions between flushes of outboxes
 std::uint64_t seed = 1;        ///< seed of the work distribution

 };  // end( struct( ParallelOptions ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS HDAStar ------------------------------*/
/*--------------------------------------------------------------------------*/
/// Hash Distributed A* on a shared-memory machine
/** Every worker runs A* on its own OPEN and CLOSED lists, and a global
 * function Dist assigns every state to the worker that owns it: a node
 * generated by a worker is sent to its owner, which checks it against its
 * closed list and possibly adds it to its OPEN list [see section 4.1 of
 * Fukunaga et al., 2017, and Kishimoto, Fukunaga and Botea, AIJ 2013].
 * Nodes are sent asynchronously, packed in batches of batch nodes, and an
 * outbox not yet full is flushed every flush_period expansions and before
 * the worker waits. The goal test is done at the expansion, the cost of
 * the best goal found (the incumbent) prunes every node with f not
 * smaller, and the search ends when no worker has a node with f smaller
 * than the incumbent and no node is travelling.
 *
 * The termination is detected with three shared counters: pending, the
 * nodes sent and not yet processed (raised before a batch becomes visible,
 * lowered after it has been processed), idle, the workers waiting, and
 * epoch, raised by a worker that stops waiting after it has left idle. A
 * waiting worker declares the end when epoch does not change while it
 * reads idle == p and pending == 0: at the moment idle was read nobody
 * had work, and no message could be in transit, since consuming it would
 * have required somebody to stop waiting, i.e., to change epoch.
 *
 * The node of a state belongs to its owner, which is the only worker that
 * writes it; parent pointers cross the workers, and they are followed only
 * after all the workers have finished. */

template< Domain D , class Dist = StateHashDistribution< D > ,
          class Runtime = ThreadRuntime ,
          class Open = std::conditional_t<
           std::is_integral_v< cost_t< D > > ,
           BucketOpenList< Node< state_t< D > , cost_t< D > > > ,
           HeapOpenList< Node< state_t< D > , cost_t< D > > > > >
class HDAStar {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using State = state_t< D >;                       ///< the state type
 using Cost = cost_t< D >;                         ///< the cost type
 using NodeT = Node< State , Cost >;               ///< the node type
 using SolutionT = Solution< State , Cost >;       ///< the solution type

 /// a node travelling to its owner
 struct Msg {
  State s;
  Cost g;
  Cost h;
  NodeT * parent;
  };

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the Domain and the distribution (which must
 /// outlive the HDAStar)

 HDAStar( const D & domain , const Dist & dist ,
          const ParallelOptions & options = {} )
  : f_domain( domain ) , f_dist( dist ) , f_options( options ) {
  if( f_options.threads == 0 )
   f_options.threads = 1;
  if( f_options.batch == 0 )
   f_options.batch = 1;
  if( f_options.flush_period == 0 )
   f_options.flush_period = 1;
  }

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the Domain, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  const unsigned p = f_options.threads;
  Stopwatch clock;
  Shared sh( p );

  // the root goes to its owner as the first message
  {
   const State s0 = f_domain.initial_state();
   std::vector< Msg > b{ Msg{ s0 , Cost( 0 ) ,
                              Cost( f_domain.heuristic( s0 ) ) , nullptr } };
   sh.pending.store( 1 );
   sh.channels.send( 0 , f_dist.owner( s0 , p ) , b );
   }

  std::vector< std::unique_ptr< Worker > > w( p );
  for( unsigned i = 0 ; i < p ; ++i )
   w[ i ] = std::make_unique< Worker >( *this , sh , i );

  Runtime::run( p , [ & ]( unsigned i ) { w[ i ]->run(); } );

  SolutionT sol;
  SearchStats & st = sol.stats;
  for( auto & x : w ) {
   st.expanded += x->f_st.expanded;
   st.reexpanded += x->f_st.reexpanded;
   st.generated += x->f_st.generated;
   st.duplicates += x->f_st.duplicates;
   st.sent += x->f_st.sent;
   st.messages += x->f_st.messages;
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

 using Channels = typename Runtime::template channels_type< Msg >;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the data shared by the workers

 struct Shared {

  explicit Shared( unsigned np ) : p( np ) , channels( np ) {}

  unsigned p;                                   // the number of workers
  Channels channels;                            // the inboxes
  alignas( cache_line ) std::atomic< Cost > incumbent{
                                                   infinite_cost< Cost >() };
  alignas( cache_line ) std::atomic< std::int64_t > pending{ 0 };
  alignas( cache_line ) std::atomic< unsigned > idle{ 0 };
  alignas( cache_line ) std::atomic< std::uint64_t > epoch{ 0 };
  alignas( cache_line ) std::atomic< bool > done{ false };
  std::atomic< bool > limit_hit{ false };
  std::mutex goal_mutex;                        // protects goal
  NodeT * goal = nullptr;                       // the best goal found
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // a worker: its OPEN and CLOSED lists, its outboxes and its counters

 struct alignas( cache_line ) Worker {

  Worker( const HDAStar & a , Shared & s , unsigned i )
   : f_a( a ) , f_sh( s ) , f_id( i ) , f_dist( a.f_dist ) ,
     v_out( s.p ) {
   for( auto & o : v_out )
    o.reserve( a.f_options.batch );
   }

  // the main loop
  void run( void ) {
   unsigned since_flush = 0;
   while( ! f_sh.done.load( std::memory_order_acquire ) ) {
    if( std::size_t n = f_sh.channels.receive( f_id , v_in ,
                                               f_open.empty() ) ) {
     // the hashes and the prefetches of the whole batch first, so that
     // the cache misses of the lookups overlap
     v_hash.resize( n );
     for( std::size_t k = 0 ; k < n ; ++k ) {
      v_hash[ k ] = f_a.f_domain.hash( v_in[ k ].s );
      f_table.prefetch( v_hash[ k ] );
      }
     for( std::size_t k = 0 ; k < n ; ++k )
      insert( v_in[ k ] , v_hash[ k ] );
     f_sh.pending.fetch_sub( std::int64_t( n ) );
     }

    if( NodeT * n = next() ) {
     expand( n );
     if( ++since_flush >= f_a.f_options.flush_period ) {
      since_flush = 0;
      flush( false );
      }
     if( f_a.f_options.node_limit &&
         ( f_pool.size() >= f_a.f_options.node_limit ) ) {
      f_sh.limit_hit.store( true );
      f_sh.done.store( true , std::memory_order_release );
      }
     continue;
     }

    flush( true );           // nothing to expand: send everything, then
    wait();                  // wait for mail or for the end
    }
   }

  // the best node of OPEN with f < incumbent, removed; nullptr if none
  NodeT * next( void ) {
   while( ! f_open.empty() ) {
    const OpenEntry< NodeT > e = f_open.top();
    if( e.stale() ) {
     f_open.pop();
     continue;
     }
    if( e.node->g + e.node->h >= f_sh.incumbent.load(
                                                std::memory_order_relaxed ) )
     return( nullptr );
    f_open.pop();
    return( e.node );
    }
   return( nullptr );
   }

  // expands n, or records it as the incumbent if it is a goal
  void expand( NodeT * n ) {
   n->closed = true;
   ++f_st.expanded;
   if( n->expanded )
    ++f_st.reexpanded;
   n->expanded = true;

   if( f_a.f_domain.is_goal( n->state ) ) {
    std::lock_guard< std::mutex > lock( f_sh.goal_mutex );
    if( n->g < f_sh.incumbent.load() ) {
     f_sh.incumbent.store( n->g );
     f_sh.goal = n;
     }
    return;
    }

   const unsigned p = f_sh.p;
   f_a.f_domain.for_each_successor( n->state , n->h ,
                                    [ & ]( const State & t , Cost c ,
                                           Cost ht ) {
    ++f_st.generated;
    const Cost g = n->g + c;
    if( g + ht >= f_sh.incumbent.load( std::memory_order_relaxed ) )
     return;
    const unsigned dst = f_dist.owner( t , p );
    if( dst == f_id ) {
     insert( Msg{ t , g , ht , n } , f_a.f_domain.hash( t ) );
     return;
     }
    auto & o = v_out[ dst ];
    o.push_back( Msg{ t , g , ht , n } );
    ++f_st.sent;
    if( o.size() >= f_a.f_options.batch )
     post( dst , false );
    } );
   }

  // adds the node of m, whose state has hash h, to OPEN, unless it is a
  // duplicate
  void insert( const Msg & m , std::uint64_t h ) {
   if( m.g + m.h >= f_sh.incumbent.load( std::memory_order_relaxed ) )
    return;
   auto [ x , fresh ] = f_table.find_or_insert( m.s , h , [ & ] {
    NodeT * y = f_pool.allocate();
    *y = NodeT{ m.s , m.g , m.h , m.parent , false , false };
    return( y );
    } );
   if( fresh )
    f_open.push( x );
   else
    if( m.g < x->g ) {
     x->g = m.g;
     x->parent = m.parent;
     x->closed = false;
     f_open.push( x );
     }
    else
     ++f_st.duplicates;
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
   o.reserve( f_a.f_options.batch );
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

  const HDAStar & f_a;                          // the search
  Shared & f_sh;                                // the shared data
  unsigned f_id;                                // the index of the worker
  Dist f_dist;                                  // its distribution
  NodePool< NodeT > f_pool;                     // its nodes
  NodeTable< NodeT > f_table;                   // its CLOSED list
  Open f_open;                                  // its OPEN list
  std::vector< std::vector< Msg > > v_out;      // its outboxes
  std::vector< Msg > v_in;                      // the last inbox received
  std::vector< std::uint64_t > v_hash;          // the hashes of v_in
  SearchStats f_st;                             // its counters
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 const D & f_domain;                                   ///< the Domain
 const Dist & f_dist;                    ///< the work distribution
 ParallelOptions f_options;                            ///< the options

 };  // end( class( HDAStar ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __HDAStar */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File HDAStar.h ---------------------------*/
/*--------------------------------------------------------------------------*/

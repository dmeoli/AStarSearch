/*--------------------------------------------------------------------------*/
/*-------------------------- File OpenMPRuntime.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for OpenMPRuntime, the runtime that runs the workers of the
 * parallel searches in an OpenMP parallel region, and for OmpLockChannels,
 * its channels protected by OpenMP locks.
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

#ifndef __OpenMPRuntime
 #define __OpenMPRuntime
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <stdexcept>

#include <string>

#include <vector>

#include <omp.h>

#include "AStarSearch/SearchUtils.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/// initializes an OpenMP lock
/** The default lock of the OpenMP runtime of LLVM is a queuing lock, which
 * hands the lock over in the order of arrival: with more threads than
 * cores the next thread in the queue is often not running, and the threads
 * form a convoy (a search of a few seconds took more than 15 minutes on a
 * runner with 3 cores). There the lock is asked as a test-and-set one; the
 * runtime of gcc, which does not implement the hints, keeps its own. */

inline void init_omp_lock( omp_lock_t * l )
{
 #if defined( KMP_VERSION_MAJOR )
  omp_init_lock_with_hint( l , omp_sync_hint_uncontended );
 #else
  omp_init_lock( l );
 #endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS OmpLockChannels --------------------------*/
/*--------------------------------------------------------------------------*/
/// one inbox per worker, protected by an OpenMP lock
/** The same protocol as MutexChannels [see HDAStar.h], with omp_test_lock()
 * in place of try_lock(). */

template< class Msg >
class OmpLockChannels {

 public:

 explicit OmpLockChannels( unsigned p ) : f_p( p ) , v_in( new Inbox[ p ] ) {
  for( unsigned i = 0 ; i < p ; ++i )
   init_omp_lock( & v_in[ i ].l );
  }

 ~OmpLockChannels() {
  for( unsigned i = 0 ; i < f_p ; ++i )
   omp_destroy_lock( & v_in[ i ].l );
  }

 OmpLockChannels( const OmpLockChannels & ) = delete;
 OmpLockChannels & operator=( const OmpLockChannels & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves b into the inbox of dst if its lock is free; true if done

 bool try_send( unsigned , unsigned dst , std::vector< Msg > & b ) {
  Inbox & in = v_in[ dst ];
  if( ! omp_test_lock( & in.l ) )
   return( false );
  put( in , b );
  omp_unset_lock( & in.l );
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves b into the inbox of dst, waiting for its lock

 void send( unsigned , unsigned dst , std::vector< Msg > & b ) {
  Inbox & in = v_in[ dst ];
  omp_set_lock( & in.l );
  put( in , b );
  omp_unset_lock( & in.l );
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
   omp_set_lock( & in.l );
  else
   if( ! omp_test_lock( & in.l ) )
    return( 0 );
  out.swap( in.q );
  in.mail.store( false , std::memory_order_relaxed );
  omp_unset_lock( & in.l );
  return( out.size() );
  }

 private:

 /// an inbox, alone in its cache lines
 struct alignas( cache_line ) Inbox {
  omp_lock_t l;
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

 unsigned f_p;                                     ///< the number of inboxes
 std::unique_ptr< Inbox[] > v_in;                  ///< the inboxes

 };  // end( class( OmpLockChannels ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS OmpLock ------------------------------*/
/*--------------------------------------------------------------------------*/
/// an OpenMP lock with the interface of std::mutex

class OmpLock {

 public:

 OmpLock( void ) { init_omp_lock( & f_l ); }
 ~OmpLock() { omp_destroy_lock( & f_l ); }
 OmpLock( const OmpLock & ) = delete;
 OmpLock & operator=( const OmpLock & ) = delete;

 void lock( void ) { omp_set_lock( & f_l ); }
 bool try_lock( void ) { return( omp_test_lock( & f_l ) ); }
 void unlock( void ) { omp_unset_lock( & f_l ); }

 private:

 omp_lock_t f_l;                                   ///< the OpenMP lock

 };  // end( class( OmpLock ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OpenMPRuntime ---------------------------*/
/*--------------------------------------------------------------------------*/
/// the runtime of OpenMP: the workers are the threads of a parallel region
/** The region asks for exactly p threads, with dynamic adjustment off, and
 * the threads are bound as OMP_PROC_BIND says. The search needs all of them
 * to run at the same time, so a smaller team is an error. */

struct OpenMPRuntime {

 /// the channels of this runtime
 template< class Msg >
 using channels_type = OmpLockChannels< Msg >;

 /// the lock of this runtime
 using lock_type = OmpLock;

 /// runs f( 0 ) , ... , f( p - 1 ), each on a thread of the team
 template< class F >
 static void run( unsigned p , F && f ) {
  const int dyn = omp_get_dynamic();
  omp_set_dynamic( 0 );
  bool short_team = false;
  #pragma omp parallel num_threads( p )
  {
   if( unsigned( omp_get_num_threads() ) != p ) {
    #pragma omp single
    short_team = true;
    }
   else {
    pin_worker( unsigned( omp_get_thread_num() ) );
    f( unsigned( omp_get_thread_num() ) );
    }
   }
  omp_set_dynamic( dyn );
  if( short_team )
   throw( std::runtime_error( "OpenMPRuntime::run: the team is smaller "
                              "than the number of workers" ) );
  }

 /// the name of the runtime
 [[nodiscard]] static std::string name( void ) { return( "openmp" ); }

 };  // end( struct( OpenMPRuntime ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __OpenMPRuntime */

/*--------------------------------------------------------------------------*/
/*------------------------ End File OpenMPRuntime.h ------------------------*/
/*--------------------------------------------------------------------------*/

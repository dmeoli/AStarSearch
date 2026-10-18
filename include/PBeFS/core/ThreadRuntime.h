/*--------------------------------------------------------------------------*/
/*-------------------------- File ThreadRuntime.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for ThreadRuntime, the runtime that runs the workers of the
 * parallel searches as C++ threads, and for MutexChannels, its channels
 * made of mutex-protected inboxes.
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

#ifndef __PBeFS_ThreadRuntime
 #define __PBeFS_ThreadRuntime
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <mutex>

#include <string>

#include <thread>

#include <vector>

#include "PBeFS/core/SearchUtils.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Parallel Best-First Search library
namespace PBeFS_di_unipi_it
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

}  // end( namespace PBeFS_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_ThreadRuntime */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ThreadRuntime.h ------------------------*/
/*--------------------------------------------------------------------------*/

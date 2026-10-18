/*--------------------------------------------------------------------------*/
/*------------------------- File FastFlowRuntime.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for FastFlowRuntime, the runtime that runs the workers of the
 * parallel searches as the workers of a FastFlow farm, and for
 * SpscMeshChannels, its channels made of FastFlow lock-free single-producer
 * single-consumer queues, one for each ordered pair of workers.
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

#ifndef __PBeFS_FastFlowRuntime
 #define __PBeFS_FastFlowRuntime
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>

#include <memory>

#include <mutex>

#include <stdexcept>

#include <string>

#include <vector>

#include <ff/ff.hpp>

#include <ff/ubuffer.hpp>

#include "PBeFS/core/SearchUtils.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Parallel Best-First Search library
namespace PBeFS_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SpscMeshChannels -------------------------*/
/*--------------------------------------------------------------------------*/
/// a lock-free single-producer single-consumer queue for each pair of
/// workers
/** The queue from src to dst carries pointers to batches of nodes, and a
 * second queue from dst to src brings the emptied batches back to src,
 * which reuses them, so that after the start no batch is allocated. A
 * sender never waits and never fails, and the receiver polls the p queues
 * directed to it; a counter per worker tells whether any of them is
 * non-empty. The queues are the unbounded ones of FastFlow
 * (ff::uSWSR_Ptr_Buffer), which need no lock and no atomic
 * read-modify-write instruction [see Aldinucci et al., "FastFlow:
 * high-level and efficient streaming on multi-core", 2017]. */

template< class Msg >
class SpscMeshChannels {

 public:

 explicit SpscMeshChannels( unsigned p ) : f_p( p ) , v_mail( p ) {
  v_q.reserve( std::size_t( p ) * p );
  v_back.reserve( std::size_t( p ) * p );
  for( std::size_t k = 0 ; k < std::size_t( p ) * p ; ++k ) {
   v_q.emplace_back( std::make_unique< ff::uSWSR_Ptr_Buffer >( 64 ) );
   v_back.emplace_back( std::make_unique< ff::uSWSR_Ptr_Buffer >( 64 ) );
   if( ! v_q.back()->init() || ! v_back.back()->init() )
    throw( std::runtime_error( "SpscMeshChannels::SpscMeshChannels: "
                               "cannot allocate the queues" ) );
   }
  }

 ~SpscMeshChannels() {
  void * x;
  for( auto & q : v_q )
   while( q->pop( & x ) )
    delete static_cast< std::vector< Msg > * >( x );
  for( auto & q : v_back )
   while( q->pop( & x ) )
    delete static_cast< std::vector< Msg > * >( x );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// sends b to dst, leaving in b an empty batch to fill; always true

 bool try_send( unsigned src , unsigned dst , std::vector< Msg > & b ) {
  send( src , dst , b );
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// sends b to dst, leaving in b an empty batch to fill

 void send( unsigned src , unsigned dst , std::vector< Msg > & b ) {
  const std::size_t k = std::size_t( src ) * f_p + dst;
  void * x = nullptr;
  auto * batch = v_back[ k ]->pop( & x )
                 ? static_cast< std::vector< Msg > * >( x )
                 : new std::vector< Msg >();
  batch->swap( b );
  b.clear();
  v_mail[ dst ].n.fetch_add( 1 , std::memory_order_release );
  v_q[ k ]->push( batch );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// true if a batch for dst is travelling

 [[nodiscard]] bool has_mail( unsigned dst ) const noexcept {
  return( v_mail[ dst ].n.load( std::memory_order_acquire ) > 0 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// moves the batches for dst into out (cleared); never waits

 std::size_t receive( unsigned dst , std::vector< Msg > & out , bool ) {
  out.clear();
  if( ! has_mail( dst ) )
   return( 0 );
  for( unsigned src = 0 ; src < f_p ; ++src ) {
   const std::size_t k = std::size_t( src ) * f_p + dst;
   void * x;
   while( v_q[ k ]->pop( & x ) ) {
    auto * batch = static_cast< std::vector< Msg > * >( x );
    if( out.empty() )
     out.swap( *batch );
    else
     out.insert( out.end() , batch->begin() , batch->end() );
    batch->clear();
    v_back[ k ]->push( batch );
    v_mail[ dst ].n.fetch_sub( 1 , std::memory_order_relaxed );
    }
   }
  return( out.size() );
  }

 private:

 /// a counter of the batches travelling to a worker
 struct alignas( cache_line ) Counter {
  std::atomic< std::int64_t > n{ 0 };
  };

 unsigned f_p;                                    ///< the number of workers
 std::vector< std::unique_ptr< ff::uSWSR_Ptr_Buffer > > v_q;
                                       ///< the queue from src to dst
 std::vector< std::unique_ptr< ff::uSWSR_Ptr_Buffer > > v_back;
                                       ///< the empty batches back to src
 std::vector< Counter > v_mail;        ///< the batches travelling to dst

 };  // end( class( SpscMeshChannels ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FastFlowRuntime --------------------------*/
/*--------------------------------------------------------------------------*/
/// the runtime of FastFlow: the workers are the workers of a farm
/** The emitter of the farm sends to the worker i its index and ends; the
 * worker runs f( i ) in its svc(). There is no collector. The library is
 * meant to be compiled with NO_DEFAULT_MAPPING, so that FastFlow does not
 * pin its threads to the cores from the first one on, regardless of the
 * CPUs the process may use: the workers are placed by pin_worker() as
 * those of the other runtimes [see SearchUtils.h]. */

struct FastFlowRuntime {

 /// the channels of this runtime
 template< class Msg >
 using channels_type = SpscMeshChannels< Msg >;

 /// the lock of this runtime
 using lock_type = std::mutex;

 /// runs f( 0 ) , ... , f( p - 1 ), each on a worker of a farm
 template< class F >
 static void run( unsigned p , F && f ) {

  struct Emitter : ff::ff_monode_t< long > {
   explicit Emitter( unsigned np ) : f_np( np ) {}
   long * svc( long * ) override {
    for( unsigned i = 0 ; i < f_np ; ++i )
     ff_send_out_to( new long( i ) , int( i ) );
    return( EOS );
    }
   unsigned f_np;
   };

  struct Worker : ff::ff_node_t< long > {
   explicit Worker( F & body ) : f_body( body ) {}
   long * svc( long * i ) override {
    pin_worker( unsigned( *i ) );
    f_body( unsigned( *i ) );
    delete i;
    return( GO_ON );
    }
   F & f_body;
   };

  std::vector< std::unique_ptr< ff::ff_node > > workers;
  for( unsigned i = 0 ; i < p ; ++i )
   workers.push_back( std::make_unique< Worker >( f ) );
  Emitter emitter( p );
  ff::ff_Farm< long > farm( std::move( workers ) , emitter );
  farm.remove_collector();
  if( farm.run_and_wait_end() < 0 )
   throw( std::runtime_error( "FastFlowRuntime::run: the farm failed" ) );
  }

 /// the name of the runtime
 [[nodiscard]] static std::string name( void ) { return( "fastflow" ); }

 };  // end( struct( FastFlowRuntime ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace PBeFS_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_FastFlowRuntime */

/*--------------------------------------------------------------------------*/
/*----------------------- End File FastFlowRuntime.h -----------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- File MPIMailbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for MPIMailbox, the messages of the searches with one MPI
 * process per worker: the non-blocking sends, the receives, and the
 * detection of the end with the four-counter method of Mattern.
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

#ifndef __AStarSearch_MPIMailbox
 #define __AStarSearch_MPIMailbox
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstring>

#include <list>

#include <type_traits>

#include <vector>

#include <mpi.h>

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MPIMailbox -----------------------------*/
/*--------------------------------------------------------------------------*/
/// the messages of a process of a search over MPI
/** A message is a vector of bytes with a tag, sent without waiting: its
 * buffer is kept until the send completes [see test()]. The end of the
 * search is detected with the four-counter method of Mattern (Distributed
 * Computing, 1987): a process joins a wave, an MPI_Iallreduce of the
 * numbers of messages it has sent and received, only when it has nothing
 * to do; two consecutive waves with equal totals, sent = received, prove
 * that no message is in transit and that every process is waiting, and
 * since every process sees the same totals they all stop at the same wave
 * [see ended()]. All the messages are counted, so that none is left
 * unreceived at the end. */

class MPIMailbox {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

 /// constructor, taking the communicator

 explicit MPIMailbox( MPI_Comm comm ) : f_comm( comm ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// sends the bytes b to dst with the given tag, without waiting; b is
 /// left empty

 void post( unsigned dst , int tag , std::vector< char > & b ) {
  Pending & p = v_pending.emplace_back();
  p.bytes.swap( b );
  b.clear();
  MPI_Isend( p.bytes.data() , int( p.bytes.size() ) , MPI_BYTE ,
             int( dst ) , tag , f_comm , & p.req );
  ++f_sent;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// sends the bytes of the value v to dst with the given tag

 template< class T >
 void post_value( unsigned dst , int tag , const T & v ) {
  static_assert( std::is_trivially_copyable_v< T > ,
                 "MPIMailbox::post_value: the value is not trivially "
                 "copyable" );
  std::vector< char > b( sizeof( T ) );
  std::memcpy( b.data() , & v , sizeof( T ) );
  post( dst , tag , b );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the value whose bytes are in in [see post_value()]

 template< class T >
 [[nodiscard]] static T value( const std::vector< char > & in ) {
  T v;
  std::memcpy( & v , in.data() , sizeof( T ) );
  return( v );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// frees the buffers of the sends that have completed

 void test( void ) {
  for( auto it = v_pending.begin() ; it != v_pending.end() ; ) {
   int done = 0;
   MPI_Test( & it->req , & done , MPI_STATUS_IGNORE );
   if( done )
    it = v_pending.erase( it );
   else
    ++it;
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// receives a message that has arrived, if any, into in, with its tag;
 /// false if there is none

 bool receive( std::vector< char > & in , int & tag ) {
  int flag = 0;
  MPI_Status st;
  MPI_Iprobe( MPI_ANY_SOURCE , MPI_ANY_TAG , f_comm , & flag , & st );
  if( ! flag )
   return( false );
  int bytes = 0;
  MPI_Get_count( & st , MPI_BYTE , & bytes );
  in.resize( std::size_t( bytes ) );
  MPI_Recv( in.data() , bytes , MPI_BYTE , st.MPI_SOURCE , st.MPI_TAG ,
            f_comm , MPI_STATUS_IGNORE );
  ++f_received;
  tag = st.MPI_TAG;
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// true if the end of the search is detected
 /** To be called at each round of the loop of the process, idle telling
  * whether it has nothing to do (no node to work on and nothing to send);
  * the process joins a new wave only when it is idle. */

 bool ended( bool idle ) {
  if( f_wave != MPI_REQUEST_NULL ) {
   int done = 0;
   MPI_Test( & f_wave , & done , MPI_STATUS_IGNORE );
   if( done ) {
    if( ( f_tot[ 0 ] == f_tot[ 1 ] ) && ( f_tot[ 0 ] == f_prev[ 0 ] ) &&
        ( f_tot[ 1 ] == f_prev[ 1 ] ) )
     return( true );
    f_prev[ 0 ] = f_tot[ 0 ];
    f_prev[ 1 ] = f_tot[ 1 ];
    }
   }
  if( ( f_wave == MPI_REQUEST_NULL ) && idle ) {
   f_mine[ 0 ] = f_sent;
   f_mine[ 1 ] = f_received;
   MPI_Iallreduce( f_mine , f_tot , 2 , MPI_LONG_LONG , MPI_SUM , f_comm ,
                   & f_wave );
   }
  return( false );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// waits for the sends in progress, which complete once the end is
 /// detected, since all the messages have then been received

 void finish( void ) {
  if( v_pending.empty() )
   return;
  std::vector< MPI_Request > r;
  for( auto & p : v_pending )
   r.push_back( p.req );
  MPI_Waitall( int( r.size() ) , r.data() , MPI_STATUSES_IGNORE );
  v_pending.clear();
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 /// a buffer of a non-blocking send, kept until the send completes
 struct Pending {
  std::vector< char > bytes;
  MPI_Request req;
  };

 MPI_Comm f_comm;                               ///< the communicator
 std::list< Pending > v_pending;                ///< the sends in progress
 long long f_sent = 0;                          ///< messages sent
 long long f_received = 0;                      ///< messages received
 MPI_Request f_wave = MPI_REQUEST_NULL;         ///< the wave in progress
 long long f_prev[ 2 ] = { -1 , -1 };           ///< the totals before
 long long f_mine[ 2 ] = { 0 , 0 };             ///< its counts in the wave
 long long f_tot[ 2 ] = { 0 , 0 };              ///< the totals of the wave

 };  // end( class( MPIMailbox ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __AStarSearch_MPIMailbox */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPIMailbox.h --------------------------*/
/*--------------------------------------------------------------------------*/

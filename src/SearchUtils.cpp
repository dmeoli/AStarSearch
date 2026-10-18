/*--------------------------------------------------------------------------*/
/*-------------------------- File SearchUtils.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the helpers of SearchUtils.h that depend on the
 * operating system: the affinity mask of the process, the placement of the
 * workers on its CPUs and the allocation of blocks on huge pages. Where
 * the system gives no affinity mask or no huge pages, they fall back on
 * std::thread::hardware_concurrency() and on std::malloc.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- IMPLEMENTATION -----------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstdlib>

#include <new>

#include <thread>

#include <vector>

#include "SearchUtils.h"

#if defined( __linux__ )
 #include <pthread.h>
 #include <sched.h>
 #include <sys/mman.h>
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------------- LOCAL CONSTANTS -----------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

 /// the size of a huge page, and the smallest block asked on huge pages
 constexpr std::size_t c_huge = std::size_t( 1 ) << 21;

 }  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*------------------------ THE CPUS OF THE PROCESS -------------------------*/
/*--------------------------------------------------------------------------*/

const std::vector< int > & AStar_di_unipi_it::affinity_cpus( void )
{
 static const std::vector< int > cpus = [] {
  std::vector< int > c;
  #if defined( __linux__ )
   cpu_set_t set;
   CPU_ZERO( & set );
   if( sched_getaffinity( 0 , sizeof( set ) , & set ) == 0 )
    for( int k = 0 ; k < CPU_SETSIZE ; ++k )
     if( CPU_ISSET( k , & set ) )
      c.push_back( k );
  #endif
  return( c );
  }();
 return( cpus );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned AStar_di_unipi_it::available_cpus( void )
{
 const std::vector< int > & cpus = affinity_cpus();
 if( ! cpus.empty() )
  return( unsigned( cpus.size() ) );
 return( std::thread::hardware_concurrency() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void AStar_di_unipi_it::pin_worker( unsigned i ) noexcept
{
 #if defined( __linux__ )
  static const bool pin = [] {
   const char * v = std::getenv( "ASTAR_PIN" );
   return( v && *v && ( v[ 0 ] != '0' ) );
   }();
  if( ! pin )
   return;
  const std::vector< int > & cpus = affinity_cpus();
  if( cpus.empty() )
   return;
  cpu_set_t one;
  CPU_ZERO( & one );
  CPU_SET( cpus[ i % cpus.size() ] , & one );
  pthread_setaffinity_np( pthread_self() , sizeof( one ) , & one );
 #else
  ( void ) i;
 #endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- HUGE PAGES -------------------------------*/
/*--------------------------------------------------------------------------*/

void * AStar_di_unipi_it::huge_allocate( std::size_t bytes )
{
 #if defined( __linux__ )
  if( bytes >= c_huge ) {
   const std::size_t r = ( bytes + c_huge - 1 ) / c_huge * c_huge;
   void * p = std::aligned_alloc( c_huge , r );
   if( ! p )
    throw( std::bad_alloc() );
   madvise( p , r , MADV_HUGEPAGE );
   return( p );
   }
 #endif
 if( void * p = std::malloc( bytes ? bytes : 1 ) )
  return( p );
 throw( std::bad_alloc() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void AStar_di_unipi_it::huge_deallocate( void * p ) noexcept
{
 std::free( p );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File SearchUtils.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

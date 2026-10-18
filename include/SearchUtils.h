/*--------------------------------------------------------------------------*/
/*--------------------------- File SearchUtils.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the small helpers shared by the searches: a stopwatch,
 * the infinite cost, the reconstruction of a path from parent pointers, a
 * spin-then-yield backoff for the waiting workers, the placement of the
 * workers on the CPUs and an allocator asking for huge pages.
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

#ifndef __SearchUtils
 #define __SearchUtils
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>

#include <chrono>

#include <cstddef>

#include <cstdint>

#include <limits>

#include <new>

#include <thread>

#include <vector>

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ )
 #include <immintrin.h>
#endif


/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------ CONSTANTS AND FUNCTIONS -------------------------*/
/*--------------------------------------------------------------------------*/

/// the size of a cache line, used to keep the data of the workers apart
inline constexpr std::size_t cache_line = 64;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the largest value of a cost type, used as "no incumbent yet"

template< class Cost >
[[nodiscard]] constexpr Cost infinite_cost( void ) noexcept
{
 if constexpr( std::numeric_limits< Cost >::has_infinity )
  return( std::numeric_limits< Cost >::infinity() );
 else
  return( std::numeric_limits< Cost >::max() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the states from the root to goal, following the parent pointers

template< class NodeT >
[[nodiscard]] auto trace_path( const NodeT * goal )
{
 std::vector< decltype( goal->state ) > path;
 for( const NodeT * n = goal ; n ; n = n->parent )
  path.push_back( n->state );
 std::reverse( path.begin() , path.end() );
 return( path );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a short busy-wait telling the core that the thread is spinning

inline void cpu_relax( void ) noexcept
{
 #if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ )
  _mm_pause();
 #elif defined( __aarch64__ )
  asm volatile( "yield" );
 #endif
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the CPUs of the affinity mask of the process
/** On Linux, the CPUs in the affinity mask of the process as it was at the
 * first call (e.g., the CPUs given to numactl or taskset), in increasing
 * order; elsewhere, an empty vector. The first call has to be made before
 * any thread is pinned. */

[[nodiscard]] const std::vector< int > & affinity_cpus( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the number of CPUs the process may run on, 0 if it is not known
/** The size of affinity_cpus() where the affinity mask is available, and
 * std::thread::hardware_concurrency() elsewhere. */

[[nodiscard]] unsigned available_cpus( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// pins the calling thread to the i-th CPU it may run on, if ASTAR_PIN is set
/** On Linux, if the environment variable ASTAR_PIN is set to a non-empty
 * value other than 0, binds the calling thread to the i-th CPU (modulo
 * their number) of affinity_cpus(); elsewhere, or without ASTAR_PIN, does
 * nothing. The runtimes call it at the start of each worker, so that the
 * workers of every runtime are placed the same way. */

void pin_worker( unsigned i ) noexcept;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a block of at least bytes bytes, on huge pages if it is large enough
/** On Linux the blocks of at least 2 MB are aligned to 2 MB and handed to
 * madvise( MADV_HUGEPAGE ), which is what the transparent huge pages need
 * when the kernel gives them only on request (the "madvise" setting), and
 * the smaller ones, and every block elsewhere, come from std::malloc.
 * Throws std::bad_alloc if there is no memory. */

[[nodiscard]] void * huge_allocate( std::size_t bytes );

/// frees a block returned by huge_allocate()

void huge_deallocate( void * p ) noexcept;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// hints the processor to bring the cache line of p in, for reading

inline void prefetch( const void * p ) noexcept
{
 #if defined( __GNUC__ ) || defined( __clang__ )
  __builtin_prefetch( p , 0 , 1 );
 #else
  ( void ) p;
 #endif
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a real number in [ 0 , 1 ) from the next word of a 64-bit generator
/** The distributions of <random> are not the same on every standard
 * library, while the words of std::mt19937_64 are; this and random_below()
 * draw from those words directly, so that a seed gives the same instance on
 * every platform. */

template< class Gen >
[[nodiscard]] double random_unit( Gen & gen )
{
 return( double( std::uint64_t( gen() ) >> 11 ) * 0x1.0p-53 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// an integer in [ 0 , n ) from the next word of a 64-bit generator, n > 0

template< class Gen >
[[nodiscard]] std::uint64_t random_below( Gen & gen , std::uint64_t n )
{
 return( std::uint64_t( gen() ) % n );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS HugePageAllocator -------------------------*/
/*--------------------------------------------------------------------------*/
/// an allocator that asks for huge pages for the blocks of 2 MB or more
/** The closed list and the arena of the nodes of a large search are read
 * at random addresses: with pages of 4 KB nearly every access to them
 * misses the TLB, while a page of 2 MB covers 512 times as much memory.
 * The blocks come from huge_allocate(). */

template< class T >
struct HugePageAllocator {

 using value_type = T;                             ///< the allocated type

 HugePageAllocator( void ) = default;

 template< class U >
 HugePageAllocator( const HugePageAllocator< U > & ) noexcept {}

 /// n uninitialized objects of type T
 [[nodiscard]] T * allocate( std::size_t n ) {
  return( static_cast< T * >( huge_allocate( n * sizeof( T ) ) ) );
  }

 /// frees what allocate() returned
 void deallocate( T * p , std::size_t ) noexcept { huge_deallocate( p ); }

 template< class U >
 bool operator==( const HugePageAllocator< U > & ) const noexcept {
  return( true );
  }

 };  // end( struct( HugePageAllocator ) )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Stopwatch -----------------------------*/
/*--------------------------------------------------------------------------*/
/// a wall-clock stopwatch, started at construction

class Stopwatch {

 public:

 Stopwatch( void ) : f_start( std::chrono::steady_clock::now() ) {}

 /// the seconds elapsed since the construction
 [[nodiscard]] double seconds( void ) const {
  return( std::chrono::duration< double >(
                       std::chrono::steady_clock::now() - f_start ).count() );
  }

 private:

 std::chrono::steady_clock::time_point f_start;     ///< the start time

 };  // end( class( Stopwatch ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Backoff ------------------------------*/
/*--------------------------------------------------------------------------*/
/// spins for a while, then yields the processor

class Backoff {

 public:

 /// waits a little, a little longer after the first 64 calls
 void pause( void ) noexcept {
  if( f_spins < 64 ) {
   ++f_spins;
   cpu_relax();
   }
  else
   std::this_thread::yield();
  }

 /// starts spinning again
 void reset( void ) noexcept { f_spins = 0; }

 private:

 unsigned f_spins = 0;                  ///< the pauses since reset()

 };  // end( class( Backoff ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __SearchUtils */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SearchUtils.h -------------------------*/
/*--------------------------------------------------------------------------*/

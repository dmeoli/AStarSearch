/*--------------------------------------------------------------------------*/
/*---------------------------- File NodeTable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template NodeTable, the closed list of the
 * searches: a map from the generated states to their canonical Node.
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

#ifndef __NodeTable
 #define __NodeTable
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>

#include <cstddef>

#include <cstdint>

#include <utility>

#include <vector>

#include "AStarSearch/SearchUtils.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS NodeTable -----------------------------*/
/*--------------------------------------------------------------------------*/
/// open addressing hash table of node pointers with linear probing
/** Every slot keeps the full 64-bit hash next to the pointer, so that a probe
 * compares two states only when their hashes match and a resize never
 * touches the nodes. Linear probing scans consecutive slots, that is,
 * consecutive cache lines, which is what makes it faster than chaining when
 * the table does not fit in the cache. The slot of a hash is taken from the
 * top bits of its product by 2^64 / phi (Fibonacci hashing), so that hash
 * values whose low bits depend on the owner thread, as with a distribution
 * hash( s ) mod p, still spread over the whole table. The load factor is
 * kept below 7 / 10, and the slots come from zeroed_allocate(), i.e., on
 * huge pages and already zeroed by the kernel, so that a table that grows
 * writes no zeros (an empty slot is all zeros, the null pointer included). A
 * search can call prefetch( hash ) for all the successors of a node before
 * looking them up, so that the cache misses of the lookups overlap. */

template< class NodeT >
class NodeTable {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

 /// the number of slots that holds n nodes without growing

 [[nodiscard]] static std::size_t slots_for( std::size_t n ) noexcept {
  return( n / 7 * 10 + 16 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// constructor, taking the initial number of slots

 explicit NodeTable( std::size_t capacity = 1024 ) : f_size( 0 ) {
  f_n = std::bit_ceil( capacity < 16 ? std::size_t( 16 ) : capacity );
  v_slots = slots( f_n );
  f_mask = f_n - 1;
  f_shift = 64 - std::countr_zero( f_n );
  }

 /// destructor: frees the slots

 ~NodeTable() { zeroed_deallocate( v_slots , f_n * sizeof( Slot ) ); }

 NodeTable( const NodeTable & ) = delete;
 NodeTable & operator=( const NodeTable & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// looks up a state, inserting the node made by make() if it is absent
 /** Looks up \p state, whose hash is \p hash; if it is not there, inserts
  * the node returned by \p make(). Returns the node of \p state and true
  * if it has just been inserted. */

 template< class State , class Make >
 std::pair< NodeT * , bool > find_or_insert( const State & state ,
                                             std::uint64_t hash ,
                                             Make && make ) {
  std::size_t i = index( hash );
  while( v_slots[ i ].node ) {
   if( ( v_slots[ i ].hash == hash ) && ( v_slots[ i ].node->state == state ) )
    return( std::pair( v_slots[ i ].node , false ) );
   i = ( i + 1 ) & f_mask;
   }
  NodeT * node = make();
  v_slots[ i ] = { hash , node };
  if( ++f_size * 10 > f_n * 7 )
   grow();
  return( std::pair( node , true ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// looks up a state, returning nullptr if it has never been generated

 template< class State >
 [[nodiscard]] NodeT * find( const State & state , std::uint64_t hash ) const
 {
  std::size_t i = index( hash );
  while( v_slots[ i ].node ) {
   if( ( v_slots[ i ].hash == hash ) && ( v_slots[ i ].node->state == state ) )
    return( v_slots[ i ].node );
   i = ( i + 1 ) & f_mask;
   }
  return( nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// brings in the cache the first slot probed for hash

 void prefetch( std::uint64_t hash ) const noexcept {
  AStar_di_unipi_it::prefetch( & v_slots[ index( hash ) ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the number of stored nodes

 [[nodiscard]] std::size_t size( void ) const noexcept {
  return( f_size );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the memory taken by the slots, in bytes

 [[nodiscard]] std::size_t bytes( void ) const noexcept {
  return( f_n * sizeof( Slot ) );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 /// a slot: the hash of the state and its node (nullptr if empty)
 struct Slot {
  std::uint64_t hash = 0;
  NodeT * node = nullptr;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // the slot of a hash: the top bits of hash * 2^64 / phi

 [[nodiscard]] std::size_t index( std::uint64_t hash ) const noexcept {
  return( static_cast< std::size_t >( ( hash * 0x9e3779b97f4a7c15ULL )
                                      >> f_shift ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // n slots, all empty: zeros that nobody writes [see zeroed_allocate()]

 static Slot * slots( std::size_t n ) {
  return( static_cast< Slot * >( zeroed_allocate( n * sizeof( Slot ) ) ) );
  }

 // doubles the number of slots, moving the occupied ones

 void grow( void ) {
  Slot * old = v_slots;
  const std::size_t n = f_n;
  f_n *= 2;
  v_slots = slots( f_n );
  f_mask = f_n - 1;
  --f_shift;
  for( std::size_t k = 0 ; k < n ; ++k ) {
   const Slot & s = old[ k ];
   if( ! s.node )
    continue;
   std::size_t i = index( s.hash );
   while( v_slots[ i ].node )
    i = ( i + 1 ) & f_mask;
   v_slots[ i ] = s;
   }
  zeroed_deallocate( old , n * sizeof( Slot ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Slot * v_slots;                             ///< the slots, zeroed
 std::size_t f_n;                            ///< the number of slots
 std::size_t f_mask;                         ///< the number of slots - 1
 int f_shift;                      ///< 64 - log2 of the number of slots
 std::size_t f_size;                         ///< the number of nodes

 };  // end( class( NodeTable ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __NodeTable */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File NodeTable.h --------------------------*/
/*--------------------------------------------------------------------------*/

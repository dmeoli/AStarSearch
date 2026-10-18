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

#include <limits>

#include <stdexcept>

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
/** Every slot keeps the full 64-bit hash next to the pointer, so that a
 * probe compares two states only when their hashes match and a resize never
 * touches the nodes. Linear probing scans consecutive slots, that is,
 * consecutive cache lines, which is what makes it faster than chaining when
 * the table does not fit in the cache. The slot of a hash is taken from the
 * top bits of its product by 2^64 / phi (Fibonacci hashing), so that hash
 * values whose low bits depend on the owner thread, as with a distribution
 * hash( s ) mod p, still spread over the whole table. The load factor is
 * kept below 7 / 10, and the slots are allocated on huge pages [see
 * HugePageAllocator in SearchUtils.h]. A search can call prefetch( hash )
 * for all the successors of a node before looking them up, so that the
 * cache misses of the lookups overlap. */

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
  v_slots.resize( std::bit_ceil( capacity < 16 ? std::size_t( 16 )
                                               : capacity ) );
  f_mask = v_slots.size() - 1;
  f_shift = 64 - std::countr_zero( v_slots.size() );
  }

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
  if( ++f_size * 10 > v_slots.size() * 7 )
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
  return( v_slots.size() * sizeof( Slot ) );
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
 // doubles the number of slots, moving the old ones

 void grow( void ) {
  std::vector< Slot , HugePageAllocator< Slot > > old( v_slots.size() * 2 );
  old.swap( v_slots );
  f_mask = v_slots.size() - 1;
  --f_shift;
  for( const Slot & s : old ) {
   if( ! s.node )
    continue;
   std::size_t i = index( s.hash );
   while( v_slots[ i ].node )
    i = ( i + 1 ) & f_mask;
   v_slots[ i ] = s;
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 std::vector< Slot , HugePageAllocator< Slot > > v_slots;   ///< the slots
 std::size_t f_mask;                         ///< the number of slots - 1
 int f_shift;                      ///< 64 - log2 of the number of slots
 std::size_t f_size;                         ///< the number of nodes

 };  // end( class( NodeTable ) )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS IndexTable ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the closed list in slots of 8 bytes, for the nodes of one NodePool
/** A slot keeps 32 bits of the hash of the state (the fingerprint) and the
 * index of the node in the NodePool plus one, so that 0 is an empty slot:
 * the table takes half the memory of NodeTable, whose slots keep the whole
 * hash and the pointer, at the price of an indirection through the chunks
 * of the pool and of a doubling that recomputes the hashes of the states
 * (with expected_nodes in the SearchOptions the table never doubles). The
 * first slot of a hash is chosen as in NodeTable, the probing is linear,
 * the load factor stays below 7 / 10, and the slots are on huge pages. Up
 * to 2^32 - 1 nodes. The node that make() returns has to be the last one
 * allocated in the pool. */

template< class NodeT , class Pool , class Hash >
class IndexTable {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

 /// the number of slots that holds n nodes without growing

 [[nodiscard]] static std::size_t slots_for( std::size_t n ) noexcept {
  return( n / 7 * 10 + 16 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// constructor, taking the pool of the nodes, the hash of the states and
 /// the initial number of slots

 IndexTable( const Pool & pool , Hash hash , std::size_t capacity = 1024 )
  : f_pool( pool ) , f_hash( std::move( hash ) ) , f_size( 0 ) {
  v_slots.resize( std::bit_ceil( capacity < 16 ? std::size_t( 16 )
                                               : capacity ) );
  f_mask = v_slots.size() - 1;
  f_shift = 64 - std::countr_zero( v_slots.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// looks up a state, inserting the node made by make() if it is absent

 template< class State , class Make >
 std::pair< NodeT * , bool > find_or_insert( const State & state ,
                                             std::uint64_t hash ,
                                             Make && make ) {
  const auto fp = std::uint32_t( hash );
  std::size_t i = index( hash );
  for( ; v_slots[ i ] ; i = ( i + 1 ) & f_mask )
   if( std::uint32_t( v_slots[ i ] >> 32 ) == fp ) {
    NodeT * n = node( v_slots[ i ] );
    if( n->state == state )
     return( std::pair( n , false ) );
    }
  NodeT * n = make();
  const std::size_t k = f_pool.size();       // the index of n, plus one
  if( k > std::numeric_limits< std::uint32_t >::max() )
   throw( std::length_error( "IndexTable::find_or_insert: more than "
                             "2^32 - 1 nodes" ) );
  v_slots[ i ] = ( std::uint64_t( fp ) << 32 ) | k;
  if( ++f_size * 10 > v_slots.size() * 7 )
   grow();
  return( std::pair( n , true ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// looks up a state, returning nullptr if it has never been generated

 template< class State >
 [[nodiscard]] NodeT * find( const State & state , std::uint64_t hash ) const
 {
  const auto fp = std::uint32_t( hash );
  for( std::size_t i = index( hash ) ; v_slots[ i ] ;
       i = ( i + 1 ) & f_mask )
   if( std::uint32_t( v_slots[ i ] >> 32 ) == fp ) {
    NodeT * n = node( v_slots[ i ] );
    if( n->state == state )
     return( n );
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
  return( v_slots.size() * sizeof( std::uint64_t ) );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 // the node of a slot
 [[nodiscard]] NodeT * node( std::uint64_t slot ) const noexcept {
  return( f_pool.at( std::size_t( std::uint32_t( slot ) ) - 1 ) );
  }

 // the slot of a hash: the top bits of hash * 2^64 / phi
 [[nodiscard]] std::size_t index( std::uint64_t hash ) const noexcept {
  return( static_cast< std::size_t >( ( hash * 0x9e3779b97f4a7c15ULL )
                                      >> f_shift ) );
  }

 // doubles the number of slots, moving the old ones by the hashes of the
 // states of their nodes
 void grow( void ) {
  std::vector< std::uint64_t , HugePageAllocator< std::uint64_t > > old(
                                                       v_slots.size() * 2 );
  old.swap( v_slots );
  f_mask = v_slots.size() - 1;
  --f_shift;
  for( const std::uint64_t s : old ) {
   if( ! s )
    continue;
   std::size_t i = index( f_hash( node( s )->state ) );
   while( v_slots[ i ] )
    i = ( i + 1 ) & f_mask;
   v_slots[ i ] = s;
   }
  }

 const Pool & f_pool;                        ///< the pool of the nodes
 Hash f_hash;                                ///< the hash of a state
 std::vector< std::uint64_t , HugePageAllocator< std::uint64_t > > v_slots;
                                             ///< the slots
 std::size_t f_mask;                         ///< the number of slots - 1
 int f_shift;                      ///< 64 - log2 of the number of slots
 std::size_t f_size;                         ///< the number of nodes

 };  // end( class( IndexTable ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __NodeTable */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File NodeTable.h --------------------------*/
/*--------------------------------------------------------------------------*/

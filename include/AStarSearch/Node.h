/*--------------------------------------------------------------------------*/
/*------------------------------ File Node.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template Node, a node of the search graph, and
 * for the class template NodePool, the arena the nodes are allocated from.
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

#ifndef __AStarSearch_Node
 #define __AStarSearch_Node
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>

#include <memory>

#include <vector>

#include "AStarSearch/Domain.h"

#include "AStarSearch/SearchUtils.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASS Node -------------------------------*/
/*--------------------------------------------------------------------------*/
/// a node of the search graph
/** There is one canonical Node per generated state. The field closed tells
 * whether the Node is in CLOSED, i.e., it has been expanded with its current
 * g; a Node reached again through a cheaper path is moved back to OPEN by
 * clearing it [see line 11 of Algorithm 1 of Fukunaga et al., 2017], while
 * expanded stays set, so that the re-expansions can be counted. The field
 * op is used by the OperatorDomain only, and fits in the padding. */

template< class State , class Cost >
struct Node {

 State state;     ///< the state
 Cost g;          ///< the cost of the best path found from the root
 Cost h;          ///< the heuristic estimate of the cost to a goal
 Node * parent;   ///< the predecessor on that path (nullptr at the root)
 bool closed;     ///< true if expanded with the current g
 bool expanded;   ///< true if ever expanded
 std::uint8_t op = no_op;  ///< the operator that generated the state

 /// the estimate f = g + h of the cost of a solution through the Node
 [[nodiscard]] Cost f( void ) const noexcept { return( g + h ); }

 };  // end( struct( Node ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS NodePool -----------------------------*/
/*--------------------------------------------------------------------------*/
/// an arena of nodes allocated in large chunks
/** Nodes are never freed one by one during the search, so allocating them
 * in blocks removes the cost of the allocator and keeps them contiguous in
 * memory; their addresses never change, which the parent pointers rely on.
 * The chunks of 2^16 nodes take a few MB each and go on huge pages [see
 * HugePageAllocator in SearchUtils.h]. */

template< class NodeT >
class NodePool {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

 /// constructor, taking the number of nodes of a chunk

 explicit NodePool( std::size_t chunk = std::size_t( 1 ) << 16 )
  : f_chunk( chunk ) , f_used( 0 ) , f_size( 0 ) {}

 /// destructor: destroys the nodes and frees the chunks

 ~NodePool() {
  for( NodeT * c : v_chunks ) {
   std::destroy_n( c , f_chunk );
   HugePageAllocator< NodeT >().deallocate( c , f_chunk );
   }
  }

 NodePool( const NodePool & ) = delete;
 NodePool & operator=( const NodePool & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a new (uninitialized) node

 [[nodiscard]] NodeT * allocate( void ) {
  if( v_chunks.empty() || ( f_used == f_chunk ) ) {
   NodeT * c = HugePageAllocator< NodeT >().allocate( f_chunk );
   std::uninitialized_default_construct_n( c , f_chunk );
   v_chunks.push_back( c );
   f_used = 0;
   }
  ++f_size;
  return( v_chunks.back() + f_used++ );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the number of nodes allocated so far

 [[nodiscard]] std::size_t size( void ) const noexcept {
  return( f_size );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the memory taken by the chunks, in bytes

 [[nodiscard]] std::size_t bytes( void ) const noexcept {
  return( v_chunks.size() * f_chunk * sizeof( NodeT ) );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 std::size_t f_chunk;                            ///< nodes per chunk
 std::size_t f_used;                   ///< nodes used in the last chunk
 std::size_t f_size;                             ///< nodes allocated
 std::vector< NodeT * > v_chunks;                          ///< the chunks

 };  // end( class( NodePool ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __AStarSearch_Node */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Node.h -----------------------------*/
/*--------------------------------------------------------------------------*/

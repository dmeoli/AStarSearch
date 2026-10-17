/*--------------------------------------------------------------------------*/
/*------------------------------ File AStar.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class template AStar, the sequential A* search of
 * Hart, Nilsson and Raphael (1968).
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

#ifndef __AStar
 #define __AStar
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Domain.h"

#include "Node.h"

#include "NodeTable.h"

#include "OpenList.h"

#include "SearchUtils.h"

#include "Solution.h"

/*--------------------------------------------------------------------------*/
/*------------------------------- NAMESPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the A* search library
namespace AStar_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS AStar -------------------------------*/
/*--------------------------------------------------------------------------*/
/// sequential A* over a Domain, with the OPEN list Open
/** This is Algorithm 1 of Fukunaga et al. (2017): the goal test is done
 * when a node is selected for expansion, so that the first goal expanded is
 * optimal if h is admissible. A node reached through a cheaper path is
 * reopened even if it is closed, so that the search stays admissible with
 * an admissible but inconsistent heuristic; with a consistent one no node
 * is ever reopened.
 *
 * Open defaults to BucketOpenList for integer costs and to HeapOpenList
 * otherwise [see OpenList.h]. */

template< Domain D ,
          class Open = std::conditional_t<
           std::is_integral_v< cost_t< D > > ,
           BucketOpenList< Node< state_t< D > , cost_t< D > > > ,
           HeapOpenList< Node< state_t< D > , cost_t< D > > > > >
class AStar {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using State = state_t< D >;                       ///< the state type
 using Cost = cost_t< D >;                         ///< the cost type
 using NodeT = Node< State , Cost >;               ///< the node type
 using SolutionT = Solution< State , Cost >;       ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the Domain (which must outlive the AStar)

 explicit AStar( const D & domain , const SearchOptions & options = {} )
  : f_domain( domain ) , f_options( options ) {}

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the Domain, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  Stopwatch clock;
  SolutionT sol;
  SearchStats & st = sol.stats;
  NodePool< NodeT > pool;
  NodeTable< NodeT > table;
  Open open;
  std::vector< Kid > kids;                 // the successors of a node

  const State s0 = f_domain.initial_state();
  NodeT * root = table.find_or_insert( s0 , f_domain.hash( s0 ) , [ & ] {
   NodeT * n = pool.allocate();
   *n = NodeT{ s0 , Cost( 0 ) , Cost( f_domain.heuristic( s0 ) ) , nullptr ,
               false , false };
   return( n );
   } ).first;
  open.push( root );

  while( ! open.empty() ) {
   const OpenEntry< NodeT > e = open.pop();
   if( e.stale() )
    continue;

   NodeT * n = e.node;
   n->closed = true;
   ++st.expanded;
   if( n->expanded )
    ++st.reexpanded;
   n->expanded = true;

   if( f_domain.is_goal( n->state ) ) {
    sol.status = Status::solved;
    sol.cost = n->g;
    sol.path = trace_path( n );
    break;
    }

   // first the successors, with the slots of their lookups prefetched,
   // then the lookups, whose cache misses thus overlap
   kids.clear();
   f_domain.for_each_successor( n->state , n->h ,
                                [ & ]( const State & t , Cost c , Cost ht ) {
    const std::uint64_t hash = f_domain.hash( t );
    table.prefetch( hash );
    kids.push_back( Kid{ t , c , ht , hash } );
    } );
   st.generated += kids.size();
   for( const Kid & k : kids ) {
    const Cost g = n->g + k.c;
    auto [ m , fresh ] = table.find_or_insert( k.s , k.hash , [ & ] {
     NodeT * x = pool.allocate();
     *x = NodeT{ k.s , g , k.h , n , false , false };
     return( x );
     } );
    if( fresh )
     open.push( m );
    else
     if( g < m->g ) {     // a cheaper path: reopen
      m->g = g;
      m->parent = n;
      m->closed = false;
      open.push( m );
      }
     else
      ++st.duplicates;
    }

   if( f_options.node_limit && ( pool.size() >= f_options.node_limit ) ) {
    sol.status = Status::node_limit;
    break;
    }
   }

  st.stored = pool.size();
  st.expanded_by = { st.expanded };
  st.stored_by = { st.stored };
  st.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 /// a successor: its state, the cost of its edge, its h, its hash
 struct Kid {
  State s;
  Cost c;
  Cost h;
  std::uint64_t hash;
  };

 const D & f_domain;                                 ///< the Domain
 SearchOptions f_options;                            ///< the options

 };  // end( class( AStar ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
/// searches a Domain with A*, choosing the OPEN list at run time
/** Searches \p domain with A*; the OPEN list is the bucket queue for
 * integer costs and the heap otherwise, unless \p open asks for the heap.
 */

template< Domain D >
[[nodiscard]] Solution< state_t< D > , cost_t< D > > astar(
 const D & domain , OpenKind open = OpenKind::automatic ,
 const SearchOptions & options = {} )
{
 using NodeT = Node< state_t< D > , cost_t< D > >;
 if constexpr( std::is_integral_v< cost_t< D > > )
  if( open != OpenKind::heap )
   return( AStar< D , BucketOpenList< NodeT > >( domain , options ).solve()
           );
 return( AStar< D , HeapOpenList< NodeT > >( domain , options ).solve() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __AStar */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File AStar.h ----------------------------*/
/*--------------------------------------------------------------------------*/

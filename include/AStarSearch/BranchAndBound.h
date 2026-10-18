/*--------------------------------------------------------------------------*/
/*------------------------- File BranchAndBound.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the best-first search with an incumbent, i.e., for
 * Branch-and-Bound (B&B): the concept BranchAndBound of a problem, its
 * options and its Solution, the queue of the open nodes, and the
 * sequential search BranchAndBoundSearch. The parallel searches are in
 * ParallelBranchAndBound.h, and the design is discussed in
 * docs/branch-and-bound.md.
 *
 * All the values (bounds, feasible values, incumbent and tolerances) are
 * doubles; a maximization problem is solved by negating them internally, so
 * that the code below only minimizes.
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

#ifndef __AStarSearch_BranchAndBound
 #define __AStarSearch_BranchAndBound
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>

#include <cmath>

#include <concepts>

#include <cstddef>

#include <cstdint>

#include <functional>

#include <limits>

#include <optional>

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
/*--------------------------------- TYPES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the sense of the optimization
enum class Sense {
 minimize ,     ///< the least value is sought
 maximize       ///< the largest value is sought
 };

/// the order in which the open nodes are extracted
enum class Selection {
 best_first ,     ///< the least bound first
 depth_first ,    ///< the deepest node first
 breadth_first ,  ///< the shallowest node first, the oldest among those
 best_estimate ,  ///< the least estimate( n ) of the problem first
 plunging         ///< a dive into the children, then best-first
 };

/// how a B&B ended
enum class BBStatus {
 optimal ,      ///< the incumbent is optimal within the tolerance
 infeasible ,   ///< no feasible solution exists (nor was given)
 node_limit ,   ///< the limit on the evaluated nodes was reached first
 time_limit     ///< the time limit was reached first
 };

/// the outcome of the evaluation of a node
struct Evaluation {
 double bound = 0;                ///< a bound on the values of the node
 bool infeasible = false;         ///< true if the node has no solution
 std::optional< double > feasible;  ///< a feasible value, n being its
                                    ///< solution, if there is one
 };

namespace detail {
 /// a callable accepting a child and its estimate, used only to state the
 /// concepts
 template< class N >
 struct bb_sink {
  void operator()( const N & , double ) const noexcept {}
  };
 }  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/
/// what evaluates and branches the nodes of a B&B
/** evaluate( n ) returns the Evaluation of the node n, possibly storing in
 * n what the branching needs, and branch( n , sink ) calls sink( c , e )
 * for every child c of the evaluated node n, e being a bound of c that is
 * cheap to compute (typically the bound of n). */

template< class W , class N >
concept BBEvaluator = requires( W & w , N & n , const N & cn ) {
 { w.evaluate( n ) } -> std::convertible_to< Evaluation >;
 w.branch( cn , detail::bb_sink< N >{} );
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a B&B problem
/** A type P is a BranchAndBound problem if it provides the copyable type
 * node_type, the constant sense, the method root() giving the node of the
 * whole set, and either the methods evaluate( n ) and branch( n , sink )
 * of a BBEvaluator, which have to be safe to call concurrently in a
 * parallel search, or make_worker( i ), which returns the BBEvaluator of
 * the worker i (e.g., one with its own copy of a model and of a solver).
 *
 * Optionally, P may give hash( n ), which the distributed search uses to
 * send n to its owner, and estimate( n ), the estimate of the best
 * feasible value under n that orders the nodes with
 * Selection::best_estimate. */

template< class P >
concept BranchAndBound = requires( const P & p , unsigned i ) {
 typename P::node_type;
 requires std::copyable< typename P::node_type >;
 { P::sense } -> std::convertible_to< Sense >;
 { p.root() } -> std::convertible_to< typename P::node_type >;
 } && ( BBEvaluator< const P , typename P::node_type > ||
        requires( const P & p , unsigned i ) {
         { p.make_worker( i ) }
          -> BBEvaluator< typename P::node_type >;
         } );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a B&B problem that gives a hash of its nodes

template< class P >
concept HashedBranchAndBound = BranchAndBound< P > &&
 requires( const P & p , const typename P::node_type & n ) {
  { p.hash( n ) } -> std::convertible_to< std::uint64_t >;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// a B&B problem that estimates the best feasible value under a node

template< class P >
concept EstimatingBranchAndBound = BranchAndBound< P > &&
 requires( const P & p , const typename P::node_type & n ) {
  { p.estimate( n ) } -> std::convertible_to< double >;
  };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BBOptions -----------------------------*/
/*--------------------------------------------------------------------------*/
/// the options of a B&B
/** A node is pruned when its bound cannot improve the incumbent z by more
 * than the tolerance max( abs_gap , rel_gap |z| ), and the search ends
 * when no open node can. With Selection::plunging, the best child of the
 * node just branched is extracted next (a dive) if its bound is within
 * best + plunge_gap ( z - best ), best being the least open bound, and
 * always while there is no incumbent. */

struct BBOptions {

 Selection selection = Selection::best_first;   ///< the order of the nodes
 double abs_gap = 0;           ///< the absolute tolerance on the gap
 double rel_gap = 0;           ///< the relative tolerance on the gap
 double plunge_gap = 0.5;      ///< how far from the best bound a dive goes
 std::optional< double > incumbent;  ///< a known feasible value, if any
 std::size_t node_limit = 0;   ///< evaluated nodes (0 = no limit)
 double time_limit = 0;        ///< seconds (0 = no limit)
 /// called with every new incumbent value (in a parallel search, by the
 /// worker that finds it, under a lock)
 std::function< void( double ) > on_incumbent;

 };  // end( struct( BBOptions ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BBStats ------------------------------*/
/*--------------------------------------------------------------------------*/
/// the counters of a B&B
/** A node is pruned when it is generated, when it is extracted (by the
 * incumbent found after its generation) or after its evaluation (by its
 * own bound); the vector has one entry per worker. */

struct BBStats {

 std::uint64_t evaluated = 0;         ///< nodes evaluated
 std::uint64_t branched = 0;          ///< nodes branched
 std::uint64_t generated = 0;         ///< children generated
 std::uint64_t infeasible = 0;        ///< nodes found infeasible
 std::uint64_t pruned_generated = 0;  ///< children pruned when generated
 std::uint64_t pruned_extracted = 0;  ///< nodes pruned when extracted
 std::uint64_t pruned_evaluated = 0;  ///< nodes pruned by their bound
 std::uint64_t incumbents = 0;        ///< improvements of the incumbent
 std::uint64_t sent = 0;              ///< nodes sent to another worker
 std::uint64_t messages = 0;          ///< batches of nodes sent
 std::size_t max_open = 0;            ///< the most open nodes of a queue
 double seconds = 0;                  ///< wall-clock time of the search
 std::vector< std::uint64_t > evaluated_by;  ///< evaluations per worker

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds the counters of a worker

 void add( const BBStats & o ) {
  evaluated += o.evaluated;
  branched += o.branched;
  generated += o.generated;
  infeasible += o.infeasible;
  pruned_generated += o.pruned_generated;
  pruned_extracted += o.pruned_extracted;
  pruned_evaluated += o.pruned_evaluated;
  incumbents += o.incumbents;
  sent += o.sent;
  messages += o.messages;
  max_open = std::max( max_open , o.max_open );
  evaluated_by.push_back( o.evaluated );
  }

 };  // end( struct( BBStats ) )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BBSolution ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the outcome of a B&B
/** value is the incumbent value and bound a bound on the optimum, both in
 * the sense of the problem, and gap = | value - bound | is valid whatever
 * the status; node is the incumbent node, empty if no feasible node was
 * found (even when an initial incumbent value was given). */

template< class Node >
struct BBSolution {

 BBStatus status = BBStatus::infeasible;   ///< how the search ended
 std::optional< Node > node;               ///< the incumbent node
 double value = std::numeric_limits< double >::quiet_NaN();  ///< its value
 double bound = std::numeric_limits< double >::quiet_NaN();  ///< the bound
 double gap = std::numeric_limits< double >::infinity();     ///< the gap
 BBStats stats;                            ///< the counters

 /// true if a feasible value is known
 [[nodiscard]] bool feasible( void ) const noexcept {
  return( std::isfinite( value ) );
  }

 };  // end( struct( BBSolution ) )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BBQueue ------------------------------*/
/*--------------------------------------------------------------------------*/
/// the open nodes of a B&B, ordered by a key
/** The nodes are kept in slots with a free list, since a node is destroyed
 * when it is branched, and a binary heap of the pairs ( key , index ) gives
 * the next one: the least key first and, among equal keys, the last pushed
 * (the first pushed with Selection::breadth_first). When the key of the
 * selection is not the bound, a second heap of the bounds, whose entries of
 * the removed nodes are deleted lazily, gives the least bound of the open
 * nodes, which the termination test needs. */

template< class Node >
class BBQueue {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

 /// an open node
 struct Item {
  Node node;          ///< the node
  double bound;       ///< its bound, in the minimization
  unsigned depth;     ///< its depth in the tree
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// constructor, taking the selection the keys are made by

 explicit BBQueue( Selection s = Selection::best_first )
  : f_track( ( s == Selection::depth_first ) ||
             ( s == Selection::breadth_first ) ||
             ( s == Selection::best_estimate ) ) ,
    f_fifo( s == Selection::breadth_first ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// adds a node with its bound, its depth and its key

 void push( Node && n , double bound , unsigned depth , double key ) {
  std::size_t i;
  if( v_free.empty() ) {
   i = v_slot.size();
   v_slot.emplace_back();
   }
  else {
   i = v_free.back();
   v_free.pop_back();
   }
  const std::uint64_t seq = ++f_seq;
  v_slot[ i ].item.emplace( Item{ std::move( n ) , bound , depth } );
  v_slot[ i ].seq = seq;
  v_heap.push_back( Key{ key , seq , i } );
  std::push_heap( v_heap.begin() , v_heap.end() , order() );
  if( f_track ) {
   v_bounds.push_back( Key{ bound , seq , i } );
   std::push_heap( v_bounds.begin() , v_bounds.end() , after );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes and returns the next node

 Item pop( void ) {
  std::pop_heap( v_heap.begin() , v_heap.end() , order() );
  const std::size_t i = v_heap.back().idx;
  v_heap.pop_back();
  Item it = std::move( *v_slot[ i ].item );
  v_slot[ i ].item.reset();
  v_slot[ i ].seq = 0;
  v_free.push_back( i );
  return( it );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the least bound of the open nodes (+infinity if there is none)

 [[nodiscard]] double best_bound( void ) {
  if( ! f_track )
   return( v_heap.empty() ? std::numeric_limits< double >::infinity()
                          : v_slot[ v_heap.front().idx ].item->bound );
  while( ! v_bounds.empty() &&
         ( v_slot[ v_bounds.front().idx ].seq != v_bounds.front().seq ) ) {
   std::pop_heap( v_bounds.begin() , v_bounds.end() , after );
   v_bounds.pop_back();
   }
  return( v_bounds.empty() ? std::numeric_limits< double >::infinity()
                           : v_bounds.front().key );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes all the nodes, returning how many there were

 std::size_t clear( void ) {
  const std::size_t n = v_heap.size();
  v_slot.clear();
  v_free.clear();
  v_heap.clear();
  v_bounds.clear();
  return( n );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// true if there is no open node

 [[nodiscard]] bool empty( void ) const noexcept { return( v_heap.empty() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the number of open nodes

 [[nodiscard]] std::size_t size( void ) const noexcept {
  return( v_heap.size() );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 /// a slot: the node (empty if free) and the number of its push (0 if free)
 struct Slot {
  std::optional< Item > item;
  std::uint64_t seq = 0;
  };

 /// an entry of a heap
 struct Key {
  double key;
  std::uint64_t seq;
  std::size_t idx;
  };

 // the order of the heaps: a comes after b if its key is larger or, for
 // equal keys, if it was pushed earlier
 static bool after( const Key & a , const Key & b ) noexcept {
  return( ( a.key > b.key ) || ( ( a.key == b.key ) && ( a.seq < b.seq ) ) );
  }

 // the same, but with the earlier pushed first for equal keys
 static bool after_fifo( const Key & a , const Key & b ) noexcept {
  return( ( a.key > b.key ) || ( ( a.key == b.key ) && ( a.seq > b.seq ) ) );
  }

 // the order of the heap of the keys
 [[nodiscard]] auto order( void ) const noexcept {
  return( f_fifo ? after_fifo : after );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool f_track;                         ///< true if v_bounds is kept
 bool f_fifo;                          ///< first in, first out on ties
 std::uint64_t f_seq = 0;              ///< the pushes so far
 std::vector< Slot > v_slot;           ///< the slots
 std::vector< std::size_t > v_free;    ///< the free slots
 std::vector< Key > v_heap;            ///< the heap of the keys
 std::vector< Key > v_bounds;          ///< the heap of the bounds

 };  // end( class( BBQueue ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the evaluator of a problem without make_worker(): the problem itself

template< class P >
struct BBSelf {
 const P & p;
 Evaluation evaluate( typename P::node_type & n ) {
  return( p.evaluate( n ) );
  }
 template< class F >
 void branch( const typename P::node_type & n , F && f ) {
  p.branch( n , std::forward< F >( f ) );
  }
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the evaluator of the worker i: make_worker( i ), or the problem itself

template< BranchAndBound P >
auto bb_worker( const P & p , unsigned i ) {
 if constexpr( requires { p.make_worker( i ); } )
  return( p.make_worker( i ) );
 else {
  ( void ) i;
  return( BBSelf< P >{ p } );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// +1 for a minimization, -1 for a maximization

template< BranchAndBound P >
constexpr double bb_sign( void ) noexcept {
 return( P::sense == Sense::minimize ? 1.0 : -1.0 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// true if a node of the given bound cannot improve the incumbent inc (both
/// in the minimization) by more than the tolerance

inline bool bb_prunable( double bound , double inc , const BBOptions & o )
 noexcept
{
 if( ! std::isfinite( inc ) )
  return( bound == std::numeric_limits< double >::infinity() );
 return( bound >= inc - std::max( o.abs_gap , o.rel_gap * std::abs( inc ) ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the key of a child in the queue, by the selection

template< BranchAndBound P >
double bb_key( const P & p , const typename P::node_type & n , double bound ,
               unsigned depth , Selection s )
{
 switch( s ) {
  case( Selection::depth_first ):
   return( - double( depth ) );
  case( Selection::breadth_first ):
   return( double( depth ) );
  case( Selection::best_estimate ):
   if constexpr( EstimatingBranchAndBound< P > )
    return( bb_sign< P >() * double( p.estimate( n ) ) );
   else
    return( bound );
  default:
   return( bound );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// processes an extracted node: prunes it by the incumbent, evaluates it,
/// updates the incumbent, prunes it by its bound, and branches it, giving
/// each child that survives to push( child , bound , depth )
/** inc is the incumbent value (in the minimization), improve( v , n ) offers
 * the feasible value v of the node n, and low is lowered to the bound of
 * each node pruned by the tolerance, so that min( low , incumbent , open
 * bounds ) is a bound on the optimum. */

template< BranchAndBound P , class W , class Inc , class Improve ,
          class Push >
void bb_process( typename P::node_type & n , double bound , unsigned depth ,
                 W & w , Inc && inc , Improve && improve , Push && push ,
                 const BBOptions & o , BBStats & st , double & low )
{
 using Node = typename P::node_type;
 constexpr double sgn = bb_sign< P >();
 if( bb_prunable( bound , inc() , o ) ) {
  ++st.pruned_extracted;
  low = std::min( low , bound );
  return;
  }
 const Evaluation e = w.evaluate( n );
 ++st.evaluated;
 if( e.infeasible ) {
  ++st.infeasible;
  return;
  }
 const double b = std::max( bound , sgn * e.bound );
 if( e.feasible && improve( sgn * *e.feasible , n ) )
  ++st.incumbents;
 if( bb_prunable( b , inc() , o ) ) {
  ++st.pruned_evaluated;
  low = std::min( low , b );
  return;
  }
 ++st.branched;
 w.branch( std::as_const( n ) , [ & ]( auto && c , double est ) {
  ++st.generated;
  const double cb = std::max( b , sgn * est );
  if( bb_prunable( cb , inc() , o ) ) {
   ++st.pruned_generated;
   low = std::min( low , cb );
   return;
   }
  push( Node( std::forward< decltype( c ) >( c ) ) , cb , depth + 1 );
  } );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BranchAndBoundSearch -----------------------*/
/*--------------------------------------------------------------------------*/
/// the sequential best-first search with an incumbent
/** The nodes are extracted by the Selection of the options and evaluated
 * only when they are extracted, a child inheriting until then the bound of
 * its parent or its own estimate, whichever is larger; a node is pruned
 * when it is generated, when it is extracted and after its evaluation, and
 * the search ends when no open node can improve the incumbent by more than
 * the tolerance [see docs/branch-and-bound.md]. */

template< BranchAndBound P >
class BranchAndBoundSearch {

/*------------------------ PUBLIC PART OF THE CLASS ------------------------*/

 public:

/*------------------------------ PUBLIC TYPES ------------------------------*/

 using Node = typename P::node_type;            ///< the node type
 using SolutionT = BBSolution< Node >;          ///< the solution type

/*------------------------------ CONSTRUCTOR -------------------------------*/

 /// constructor, taking the problem (which must outlive the search)

 explicit BranchAndBoundSearch( const P & problem ,
                                const BBOptions & options = {} )
  : f_p( problem ) , f_options( options ) {}

/*-------------------------------- SOLVING ---------------------------------*/
 /// searches the tree, returning the Solution

 [[nodiscard]] SolutionT solve( void ) const
 {
  constexpr double sgn = detail::bb_sign< P >();
  constexpr double inf = std::numeric_limits< double >::infinity();
  const BBOptions & o = f_options;
  const Selection sel = o.selection;
  Stopwatch clock;
  SolutionT sol;
  BBStats & st = sol.stats;
  auto w = detail::bb_worker( f_p , 0u );

  double inc = o.incumbent ? sgn * *o.incumbent : inf;
  std::optional< Node > inc_node;
  const auto improve = [ & ]( double v , const Node & n ) {
   if( v >= inc )
    return( false );
   inc = v;
   inc_node = n;
   if( o.on_incumbent )
    o.on_incumbent( sgn * v );
   return( true );
   };

  using Item = typename BBQueue< Node >::Item;
  BBQueue< Node > q( sel );
  std::optional< Item > dive;              // the node of the dive, if any
  std::vector< Item > kids;                // the children of a node
  double low = inf;                        // bounds of the pruned nodes

  const auto push = [ & ]( Item && it ) {
   const double key = detail::bb_key( f_p , it.node , it.bound , it.depth ,
                                      sel );
   q.push( std::move( it.node ) , it.bound , it.depth , key );
   st.max_open = std::max( st.max_open , q.size() );
   };

  push( Item{ f_p.root() , -inf , 0 } );
  sol.status = BBStatus::optimal;

  for( ;; ) {
   if( o.node_limit && ( st.evaluated >= o.node_limit ) ) {
    sol.status = BBStatus::node_limit;
    break;
    }
   if( ( o.time_limit > 0 ) && ( clock.seconds() >= o.time_limit ) ) {
    sol.status = BBStatus::time_limit;
    break;
    }
   const double best = std::min( q.best_bound() ,
                                 dive ? dive->bound : inf );
   if( q.empty() && ! dive )
    break;
   if( detail::bb_prunable( best , inc , o ) ) {   // nothing can improve
    st.pruned_extracted += q.clear() + ( dive ? 1 : 0 );
    dive.reset();
    low = std::min( low , best );
    break;
    }

   Item it = dive ? std::move( *dive ) : q.pop();
   dive.reset();
   kids.clear();
   detail::bb_process< P >( it.node , it.bound , it.depth , w ,
                            [ & ] { return( inc ); } , improve ,
                            [ & ]( Node && c , double b , unsigned d ) {
                             kids.push_back( Item{ std::move( c ) , b , d } );
                             } , o , st , low );
   if( kids.empty() )
    continue;

   // with plunging, the best child dives if it is close to the best bound
   std::size_t k = kids.size();
   if( sel == Selection::plunging ) {
    std::size_t c = 0;
    for( std::size_t j = 1 ; j < kids.size() ; ++j )
     if( kids[ j ].bound <= kids[ c ].bound )
      c = j;
    const double b0 = std::min( q.best_bound() , kids[ c ].bound );
    if( ! std::isfinite( inc ) ||
        ( kids[ c ].bound <= b0 + o.plunge_gap * ( inc - b0 ) ) )
     k = c;
    }
   for( std::size_t j = 0 ; j < kids.size() ; ++j )
    if( j == k )
     dive.emplace( std::move( kids[ j ] ) );
    else
     push( std::move( kids[ j ] ) );
   }

  // the open nodes left by a limit bound the optimum too
  low = std::min( { low , q.best_bound() , dive ? dive->bound : inf } );
  if( ( sol.status == BBStatus::optimal ) && ! std::isfinite( inc ) )
   sol.status = BBStatus::infeasible;
  sol.node = std::move( inc_node );
  if( std::isfinite( inc ) )
   sol.value = sgn * inc;
  const double lb = std::min( inc , low );
  sol.bound = sgn * lb;
  sol.gap = std::isfinite( inc ) ? inc - lb : inf;
  st.evaluated_by = { st.evaluated };
  st.seconds = clock.seconds();
  return( sol );
  }

/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/

 private:

 const P & f_p;                                       ///< the problem
 BBOptions f_options;                                 ///< the options

 };  // end( class( BranchAndBoundSearch ) )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
/// the sequential B&B of a problem, with the given options

template< BranchAndBound P >
[[nodiscard]] BBSolution< typename P::node_type > branch_and_bound(
                              const P & problem , const BBOptions & o = {} )
{
 return( BranchAndBoundSearch< P >( problem , o ).solve() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace AStar_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* __AStarSearch_BranchAndBound */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BranchAndBound.h ------------------------*/
/*--------------------------------------------------------------------------*/

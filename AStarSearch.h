/**
 * Artificial Intelligence: A Modern Approach, by Russel and Norvig.
 *
 * The most widely known form of best-first search is called A* Search.
 * It evaluates nodes by combining g(n), the cost to reach the node, and h(n),
 * the cost to get from the node to the goal: f(n) = g(n) + h(n).
 * Since g(n) gives the path cost from the start node to node n, and h(n) is
 * the estimated cost of the cheapest path from n to the goal, we have
 * f(n) = estimated cost of the cheapest solution through n.
 */

#ifndef A_STAR_SEARCH_H
#define A_STAR_SEARCH_H

#include <vector>
#include <algorithm>

using namespace std;

template< class AStarState >
class AStarSearch
{

 public:

 enum STATE
 {
  SEARCH_STATE_SEARCHING ,
  SEARCH_STATE_SUCCEEDED ,
  SEARCH_STATE_FAILED ,
  SEARCH_STATE_OUT_OF_MEMORY
 };

 class Node
 {

  public:

  Node * parent;
  Node * child;

  float g;
  float h;
  float f;

  AStarState a_star_state;

  Node() :
   parent( nullptr ) , child( nullptr ) ,
   g( 0.0f ) , h( 0.0f ) , f( 0.0f ) {}
 };

 class HeapCompare
 {

  public:

  bool operator()( const Node * x , const Node * y ) const {
   return x->f > y->f;
  }

 };

 AStarSearch() :
  current_solution_node( nullptr ) ,
  state( 0 ) , steps( 0 ) {}

 void set_start_goal_states( AStarState & start_state ,
                             AStarState & goal_state ) {
  start = new Node;
  goal = new Node;
  start->a_star_state = start_state;
  goal->a_star_state = goal_state;
  state = SEARCH_STATE_SEARCHING;
  start->g = 0.0f;
  start->h = start->a_star_state.goal_distance_estimate( goal->a_star_state );
  start->f = start->g + start->h;
  start->parent = nullptr;
  open_list.push_back( start );
  push_heap( open_list.begin() , open_list.end() , HeapCompare() );
  steps = 0;
 }

 unsigned int search_step() {
  if( state == SEARCH_STATE_SUCCEEDED || state == SEARCH_STATE_FAILED )
   return state;
  if( open_list.empty() ) {
   free_all_nodes();
   state = SEARCH_STATE_FAILED;
   return state;
  }
  steps++;
  Node * first = open_list.front();
  pop_heap( open_list.begin() , open_list.end() , HeapCompare() );
  open_list.pop_back();
  if( first->a_star_state.is_goal( goal->a_star_state ) ) {
   goal->parent = first->parent;
   goal->g = first->g;
   if( !first->a_star_state.is_same_state( start->a_star_state ) ) {
    delete first;
    Node * nodeChild = goal;
    Node * nodeParent = goal->parent;
    do {
     nodeParent->child = nodeChild;
     nodeChild = nodeParent;
     nodeParent = nodeParent->parent;
    } while( nodeChild != start );
   }
   free_unused_nodes();
   state = SEARCH_STATE_SUCCEEDED;
   return state;
  } else {
   successors.clear();
   if( !first->a_star_state.get_successors( this , first->parent
                                                   ? &first->parent->a_star_state
                                                   : nullptr ) ) {
    typename vector< Node * >::iterator iter_succ;
    for( iter_succ = successors.begin() ;
         iter_succ != successors.end() ; iter_succ++ )
     delete *iter_succ;
    successors.clear();
    free_all_nodes();
    state = SEARCH_STATE_OUT_OF_MEMORY;
    return state;
   }
   typename vector< Node * >::iterator iter_succ;
   for( iter_succ = successors.begin() ;
        iter_succ != successors.end() ; iter_succ++ ) {
    float g =
     first->g + first->a_star_state.get_cost( ( *iter_succ )->a_star_state );
    typename vector< Node * >::iterator iter_open;
    for( iter_open = open_list.begin() ;
         iter_open != open_list.end() ; iter_open++ ) {
     if( ( *iter_open )->a_star_state.is_same_state( ( *iter_succ )
     ->a_star_state
     ) )
      break;
    }
    if( iter_open != open_list.end() ) {
     if( ( *iter_open )->g <= g ) {
      delete *iter_succ;
      continue;
     }
    }
    typename vector< Node * >::iterator iter_closed;
    for( iter_closed = closed_list.begin() ;
         iter_closed != closed_list.end() ; iter_closed++ ) {
     if( ( *iter_closed )->a_star_state.is_same_state(
      ( *iter_succ )->a_star_state ) )
      break;
    }
    if( iter_closed != closed_list.end() ) {
     if( ( *iter_closed )->g <= g ) {
      delete *iter_succ;
      continue;
     }
    }
    ( *iter_succ )->parent = first;
    ( *iter_succ )->g = g;
    ( *iter_succ )->h = ( *iter_succ )->a_star_state.goal_distance_estimate(
     goal->a_star_state );
    ( *iter_succ )->f = ( *iter_succ )->g + ( *iter_succ )->h;
    if( iter_closed != closed_list.end() ) {
     delete *iter_closed;
     closed_list.erase( iter_closed );
    }
    if( iter_open != open_list.end() ) {
     delete *iter_open;
     open_list.erase( iter_open );
     make_heap( open_list.begin() , open_list.end() , HeapCompare() );
    }
    open_list.push_back( *iter_succ );
    push_heap( open_list.begin() , open_list.end() , HeapCompare() );
   }
   closed_list.push_back( first );
  }
  return state;
 }

 bool add_successor( AStarState & state ) {
  Node * node = new Node;
  if( node ) {
   node->a_star_state = state;
   successors.push_back( node );
   return true;
  }
  return false;
 }

 void free_solution_nodes() {
  Node * node = start;
  if( start->child ) {
   do {
    Node * del = node;
    node = node->child;
    delete del;
   } while( node != goal );
   delete node;
  } else {
   delete start;
   delete goal;
  }
 }

 AStarState * get_solution_start() {
  current_solution_node = start;
  if( start )
   return &start->a_star_state;
  else
   return nullptr;
 }

 AStarState * get_solution_next() {
  if( current_solution_node ) {
   if( current_solution_node->child ) {
    Node * child = current_solution_node->child;
    current_solution_node = current_solution_node->child;
    return &child->a_star_state;
   }
  }
  return nullptr;
 }

 AStarState * get_solution_end() {
  current_solution_node = goal;
  if( goal )
   return &goal->a_star_state;
  else
   return nullptr;
 }

 AStarState * get_solution_prev() {
  if( current_solution_node ) {
   if( current_solution_node->parent ) {
    Node * parent = current_solution_node->parent;
    current_solution_node = current_solution_node->parent;
    return &parent->a_star_state;
   }
  }
  return nullptr;
 }

 int get_step_count() {
  return steps;
 }

 private:

 vector< Node * > open_list;
 vector< Node * > closed_list;
 vector< Node * > successors;

 unsigned int state;
 int steps;

 Node * start;
 Node * goal;
 Node * current_solution_node;

 void free_unused_nodes() {
  typename vector< Node * >::iterator iter_open;
  for( iter_open = open_list.begin() ;
       iter_open != open_list.end() ; iter_open++ ) {
   if( !( *iter_open )->child )
    delete *iter_open;
  }
  open_list.clear();
  typename vector< Node * >::iterator iter_closed;
  for( iter_closed = closed_list.begin() ;
       iter_closed != closed_list.end() ; iter_closed++ ) {
   if( !( *iter_closed )->child )
    delete *iter_closed;
  }
  closed_list.clear();
 }

 void free_all_nodes() {
  typename vector< Node * >::iterator iter_open;
  for( iter_open = open_list.begin() ;
       iter_open != open_list.end() ; iter_open++ ) {
   delete *iter_open;
  }
  open_list.clear();
  typename vector< Node * >::iterator iter_closed;
  for( iter_closed = closed_list.begin() ;
       iter_closed != closed_list.end() ; iter_closed++ ) {
   delete *iter_closed;
  }
  closed_list.clear();
  delete goal;
 }
};

#endif // A_STAR_SEARCH_H
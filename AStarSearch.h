/**
 * Artificial Intelligence: A Modern Approach, by Russel and Norvig.
 *
 * The most widely known form of best-first search is called A* Search.
 * It evaluates nodes by combining g(n), the cost to reach the node, and h(n),
 * the cost to get from the node to the goal: f(n) = g(n) + h(n).
 * Since g(n) gives the path cost from the start node to node n, and h(n) is
 * the estimated cost of the cheapest path from n to the goal, we have
 * f(n) = estimated cost of the cheapest solution through n.
 *
 * @author Donato Meoli
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

  AStarState aStarState;

  Node() {
   parent = nullptr;
   child = nullptr;
   g = 0.0f;
   h = 0.0f;
   f = 0.0f;
  }
 };

 class HeapCompare
 {

  public:

  bool operator()( const Node * x , const Node * y ) const {
   return x->f > y->f;
  }

 };

 AStarSearch() {
  currentSolutionNode = nullptr;
  state = 0;
  steps = 0;
 }

 void setStartAndGoalStates( AStarState & startState ,
                             AStarState & goalState ) {
  start = new Node;
  goal = new Node;
  start->aStarState = startState;
  goal->aStarState = goalState;
  state = SEARCH_STATE_SEARCHING;
  start->g = 0.0f;
  start->h = start->aStarState.goalDistanceEstimate( goal->aStarState );
  start->f = start->g + start->h;
  start->parent = nullptr;
  openList.push_back( start );
  push_heap( openList.begin() , openList.end() , HeapCompare() );
  steps = 0;
 }

 unsigned int searchStep() {
  if( state == SEARCH_STATE_SUCCEEDED || state == SEARCH_STATE_FAILED )
   return state;
  if( openList.empty() ) {
   freeAllNodes();
   state = SEARCH_STATE_FAILED;
   return state;
  }
  steps++;
  Node * first = openList.front();
  pop_heap( openList.begin() , openList.end() , HeapCompare() );
  openList.pop_back();
  if( first->aStarState.isGoal( goal->aStarState ) ) {
   goal->parent = first->parent;
   goal->g = first->g;
   if( !first->aStarState.isSameState( start->aStarState ) ) {
    delete first;
    Node * nodeChild = goal;
    Node * nodeParent = goal->parent;
    do {
     nodeParent->child = nodeChild;
     nodeChild = nodeParent;
     nodeParent = nodeParent->parent;
    } while( nodeChild != start );
   }
   freeUnusedNodes();
   state = SEARCH_STATE_SUCCEEDED;
   return state;
  } else {
   successors.clear();
   if( !first->aStarState.getSuccessors( this , first->parent
                                                ? &first->parent->aStarState
                                                : nullptr ) ) {
    typename vector< Node * >::iterator iterSucc;
    for( iterSucc = successors.begin() ;
         iterSucc != successors.end() ; iterSucc++ )
     delete *iterSucc;
    successors.clear();
    freeAllNodes();
    state = SEARCH_STATE_OUT_OF_MEMORY;
    return state;
   }
   typename vector< Node * >::iterator iterSucc;
   for( iterSucc = successors.begin() ;
        iterSucc != successors.end() ; iterSucc++ ) {
    float g = first->g + first->aStarState.getCost( ( *iterSucc )->aStarState );
    typename vector< Node * >::iterator iterOpen;
    for( iterOpen = openList.begin() ;
         iterOpen != openList.end() ; iterOpen++ ) {
     if( ( *iterOpen )->aStarState.isSameState( ( *iterSucc )->aStarState ) )
      break;
    }
    if( iterOpen != openList.end() ) {
     if( ( *iterOpen )->g <= g ) {
      delete *iterSucc;
      continue;
     }
    }
    typename vector< Node * >::iterator iterClosed;
    for( iterClosed = closedList.begin() ;
         iterClosed != closedList.end() ; iterClosed++ ) {
     if( ( *iterClosed )->aStarState.isSameState( ( *iterSucc )->aStarState ) )
      break;
    }
    if( iterClosed != closedList.end() ) {
     if( ( *iterClosed )->g <= g ) {
      delete *iterSucc;
      continue;
     }
    }
    ( *iterSucc )->parent = first;
    ( *iterSucc )->g = g;
    ( *iterSucc )->h = ( *iterSucc )->aStarState.goalDistanceEstimate(
     goal->aStarState );
    ( *iterSucc )->f = ( *iterSucc )->g + ( *iterSucc )->h;
    if( iterClosed != closedList.end() ) {
     delete *iterClosed;
     closedList.erase( iterClosed );
    }
    if( iterOpen != openList.end() ) {
     delete *iterOpen;
     openList.erase( iterOpen );
     make_heap( openList.begin() , openList.end() , HeapCompare() );
    }
    openList.push_back( *iterSucc );
    push_heap( openList.begin() , openList.end() , HeapCompare() );
   }
   closedList.push_back( first );
  }
  return state;
 }

 bool addSuccessor( AStarState & state ) {
  Node * node = new Node;
  if( node ) {
   node->aStarState = state;
   successors.push_back( node );
   return true;
  }
  return false;
 }

 void freeSolutionNodes() {
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

 AStarState * getSolutionStart() {
  currentSolutionNode = start;
  if( start )
   return &start->aStarState;
  else
   return nullptr;
 }

 AStarState * getSolutionNext() {
  if( currentSolutionNode ) {
   if( currentSolutionNode->child ) {
    Node * child = currentSolutionNode->child;
    currentSolutionNode = currentSolutionNode->child;
    return &child->aStarState;
   }
  }
  return nullptr;
 }

 AStarState * getSolutionEnd() {
  currentSolutionNode = goal;
  if( goal )
   return &goal->aStarState;
  else
   return nullptr;
 }

 AStarState * getSolutionPrev() {
  if( currentSolutionNode ) {
   if( currentSolutionNode->parent ) {
    Node * parent = currentSolutionNode->parent;
    currentSolutionNode = currentSolutionNode->parent;
    return &parent->aStarState;
   }
  }
  return nullptr;
 }

 int getStepCount() {
  return steps;
 }

 private:

 vector< Node * > openList;
 vector< Node * > closedList;
 vector< Node * > successors;

 unsigned int state;
 int steps;

 Node * start;
 Node * goal;
 Node * currentSolutionNode;

 void freeUnusedNodes() {
  typename vector< Node * >::iterator iterOpen;
  for( iterOpen = openList.begin() ; iterOpen != openList.end() ; iterOpen++ ) {
   if( !( *iterOpen )->child )
    delete *iterOpen;
  }
  openList.clear();
  typename vector< Node * >::iterator iterClosed;
  for( iterClosed = closedList.begin() ;
       iterClosed != closedList.end() ; iterClosed++ ) {
   if( !( *iterClosed )->child )
    delete *iterClosed;
  }
  closedList.clear();
 }

 void freeAllNodes() {
  typename vector< Node * >::iterator iterOpen;
  for( iterOpen = openList.begin() ; iterOpen != openList.end() ; iterOpen++ ) {
   delete *iterOpen;
  }
  openList.clear();
  typename vector< Node * >::iterator iterClosed;
  for( iterClosed = closedList.begin() ;
       iterClosed != closedList.end() ; iterClosed++ ) {
   delete *iterClosed;
  }
  closedList.clear();
  delete goal;
 }
};

#endif // A_STAR_SEARCH_H
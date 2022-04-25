#ifndef A_STAR_SEARCH_MAP_SEARCH_NODE_H
#define A_STAR_SEARCH_MAP_SEARCH_NODE_H

#include <cmath>
#include <iostream>

#include "../AStarSearch.h"
#include "../AStarState.h"

class MapSearchNode : public AStarState< MapSearchNode >
{

 public:

 static const int MAP_WIDTH = 20;
 static const int MAP_HEIGHT = 20;

 int world_map[MAP_WIDTH * MAP_HEIGHT] = {
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 ,  // 00
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,
  1 ,  // 01
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 ,
  1 ,  // 02
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 ,
  1 ,  // 03
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 ,
  1 ,  // 04
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 ,
  1 ,  // 05
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 ,
  1 ,  // 06
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,
  1 ,  // 07
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 ,  // 08
  1 , 9 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,
  1 ,  // 09
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 ,  // 10
  1 , 9 , 9 , 9 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 1 ,
  1 ,  // 11
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 ,
  1 ,  // 12
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 ,
  1 ,  // 13
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 ,
  1 ,  // 14
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 ,
  1 ,  // 15
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 ,
  1 ,  // 16
  1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 9 , 9 , 9 , 1 , 9 , 9 , 9 ,
  9 ,  // 17
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 ,  // 18
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1    // 19
 };

 MapSearchNode() : x( 0 ) , y( 0 ) {}

 MapSearchNode( unsigned int x , unsigned int y ) : x( x ) , y( y ) {}

 int get_map( int x , int y ) {
  if( x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT )
   return 9;
  return world_map[ ( y * MAP_WIDTH ) + x ];
 }

 float goal_distance_estimate( MapSearchNode & goal_node ) override {
  return abs( x - goal_node.x ) + abs( y - goal_node.y );
 }

 bool is_goal( MapSearchNode & goal_node ) override {
  return x == goal_node.x && y == goal_node.y;
 }

 bool get_successors( AStarSearch< MapSearchNode > * a_star_search ,
                      MapSearchNode * parent_node ) override {
  int parentX = -1;
  int parentY = -1;
  if( parent_node ) {
   parentX = parent_node->x;
   parentY = parent_node->y;
  }

  MapSearchNode map_search_node;

  // push each possible move except allowing the search to go backwards

  if( ( get_map( x - 1 , y ) < 9 )
      && !( ( parentX == x - 1 ) && ( parentY == y ) )
   ) {
   map_search_node = MapSearchNode( x - 1 , y );
   a_star_search->add_successor( map_search_node );
  }

  if( ( get_map( x , y - 1 ) < 9 )
      && !( ( parentX == x ) && ( parentY == y - 1 ) )
   ) {
   map_search_node = MapSearchNode( x , y - 1 );
   a_star_search->add_successor( map_search_node );
  }

  if( ( get_map( x + 1 , y ) < 9 )
      && !( ( parentX == x + 1 ) && ( parentY == y ) )
   ) {
   map_search_node = MapSearchNode( x + 1 , y );
   a_star_search->add_successor( map_search_node );
  }
  if( ( get_map( x , y + 1 ) < 9 )
      && !( ( parentX == x ) && ( parentY == y + 1 ) )
   ) {
   map_search_node = MapSearchNode( x , y + 1 );
   a_star_search->add_successor( map_search_node );
  }

  return true;
 }

 float get_cost( MapSearchNode & successor ) override {
  return ( float ) get_map( x , y );
 }

 bool is_same_state( MapSearchNode & rhs ) override {
  return x == rhs.x && y == rhs.y;
 }

 void print_node_info() const {
  char str[100];
  sprintf( str , "Node position: (%d,%d)" , x , y );
  cout << str << endl;
 }

 private:

 int x;
 int y;
};

#endif // A_STAR_SEARCH_MAP_SEARCH_NODE_H

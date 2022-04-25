#ifndef A_STAR_SEARCH_PATH_SEARCH_NODE_H
#define A_STAR_SEARCH_PATH_SEARCH_NODE_H

#include <string>
#include <iostream>

#include "../AStarSearch.h"
#include "../AStarState.h"

const int MAX_CITIES = 20;

enum CITIES
{
 Arad ,
 Bucharest ,
 Craiova ,
 Drobeta ,
 Eforie ,
 Fagaras ,
 Giurgiu ,
 Hirsova ,
 Iasi ,
 Lugoj ,
 Mehadia ,
 Neamt ,
 Oradea ,
 Pitesti ,
 RimnicuVilcea ,
 Sibiu ,
 Timisoara ,
 Urziceni ,
 Vaslui ,
 Zerind
};

vector< string > city_names( MAX_CITIES );

float romania_map[MAX_CITIES][MAX_CITIES];

class PathSearchNode : public AStarState< PathSearchNode >
{

 public:

 CITIES city;

 PathSearchNode() : city( Arad ) {}

 explicit PathSearchNode( CITIES in ) : city( in ) {}

 float goal_distance_estimate( PathSearchNode & goal_node ) override {
  switch( city ) {
   case Arad:
    return 366;
   case Bucharest:
    return 0;
   case Craiova:
    return 160;
   case Drobeta:
    return 242;
   case Eforie:
    return 161;
   case Fagaras:
    return 176;
   case Giurgiu:
    return 77;
   case Hirsova:
    return 151;
   case Iasi:
    return 226;
   case Lugoj:
    return 244;
   case Mehadia:
    return 241;
   case Neamt:
    return 234;
   case Oradea:
    return 380;
   case Pitesti:
    return 100;
   case RimnicuVilcea:
    return 193;
   case Sibiu:
    return 253;
   case Timisoara:
    return 329;
   case Urziceni:
    return 80;
   case Vaslui:
    return 199;
   case Zerind:
    return 374;
   default:
    return 0.0f;
  }
 }

 bool is_goal( PathSearchNode & goal_node ) override {
  return city == Bucharest;
 }

 bool get_successors( AStarSearch< PathSearchNode > * a_star_search ,
                      PathSearchNode * parent_node ) override {
  PathSearchNode path_search_node;
  for( int c = 0 ; c < MAX_CITIES ; c++ ) {
   if( romania_map[ city ][ c ] < 0 )
    continue;
   path_search_node = PathSearchNode( ( CITIES ) c );
   a_star_search->add_successor( path_search_node );
  }
  return true;
 }

 float get_cost( PathSearchNode & successor ) override {
  return romania_map[ city ][ successor.city ];
 }

 bool is_same_state( PathSearchNode & rhs ) override {
  return city == rhs.city;
 }

 void print_node_info() const {
  cout << " " << city_names[ city ] << endl;
 }
};

#endif // A_STAR_SEARCH_PATH_SEARCH_NODE_H

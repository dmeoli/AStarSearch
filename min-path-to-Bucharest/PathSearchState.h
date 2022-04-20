#ifndef A_STAR_SEARCH_PATH_SEARCH_STATE_H
#define A_STAR_SEARCH_PATH_SEARCH_STATE_H

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

vector< string > cityNames( MAX_CITIES );

float romaniaMap[MAX_CITIES][MAX_CITIES];

class PathSearchState : public AStarState< PathSearchState >
{

 public:

 CITIES city;

 PathSearchState() {
  city = Arad;
 }

 explicit PathSearchState( CITIES in ) {
  this->city = in;
 }

 float goalDistanceEstimate( PathSearchState & nodeGoal ) override {
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

 bool isGoal( PathSearchState & nodeGoal ) override {
  return city == Bucharest;
 }

 bool getSuccessors( AStarSearch< PathSearchState > * aStarSearch ,
                     PathSearchState * parentNode ) override {
  PathSearchState pathSearchState;
  for( int c = 0 ; c < MAX_CITIES ; c++ ) {
   if( romaniaMap[ city ][ c ] < 0 )
    continue;
   pathSearchState = PathSearchState( ( CITIES ) c );
   aStarSearch->addSuccessor( pathSearchState );
  }
  return true;
 }

 float getCost( PathSearchState & successor ) override {
  return romaniaMap[ city ][ successor.city ];
 }

 bool isSameState( PathSearchState & rhs ) override {
  return city == rhs.city;
 }

 void printNodeInfo() const {
  cout << " " << cityNames[ city ] << endl;
 }
};

#endif // A_STAR_SEARCH_PATH_SEARCH_STATE_H

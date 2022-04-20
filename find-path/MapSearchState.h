#ifndef MAP_SEARCH_STATE_H
#define MAP_SEARCH_STATE_H

#include <cmath>
#include <iostream>
#include "../AStarSearch.h"
#include "../AStarState.h"

class MapSearchState : public AStarState< MapSearchState >
{

 public:

 static const int MAP_WIDTH = 20;
 static const int MAP_HEIGHT = 20;

 int worldMap[MAP_WIDTH * MAP_HEIGHT] = {
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,   // 00
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,
  9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,   // 01
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,   // 02
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,   // 03
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 1 ,   // 04
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 ,
  1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 ,   // 05
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,   // 06
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,
  1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,   // 07
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,   // 08
  1 , 9 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,
  1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,   // 09
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 9 ,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,   // 10
  1 , 9 , 9 , 9 , 9 , 9 , 1 , 9 , 1 , 9 ,
  1 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 ,   // 11
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,   // 12
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,   // 13
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 ,
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 1 ,   // 14
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 ,
  1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 ,   // 15
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,   // 16
  1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,
  1 , 1 , 9 , 9 , 9 , 1 , 9 , 9 , 9 , 9 ,   // 17
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,   // 18
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1    // 19
 };

 int getMap( int x , int y ) {
  if( x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT ) return 9;
  return worldMap[ ( y * MAP_WIDTH ) + x ];
 }

 MapSearchState() {
  x = 0;
  y = 0;
 }

 MapSearchState( int x , int y ) {
  this->x = x;
  this->y = y;
 }

 float goalDistanceEstimate( MapSearchState & nodeGoal ) override {
  return fabs( x - nodeGoal.x ) + fabs( y - nodeGoal.y );
 }

 bool isGoal( MapSearchState & nodeGoal ) override {
  return x == nodeGoal.x && y == nodeGoal.y;
 }

 bool getSuccessors( AStarSearch< MapSearchState > * aStarSearch ,
                     MapSearchState * parentNode ) override {
  int parentX = -1;
  int parentY = -1;
  if( parentNode ) {
   parentX = parentNode->x;
   parentY = parentNode->y;
  }
  MapSearchState mapSearchState;
  if( getMap( x - 1 , y ) < 9 && !( parentX == x - 1 && parentY == y ) ) {
   mapSearchState = MapSearchState( x - 1 , y );
   aStarSearch->addSuccessor( mapSearchState );
  }
  if( getMap( x , y - 1 < 9 ) && !( parentX == x && parentY == y - 1 ) ) {
   mapSearchState = MapSearchState( x , y - 1 );
   aStarSearch->addSuccessor( mapSearchState );
  }
  if( getMap( x + 1 , y ) < 9 && !( parentX == x + 1 && parentY == y ) ) {
   mapSearchState = MapSearchState( x + 1 , y );
   aStarSearch->addSuccessor( mapSearchState );
  }
  if( getMap( x , y + 1 ) < 9 && !( parentX == x && parentY == y + 1 ) ) {
   mapSearchState = MapSearchState( x , y + 1 );
   aStarSearch->addSuccessor( mapSearchState );
  }
  return true;
 }

 float getCost( MapSearchState & successor ) override {
  return ( float ) getMap( x , y );
 }

 bool isSameState( MapSearchState & rhs ) override {
  return x == rhs.x && y == rhs.y;
 }

 void printNodeInfo() const {
  char str[100];
  sprintf( str , "Node position: (%d,%d)" , x , y );
  cout << str << endl;
 }

 private:

 int x;
 int y;
};

#endif

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

 int worldMap[MAP_WIDTH * MAP_HEIGHT] = {
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,  // 00
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,  // 01
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,  // 02
  1 , 9 , 9 , 1 , 1 , 9 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,  // 03
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 1 ,  // 04
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 ,1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 ,  // 05
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,  // 06
  1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,  // 07
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,  // 08
  1 , 9 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 ,1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,  // 09
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 1 , 1 , 9 ,1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,  // 10
  1 , 9 , 9 , 9 , 9 , 9 , 1 , 9 , 1 , 9 ,1 , 9 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 ,  // 11
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,  // 12
  1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 9 , 1 , 9 , 9 , 9 , 1 , 1 ,  // 13
  1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 9 ,1 , 9 , 1 , 1 , 1 , 1 , 9 , 9 , 1 , 1 ,  // 14
  1 , 9 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 9 ,1 , 1 , 1 , 1 , 9 , 1 , 1 , 1 , 1 , 1 ,  // 15
  1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,1 , 9 , 9 , 9 , 9 , 1 , 1 , 1 , 1 , 1 ,  // 16
  1 , 1 , 9 , 9 , 9 , 9 , 9 , 9 , 9 , 1 ,1 , 1 , 9 , 9 , 9 , 1 , 9 , 9 , 9 , 9 ,  // 17
  1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,1 , 9 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,  // 18
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1    // 19
 };

 int getMap( int x , int y ) {
  if( x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT )
   return 9;
  return worldMap[ ( y * MAP_WIDTH ) + x ];
 }

 MapSearchNode() {
  x = 0;
  y = 0;
 }

 MapSearchNode( unsigned int x , unsigned int y ) {
  this->x = x;
  this->y = y;
 }

 float goalDistanceEstimate( MapSearchNode & nodeGoal ) override {
  return abs( x - nodeGoal.x ) + abs( y - nodeGoal.y );
 }

 bool isGoal( MapSearchNode & nodeGoal ) override {
  return x == nodeGoal.x && y == nodeGoal.y;
 }

 bool getSuccessors( AStarSearch< MapSearchNode > * aStarSearch ,
                     MapSearchNode * parentNode ) override {
  int parentX = -1;
  int parentY = -1;
  if( parentNode ) {
   parentX = parentNode->x;
   parentY = parentNode->y;
  }

  MapSearchNode mapSearchNode;

  // push each possible move except allowing the search to go backwards

  if( ( getMap( x - 1 , y ) < 9 )
      && !( ( parentX == x - 1 ) && ( parentY == y ) )
   ) {
   mapSearchNode = MapSearchNode( x - 1 , y );
   aStarSearch->addSuccessor( mapSearchNode );
  }

  if( ( getMap( x , y - 1 ) < 9 )
      && !( ( parentX == x ) && ( parentY == y - 1 ) )
   ) {
   mapSearchNode = MapSearchNode( x , y - 1 );
   aStarSearch->addSuccessor( mapSearchNode );
  }

  if( ( getMap( x + 1 , y ) < 9 )
      && !( ( parentX == x + 1 ) && ( parentY == y ) )
   ) {
   mapSearchNode = MapSearchNode( x + 1 , y );
   aStarSearch->addSuccessor( mapSearchNode );
  }
  if( ( getMap( x , y + 1 ) < 9 )
      && !( ( parentX == x ) && ( parentY == y + 1 ) )
   ) {
   mapSearchNode = MapSearchNode( x , y + 1 );
   aStarSearch->addSuccessor( mapSearchNode );
  }

  return true;
 }

 float getCost( MapSearchNode & successor ) override {
  return ( float ) getMap( x , y );
 }

 bool isSameState( MapSearchNode & rhs ) override {
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

#endif // A_STAR_SEARCH_MAP_SEARCH_NODE_H

#ifndef A_STAR_SEARCH_PUZZLE_STATE_H
#define A_STAR_SEARCH_PUZZLE_STATE_H

#include <cstring>
#include <iostream>
#include "../AStarSearch.h"
#include "../AStarState.h"

#define BOARD_WIDTH 3
#define BOARD_HEIGHT 3

#define GM_TILE -1
#define GM_SPACE 0
#define GM_OFF_BOARD 1

class PuzzleState : public AStarState< PuzzleState >
{

 public:

 enum TILE
 {
  TL_SPACE ,
  TL_1 ,
  TL_2 ,
  TL_3 ,
  TL_4 ,
  TL_5 ,
  TL_6 ,
  TL_7 ,
  TL_8
 };

 static TILE goal[BOARD_WIDTH * BOARD_HEIGHT];
 static TILE start[BOARD_WIDTH * BOARD_HEIGHT];

 TILE tiles[BOARD_WIDTH * BOARD_HEIGHT]{};

 PuzzleState() {
  memcpy( tiles , goal , sizeof( TILE ) * BOARD_WIDTH * BOARD_HEIGHT );
 }

 explicit PuzzleState( TILE * paramTiles ) {
  memcpy( tiles , paramTiles , sizeof( TILE ) * BOARD_WIDTH * BOARD_HEIGHT );
 }

 float goalDistanceEstimate( PuzzleState & nodeGoal ) override {
  int i , cx , cy , ax , ay , h = 0 , s , t;
  TILE correctFollowerTo[BOARD_WIDTH * BOARD_HEIGHT] = {
   TL_SPACE ,
   TL_2 ,
   TL_3 ,
   TL_4 ,
   TL_5 ,
   TL_6 ,
   TL_7 ,
   TL_8 ,
   TL_1
  };
  int clockwiseTileOf[BOARD_WIDTH * BOARD_HEIGHT] = {
   1 ,
   2 ,
   5 ,
   0 ,
   -1 ,
   8 ,
   3 ,
   6 ,
   7
  };
  int tileX[BOARD_WIDTH * BOARD_HEIGHT] = {
   1 ,
   0 ,
   1 ,
   2 ,
   2 ,
   2 ,
   1 ,
   0 ,
   0
  };
  int tileY[BOARD_WIDTH * BOARD_HEIGHT] = {
   1 ,
   0 ,
   0 ,
   0 ,
   1 ,
   2 ,
   2 ,
   2 ,
   1
  };
  s = 0;
  if( tiles[ ( BOARD_HEIGHT * BOARD_WIDTH ) / 2 ] !=
      nodeGoal.tiles[ ( BOARD_HEIGHT * BOARD_WIDTH ) / 2 ] )
   s = 1;
  for( i = 0 ; i < ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
   if( tiles[ i ] == TL_SPACE )
    continue;
   cx = tileX[ tiles[ i ] ];
   cy = tileY[ tiles[ i ] ];
   ax = i % BOARD_WIDTH;
   ay = i / BOARD_WIDTH;
   // Manhattan distance
   h += abs( cx - ax ) + abs( cy - ay );
   if( ax == ( BOARD_WIDTH / 2 ) && ay == ( BOARD_HEIGHT / 2 ) )
    continue;
   if( correctFollowerTo[ tiles[ i ] ] != tiles[ clockwiseTileOf[ i ] ] )
    s += 2;
  }
  t = h + ( 3 * s );
  return ( float ) t;
 }

 bool isGoal( PuzzleState & nodeGoal ) override {
  return isSameState( nodeGoal );
 }

 bool getSuccessors( AStarSearch< PuzzleState > * aStarSearch ,
                     PuzzleState * parentNode ) override {
  PuzzleState newNode;
  int spx , spy;
  getSpacePosition( this , &spx , &spy );
  if( legalMove( tiles , newNode.tiles , spx , spy , spx , spy - 1 ) ) {
   if( !aStarSearch->addSuccessor( newNode ) ) return false;
  }
  if( legalMove( tiles , newNode.tiles , spx , spy , spx , spy + 1 ) ) {
   if( !aStarSearch->addSuccessor( newNode ) ) return false;
  }
  if( legalMove( tiles , newNode.tiles , spx , spy , spx - 1 , spy ) ) {
   if( !aStarSearch->addSuccessor( newNode ) ) return false;
  }
  if( legalMove( tiles , newNode.tiles , spx , spy , spx + 1 , spy ) ) {
   if( !aStarSearch->addSuccessor( newNode ) ) return false;
  }
  return true;
 }

 float getCost( PuzzleState & successor ) override {
  return 1.0f;
 }

 bool isSameState( PuzzleState & rhs ) override {
  for( int i = 0 ; i < BOARD_HEIGHT * BOARD_WIDTH ; i++ ) {
   if( tiles[ i ] != rhs.tiles[ i ] ) return false;
  }
  return true;
 }

 void printNodeInfo() {
  char str[100];
  sprintf( str , "%c %c %c\n%c %c %c\n%c %c %c\n" ,
           tiles[ 0 ] + '0' ,
           tiles[ 1 ] + '0' ,
           tiles[ 2 ] + '0' ,
           tiles[ 3 ] + '0' ,
           tiles[ 4 ] + '0' ,
           tiles[ 5 ] + '0' ,
           tiles[ 6 ] + '0' ,
           tiles[ 7 ] + '0' ,
           tiles[ 8 ] + '0'
  );
  cout << str;
 }

 private:

 static void getSpacePosition( PuzzleState * pn , int * rx , int * ry ) {
  for( int y = 0 ; y < BOARD_HEIGHT ; y++ ) {
   for( int x = 0 ; x < BOARD_WIDTH ; x++ ) {
    if( pn->tiles[ ( y * BOARD_WIDTH ) + x ] == TL_SPACE ) {
     *rx = x;
     *ry = y;
     return;
    }
   }
  }
 }

 static bool legalMove( TILE * startTiles , TILE * targetTiles ,
                        int spx , int spy , int tx , int ty ) {
  if( getMap( spx , spy , startTiles ) == GM_SPACE ) {
   if( getMap( tx , ty , startTiles ) == GM_TILE ) {
    for( int i = 0 ; i < ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
     targetTiles[ i ] = startTiles[ i ];
    }
    targetTiles[ ( ty * BOARD_WIDTH ) + tx ] = startTiles[
     ( spy * BOARD_WIDTH ) + spx ];
    targetTiles[ ( spy * BOARD_WIDTH ) + spx ] = startTiles[
     ( ty * BOARD_WIDTH ) + tx ];
    return true;
   }
  }
  return false;
 }

 static int getMap( int x , int y ,
                    const TILE * tiles ) {
  if( x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT )
   return GM_OFF_BOARD;
  if( tiles[ ( y * BOARD_WIDTH ) + x ] == TL_SPACE ) return GM_SPACE;
  return GM_TILE;
 }
};

PuzzleState::TILE PuzzleState::goal[] = {
 TL_1 ,
 TL_2 ,
 TL_3 ,
 TL_8 ,
 TL_SPACE ,
 TL_4 ,
 TL_7 ,
 TL_6 ,
 TL_5
};

PuzzleState::TILE PuzzleState::start[] = {
 TL_1 ,
 TL_3 ,
 TL_4 ,
 TL_8 ,
 TL_SPACE ,
 TL_2 ,
 TL_7 ,
 TL_6 ,
 TL_5
};

#endif // A_STAR_SEARCH_PUZZLE_STATE_H
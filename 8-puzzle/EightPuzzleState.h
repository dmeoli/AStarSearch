#ifndef A_STAR_SEARCH_EIGHT_PUZZLE_STATE_H
#define A_STAR_SEARCH_EIGHT_PUZZLE_STATE_H

#include <cstring>
#include <iostream>
#include <cassert>

#include "../AStarSearch.h"
#include "../AStarState.h"

#define BOARD_WIDTH 3
#define BOARD_HEIGHT 3

#define GM_TILE -1
#define GM_SPACE 0
#define GM_OFF_BOARD 1

class EightPuzzleState : public AStarState< EightPuzzleState >
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

 EightPuzzleState() {
  memcpy( tiles , goal , sizeof( TILE ) * BOARD_WIDTH * BOARD_HEIGHT );
 }

 explicit EightPuzzleState( TILE * param_tiles ) {
  memcpy( tiles , param_tiles , sizeof( TILE ) * BOARD_WIDTH * BOARD_HEIGHT );
 }

 float goal_distance_estimate( EightPuzzleState & goal_node ) override {
  // Nilsson's sequence score heuristic: not admissible and so it is not
  // optimal, i.e., it may result in a suboptimal solution, but may do so in
  // a much shorter time than that taken by an admissible heuristic, e.g.,
  // Manhattan distance that is optimal, so it will always find the cheapest
  // path solution.
  // https://cs.stackexchange.com/questions/1904/nilssons-sequence-score-for-8-puzzle-problem-in-a-algorithm
  int i , cx , cy , ax , ay , h = 0 , s , t;
  // given a tile this returns the tile that should be clockwise
  TILE correct_follower_to[BOARD_WIDTH * BOARD_HEIGHT] = {
   TL_SPACE , // always wrong and then skipped
   TL_2 ,
   TL_3 ,
   TL_4 ,
   TL_5 ,
   TL_6 ,
   TL_7 ,
   TL_8 ,
   TL_1
  };
  // given a table index returns the index of the tile that is clockwise to
  // it 3*3 only
  int clockwise_tile_of[BOARD_WIDTH * BOARD_HEIGHT] = {
   1 ,
   2 ,
   5 ,
   0 ,
   -1 , // never called with center square
   8 ,
   3 ,
   6 ,
   7
  };
  int tile_x[BOARD_WIDTH * BOARD_HEIGHT] = {
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
  int tile_y[BOARD_WIDTH * BOARD_HEIGHT] = {
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
  // a tile in the center scores 1 (since it should be empty)
  if( tiles[ ( BOARD_HEIGHT * BOARD_WIDTH ) / 2 ] != TL_SPACE )
   s = 1;
  for( i = 0 ; i < ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
   if( tiles[ i ] == TL_SPACE )
    continue;
   cx = tile_x[ tiles[ i ] ];
   cy = tile_y[ tiles[ i ] ];
   ax = i % BOARD_WIDTH;
   ay = i / BOARD_WIDTH;
   // Manhattan distance
   h += abs( cx - ax ) + abs( cy - ay );
   // For each tile not in the center (since it is the last), if the tile
   // clockwise to it is not the one that should be clockwise to it then
   // score 2...
   if( ax == ( BOARD_WIDTH / 2 ) && ay == ( BOARD_HEIGHT / 2 ) )
    continue;
   if( correct_follower_to[ tiles[ i ] ] != tiles[ clockwise_tile_of[ i ] ] )
    s += 2;
  }
  // ... multiply this sequence by 3 and finally add the total distance
  // you need to move each tile back to its correct position, i.e., the
  // Manhattan distance.
  t = h + ( 3 * s );
  return ( float ) t;
 }

 bool is_goal( EightPuzzleState & goal_node ) override {
  return is_same_state( goal_node );
 }

 bool get_successors( AStarSearch< EightPuzzleState > * a_star_search ,
                      EightPuzzleState * parent_node ) override {
  EightPuzzleState new_node;
  int sp_x , sp_y;
  get_space_position( this , &sp_x , &sp_y );
  if( legal_move( tiles , new_node.tiles , sp_x , sp_y , sp_x , sp_y - 1 ) ) {
   if( !a_star_search->add_successor( new_node ) )
    return false;
  }
  if( legal_move( tiles , new_node.tiles , sp_x , sp_y , sp_x , sp_y + 1 ) ) {
   if( !a_star_search->add_successor( new_node ) )
    return false;
  }
  if( legal_move( tiles , new_node.tiles , sp_x , sp_y , sp_x - 1 , sp_y ) ) {
   if( !a_star_search->add_successor( new_node ) )
    return false;
  }
  if( legal_move( tiles , new_node.tiles , sp_x , sp_y , sp_x + 1 , sp_y ) ) {
   if( !a_star_search->add_successor( new_node ) )
    return false;
  }
  return true;
 }

 float get_cost( EightPuzzleState & successor ) override {
  return 1.0f;
 }

 bool is_same_state( EightPuzzleState & rhs ) override {
  for( int i = 0 ; i < ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
   if( tiles[ i ] != rhs.tiles[ i ] )
    return false;
  }
  return true;
 }

 void print_node_info() {
  int r;
  std::cout << "\n\n";
  for( int y = 0 ; y < BOARD_HEIGHT ; y++ ) {
   std::cout << "+----+----+----+\n";
   for( int x = 0 ; x < BOARD_WIDTH ; x++ ) {
    r = tiles[ ( y * BOARD_WIDTH ) + x ];
    std::cout << "| ";
    if( r < ( BOARD_HEIGHT * BOARD_WIDTH ) ) std::cout << " ";
    if( !r ) std::cout << "  ";
    else std::cout << r << " ";
   }
   std::cout << "|\n";
  }
  std::cout << "+----+----+----+\n";
 }

 private:

 static void get_space_position( EightPuzzleState * pn ,
                                 int * r_x , int * r_y ) {
  for( int y = 0 ; y < BOARD_HEIGHT ; y++ ) {
   for( int x = 0 ; x < BOARD_WIDTH ; x++ ) {
    if( pn->tiles[ ( y * BOARD_WIDTH ) + x ] == TL_SPACE ) {
     *r_x = x;
     *r_y = y;
     return;
    }
   }
  }
 }

 static bool legal_move( TILE * start_tiles , TILE * target_tiles ,
                         int sp_x , int sp_y , int t_x , int t_y ) {
  if( get_map( sp_x , sp_y , start_tiles ) == GM_SPACE ) {
   if( get_map( t_x , t_y , start_tiles ) == GM_TILE ) {
    for( int i = 0 ; i < ( BOARD_HEIGHT * BOARD_WIDTH ) ; i++ ) {
     target_tiles[ i ] = start_tiles[ i ];
    }
    target_tiles[ ( t_y * BOARD_WIDTH ) + t_x ] = start_tiles[
     ( sp_y * BOARD_WIDTH ) + sp_x ];
    target_tiles[ ( sp_y * BOARD_WIDTH ) + sp_x ] = start_tiles[
     ( t_y * BOARD_WIDTH ) + t_x ];
    return true;
   }
  }
  return false;
 }

 static int get_map( int x , int y ,
                     const TILE * tiles ) {
  if( x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT )
   return GM_OFF_BOARD;
  if( tiles[ ( y * BOARD_WIDTH ) + x ] == TL_SPACE )
   return GM_SPACE;
  return GM_TILE;
 }
};

EightPuzzleState::TILE EightPuzzleState::goal[] =
 {
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

#endif // A_STAR_SEARCH_EIGHT_PUZZLE_STATE_H
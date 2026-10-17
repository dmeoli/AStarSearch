/*--------------------------------------------------------------------------*/
/*--------------------------- File FindPath.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Finds a shortest path with A* on a 20 x 20 maze, where '9' is a wall and
 * the other cells cost what they say to leave
 *
 * Usage:   FindPath [<start column> <start row> <goal column> <goal row>]
 *
 * Example: FindPath 3 6 17 15
 *
 * Without the arguments the start and the goal are free cells drawn at
 * random. The start and the goal may lie in a wall, which costs 9 to
 * leave.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstdlib>

#include <iostream>

#include <random>

#include "AStar.h"

#include "Grid.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 static const char * maze[ 20 ] = {
  "11111111111111111111" , "19999999999999999991" ,
  "19911999191919199911" , "19911999191919199911" ,
  "19111199191911119911" , "19119111191111911111" ,
  "19999111111999911111" , "19999999911199999991" ,
  "19111111111911111111" , "19199999991199999991" ,
  "19111191191111111111" , "19999919191999991111" ,
  "19191999191919199911" , "19191999191919199911" ,
  "19111199191911119911" , "19119111191111911111" ,
  "19999111111999911111" , "11999999911199919999" ,
  "19111111111911111111" , "11111111111111111111" };

 GridMap m( 20 , 20 );
 for( int y = 0 ; y < 20 ; ++y )
  for( int x = 0 ; x < 20 ; ++x )
   m.set_cost( x , y , maze[ y ][ x ] == '9' ? 0 : maze[ y ][ x ] - '0' );

 int sx , sy , gx , gy;
 if( argc == 5 ) {
  sx = std::atoi( argv[ 1 ] );
  sy = std::atoi( argv[ 2 ] );
  gx = std::atoi( argv[ 3 ] );
  gy = std::atoi( argv[ 4 ] );
  }
 else {
  std::mt19937 gen( std::random_device{}() );
  do {
   sx = int( gen() % 20 );
   sy = int( gen() % 20 );
   } while( ! m.cost( sx , sy ) );
  do {
   gx = int( gen() % 20 );
   gy = int( gen() % 20 );
   } while( ! m.cost( gx , gy ) );
  }

 // as in the original example, the start and the goal may lie in a wall,
 // which is then left (or entered) at the cost of the wall, 9
 if( ( sx >= 0 ) && ( sx < 20 ) && ( sy >= 0 ) && ( sy < 20 ) &&
     ! m.cost( sx , sy ) )
  m.set_cost( sx , sy , 9 );
 if( ( gx >= 0 ) && ( gx < 20 ) && ( gy >= 0 ) && ( gy < 20 ) &&
     ! m.cost( gx , gy ) )
  m.set_cost( gx , gy , 9 );

 try {
  const Grid< 4 > g( m , sx , sy , gx , gy );
  const auto sol = astar( g );
  if( ! sol.solved() ) {
   std::cout << "Search terminated. Did not find goal state!" << std::endl;
   return( 1 );
   }
  std::cout << "Search found goal state..." << std::endl
            << "Displaying solution:" << std::endl;
  for( auto s : sol.path )
   std::cout << "Node position: (" << s % 20 << "," << s / 20 << ")"
             << std::endl;
  std::cout << "Solution steps: " << sol.path.size() - 1 << std::endl
            << "Solution cost: " << sol.cost << std::endl
            << "Search steps: " << sol.stats.expanded << std::endl;
  return( 0 );
  }
 catch( std::exception & e ) {
  std::cerr << "FindPath: " << e.what() << std::endl;
  return( 2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File FindPath.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

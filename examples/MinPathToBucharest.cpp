/*--------------------------------------------------------------------------*/
/*---------------------- File MinPathToBucharest.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Finds with A* the shortest road from a city of Romania to Bucharest, on
 * the map of Russell and Norvig, "Artificial Intelligence: a modern
 * approach", with the straight-line distance to Bucharest as heuristic
 *
 * Usage:   MinPathToBucharest {Arad|Bucharest|Craiova|Drobeta|Eforie|
 *                              Fagaras|Giurgiu|Hirsova|Iasi|Lugoj|Mehadia|
 *                              Neamt|Oradea|Pitesti|RimnicuVilcea|Sibiu|
 *                              Timisoara|Urziceni|Vaslui|Zerind}
 *
 * Example: MinPathToBucharest Arad
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

#include <iostream>

#include <string>

#include "AStar.h"

#include "ExplicitGraph.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 static const char * name[] = { "Arad" , "Bucharest" , "Craiova" , "Drobeta" ,
                                "Eforie" , "Fagaras" , "Giurgiu" , "Hirsova" ,
                                "Iasi" , "Lugoj" , "Mehadia" , "Neamt" ,
                                "Oradea" , "Pitesti" , "RimnicuVilcea" ,
                                "Sibiu" , "Timisoara" , "Urziceni" , "Vaslui" ,
                                "Zerind" };
 enum { Arad , Bucharest , Craiova , Drobeta , Eforie , Fagaras , Giurgiu ,
        Hirsova , Iasi , Lugoj , Mehadia , Neamt , Oradea , Pitesti ,
        RimnicuVilcea , Sibiu , Timisoara , Urziceni , Vaslui , Zerind , n };

 std::uint32_t start = Arad;
 if( argc == 2 ) {
  start = n;
  for( std::uint32_t c = 0 ; c < n ; ++c )
   if( argv[ 1 ] == std::string( name[ c ] ) )
    start = c;
  if( start == n ) {
   std::cout << "There is no city named " << argv[ 1 ] << " in the map!"
             << std::endl;
   return( 1 );
   }
  }

 ExplicitGraph< int > g( n , start , Bucharest );
 const int road[][ 3 ] = {
  { Arad , Sibiu , 140 } , { Arad , Zerind , 75 } ,
  { Arad , Timisoara , 118 } ,
  { Bucharest , Giurgiu , 90 } , { Bucharest , Urziceni , 85 } ,
  { Bucharest , Fagaras , 211 } , { Bucharest , Pitesti , 101 } ,
  { Craiova , Drobeta , 120 } , { Craiova , RimnicuVilcea , 146 } ,
  { Craiova , Pitesti , 138 } , { Drobeta , Mehadia , 75 } ,
  { Eforie , Hirsova , 86 } , { Fagaras , Sibiu , 99 } ,
  { Hirsova , Urziceni , 98 } , { Iasi , Vaslui , 92 } ,
  { Iasi , Neamt , 87 } ,
  { Lugoj , Timisoara , 111 } , { Lugoj , Mehadia , 70 } ,
  { Oradea , Zerind , 71 } , { Oradea , Sibiu , 151 } ,
  { Pitesti , RimnicuVilcea , 97 } , { RimnicuVilcea , Sibiu , 80 } ,
  { Urziceni , Vaslui , 142 } };
 for( const auto & r : road )
  g.add_edge( std::uint32_t( r[ 0 ] ) , std::uint32_t( r[ 1 ] ) , r[ 2 ] );
 const int sld[ n ] = { 366 , 0 , 160 , 242 , 161 , 176 , 77 , 151 , 226 ,
                        244 , 241 , 234 , 380 , 100 , 193 , 253 , 329 , 80 ,
                        199 , 374 };
 for( std::uint32_t c = 0 ; c < n ; ++c )
  g.set_heuristic( c , sld[ c ] );

 const auto sol = astar( g );
 std::cout << "Search found goal state..." << std::endl
           << "Displaying solution:" << std::endl;
 for( auto c : sol.path )
  std::cout << " " << name[ c ] << std::endl;
 std::cout << "Solution steps: " << sol.path.size() - 1 << std::endl
           << "Solution cost: " << sol.cost << std::endl
           << "Search steps: " << sol.stats.expanded << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File MinPathToBucharest.cpp ---------------------*/
/*--------------------------------------------------------------------------*/

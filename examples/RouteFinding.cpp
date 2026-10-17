/*--------------------------------------------------------------------------*/
/*------------------------- File RouteFinding.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Route finding with A* on the road map of Romania of figure 3.2 of Russell
 * and Norvig, "Artificial Intelligence: A Modern Approach" (3rd edition,
 * 2010).
 *
 * Usage:   RouteFinding [<from> [<to>]]
 *
 * Example: RouteFinding Lugoj Neamt
 *
 * The cities are Arad, Bucharest, Craiova, Drobeta, Eforie, Fagaras,
 * Giurgiu, Hirsova, Iasi, Lugoj, Mehadia, Neamt, Oradea, Pitesti, Rimnicu
 * (Vilcea), Sibiu, Timisoara, Urziceni, Vaslui and Zerind; the default is
 * the trip of the book, from Arad to Bucharest. Towards Bucharest the
 * heuristic is the straight-line distance of figure 3.22 of the book;
 * towards any other city it is the distance between the coordinates of the
 * cities in aima-python (github.com/aimacode/aima-python), which on this
 * map never exceeds the length of a road and is therefore consistent.
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

#include <cmath>

#include <iostream>

#include <string>

#include "AStar.h"

#include "ExplicitGraph.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE MAP ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

 enum City : std::uint32_t { Arad , Bucharest , Craiova , Drobeta , Eforie ,
                             Fagaras , Giurgiu , Hirsova , Iasi , Lugoj ,
                             Mehadia , Neamt , Oradea , Pitesti , Rimnicu ,
                             Sibiu , Timisoara , Urziceni , Vaslui , Zerind ,
                             NCities };

 const char * const c_name[ NCities ] = {
  "Arad" , "Bucharest" , "Craiova" , "Drobeta" , "Eforie" , "Fagaras" ,
  "Giurgiu" , "Hirsova" , "Iasi" , "Lugoj" , "Mehadia" , "Neamt" , "Oradea" ,
  "Pitesti" , "Rimnicu" , "Sibiu" , "Timisoara" , "Urziceni" , "Vaslui" ,
  "Zerind" };

 /// the roads and their length in km (figure 3.2)
 struct Road { City a , b; int km; };
 const Road c_road[] = {
  { Oradea , Zerind , 71 } , { Oradea , Sibiu , 151 } ,
  { Zerind , Arad , 75 } , { Arad , Sibiu , 140 } ,
  { Arad , Timisoara , 118 } ,
  { Timisoara , Lugoj , 111 } , { Lugoj , Mehadia , 70 } ,
  { Mehadia , Drobeta , 75 } , { Drobeta , Craiova , 120 } ,
  { Craiova , Rimnicu , 146 } , { Craiova , Pitesti , 138 } ,
  { Sibiu , Rimnicu , 80 } , { Sibiu , Fagaras , 99 } ,
  { Rimnicu , Pitesti , 97 } , { Fagaras , Bucharest , 211 } ,
  { Pitesti , Bucharest , 101 } , { Bucharest , Giurgiu , 90 } ,
  { Bucharest , Urziceni , 85 } , { Urziceni , Hirsova , 98 } ,
  { Hirsova , Eforie , 86 } , { Urziceni , Vaslui , 142 } ,
  { Vaslui , Iasi , 92 } , { Iasi , Neamt , 87 } };

 /// the straight-line distance to Bucharest (figure 3.22)
 const int c_to_bucharest[ NCities ] = {
  366 , 0 , 160 , 242 , 161 , 176 , 77 , 151 , 226 , 244 , 241 , 234 , 380 ,
  100 , 193 , 253 , 329 , 80 , 199 , 374 };

 /// the coordinates of the cities in aima-python
 const int c_xy[ NCities ][ 2 ] = {
  { 91 , 492 } , { 400 , 327 } , { 253 , 288 } , { 165 , 299 } ,
  { 562 , 293 } , { 305 , 449 } , { 375 , 270 } , { 534 , 350 } ,
  { 473 , 506 } , { 165 , 379 } , { 168 , 339 } , { 406 , 537 } ,
  { 131 , 571 } , { 320 , 368 } , { 233 , 410 } , { 207 , 457 } ,
  { 94 , 410 } , { 456 , 350 } , { 509 , 444 } , { 108 , 531 } };

 /// the city called name, NCities if there is none
 City find( const std::string & name ) {
  for( std::uint32_t c = 0 ; c < NCities ; ++c )
   if( name == c_name[ c ] )
    return( City( c ) );
  return( NCities );
  }

 }  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char * argv[] )
{
 const City from = argc > 1 ? find( argv[ 1 ] ) : Arad;
 const City to = argc > 2 ? find( argv[ 2 ] ) : Bucharest;
 if( ( from == NCities ) || ( to == NCities ) ) {
  std::cerr << "RouteFinding: unknown city" << std::endl;
  return( 2 );
  }

 // real costs, since the heuristic from the coordinates is not an integer
 ExplicitGraph< double > g( NCities , from , to );
 for( const Road & r : c_road )
  g.add_edge( r.a , r.b , r.km );
 for( std::uint32_t c = 0 ; c < NCities ; ++c )
  g.set_heuristic( c , to == Bucharest
                        ? double( c_to_bucharest[ c ] )
                        : std::hypot( c_xy[ c ][ 0 ] - c_xy[ to ][ 0 ] ,
                                      c_xy[ c ][ 1 ] - c_xy[ to ][ 1 ] ) );

 const auto sol = astar( g );
 if( ! sol.solved() ) {
  std::cout << "no route" << std::endl;
  return( 1 );
  }
 double km = 0;
 for( std::size_t k = 0 ; k < sol.path.size() ; ++k ) {
  if( k > 0 )
   for( const Road & r : c_road )
    if( ( ( r.a == sol.path[ k - 1 ] ) && ( r.b == sol.path[ k ] ) ) ||
        ( ( r.b == sol.path[ k - 1 ] ) && ( r.a == sol.path[ k ] ) ) )
     km += r.km;
  std::cout << c_name[ sol.path[ k ] ] << " (" << km << " km)" << std::endl;
  }
 std::cout << "expanded: " << sol.stats.expanded << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File RouteFinding.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

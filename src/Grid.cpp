/*--------------------------------------------------------------------------*/
/*----------------------------- File Grid.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the class GridMap: the construction of a map, free,
 * random or read from a benchmark of the Moving AI Lab, and the methods
 * that change it.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- IMPLEMENTATION -----------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <istream>

#include <random>

#include <stdexcept>

#include <string>

#include "Grid.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE AND USING ---------------------------*/
/*--------------------------------------------------------------------------*/

using namespace AStar_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------- CONSTRUCTION OF A MAP --------------------------*/
/*--------------------------------------------------------------------------*/

GridMap::GridMap( int width , int height )
 : f_width( width ) , f_height( height ) ,
   v_cost( std::size_t( width ) * std::size_t( height ) , 1 )
{
 if( ( width < 0 ) || ( height < 0 ) )
  throw( std::invalid_argument( "GridMap::GridMap: negative size" ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

GridMap GridMap::random( int width , int height , double p ,
                         std::uint64_t seed )
{
 GridMap m( width , height );
 std::mt19937_64 gen( seed );
 std::bernoulli_distribution blocked( p );
 for( auto & c : m.v_cost )
  c = blocked( gen ) ? 0 : 1;
 m.set_cost( 0 , height - 1 , 1 );
 m.set_cost( width - 1 , height - 1 , 1 );
 return( m );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

GridMap GridMap::read_movingai( std::istream & in )
{
 std::string word;
 int width = -1 , height = -1;
 while( in >> word && ( word != "map" ) ) {
  if( word == "height" )
   in >> height;
  else
   if( word == "width" )
    in >> width;
  }
 if( ( width <= 0 ) || ( height <= 0 ) )
  throw( std::invalid_argument( "GridMap::read_movingai: bad header" ) );
 GridMap m( width , height );
 for( int y = 0 ; y < height ; ++y ) {
  std::string row;
  if( ! ( in >> row ) || ( int( row.size() ) != width ) )
   throw( std::invalid_argument( "GridMap::read_movingai: bad row" ) );
  for( int x = 0 ; x < width ; ++x ) {
   const char c = row[ x ];
   m.set_cost( x , y , ( c == '.' ) || ( c == 'G' ) || ( c == 'S' ) ? 1 : 0 );
   }
  }
 return( m );
 }

/*--------------------------------------------------------------------------*/
/*---------------- METHODS FOR CHANGING AND READING THE MAP ----------------*/
/*--------------------------------------------------------------------------*/

void GridMap::set_cost( int x , int y , int c )
{
 if( ( x < 0 ) || ( y < 0 ) || ( x >= f_width ) || ( y >= f_height ) ||
     ( c < 0 ) || ( c > 255 ) )
  throw( std::invalid_argument( "GridMap::set_cost: bad cell or cost" ) );
 v_cost[ std::size_t( y ) * f_width + x ] = std::uint8_t( c );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int GridMap::min_cost( void ) const noexcept
{
 int m = 256;
 for( auto c : v_cost )
  if( c && ( c < m ) )
   m = c;
 return( m == 256 ? 1 : m );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Grid.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/

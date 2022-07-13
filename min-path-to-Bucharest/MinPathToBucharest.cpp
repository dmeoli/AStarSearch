/**
 * A* Search implementation to find the minimum path between two cities given
 * a map. The example is taken from the book Artificial Intelligence:
 * A Modern Approach, by Russel and Norvig, where a map of Romania is given.
 * The target node is Bucharest, and the user can specify the initial city
 * from which the search algorithm will start looking for the minimum path to
 * Bucharest.
 *
 * Usage:   ./MinPathToBucharest {Arad|Bucharest|Craiova|Drobeta|Eforie|
 *                               |Fagaras|Giurgiu|Hirsova|Iasi|Lugoj|Mehadia|
 *                               |Neamt|Oradea|Pitesti|RimnicuVilcea|Sibiu|
 *                               |Timisoara|Urziceni|Vaslui|Zerind}
 *
 * Example: ./MinPathToBucharest Arad
 */

#include "PathSearchNode.h"

int main( int argc , char * argv[] ) {
 for( auto & i : romania_map ) {
  for( float & j : i ) {
   j = -1.0f;
  }
 }

 romania_map[ Arad ][ Sibiu ] = 140;
 romania_map[ Arad ][ Zerind ] = 75;
 romania_map[ Arad ][ Timisoara ] = 118;
 romania_map[ Bucharest ][ Giurgiu ] = 90;
 romania_map[ Bucharest ][ Urziceni ] = 85;
 romania_map[ Bucharest ][ Fagaras ] = 211;
 romania_map[ Bucharest ][ Pitesti ] = 101;
 romania_map[ Craiova ][ Drobeta ] = 120;
 romania_map[ Craiova ][ RimnicuVilcea ] = 146;
 romania_map[ Craiova ][ Pitesti ] = 138;
 romania_map[ Drobeta ][ Craiova ] = 120;
 romania_map[ Drobeta ][ Mehadia ] = 75;
 romania_map[ Eforie ][ Hirsova ] = 75;
 romania_map[ Fagaras ][ Bucharest ] = 211;
 romania_map[ Fagaras ][ Sibiu ] = 99;
 romania_map[ Giurgiu ][ Bucharest ] = 90;
 romania_map[ Hirsova ][ Eforie ] = 86;
 romania_map[ Hirsova ][ Urziceni ] = 98;
 romania_map[ Iasi ][ Vaslui ] = 92;
 romania_map[ Iasi ][ Neamt ] = 87;
 romania_map[ Lugoj ][ Timisoara ] = 111;
 romania_map[ Lugoj ][ Mehadia ] = 70;
 romania_map[ Mehadia ][ Lugoj ] = 70;
 romania_map[ Mehadia ][ Drobeta ] = 75;
 romania_map[ Neamt ][ Iasi ] = 87;
 romania_map[ Oradea ][ Zerind ] = 71;
 romania_map[ Oradea ][ Sibiu ] = 151;
 romania_map[ Pitesti ][ Bucharest ] = 101;
 romania_map[ Pitesti ][ RimnicuVilcea ] = 97;
 romania_map[ Pitesti ][ Craiova ] = 138;
 romania_map[ RimnicuVilcea ][ Pitesti ] = 97;
 romania_map[ RimnicuVilcea ][ Craiova ] = 146;
 romania_map[ RimnicuVilcea ][ Sibiu ] = 80;
 romania_map[ Sibiu ][ RimnicuVilcea ] = 80;
 romania_map[ Sibiu ][ Fagaras ] = 99;
 romania_map[ Sibiu ][ Oradea ] = 151;
 romania_map[ Sibiu ][ Arad ] = 140;
 romania_map[ Timisoara ][ Arad ] = 118;
 romania_map[ Timisoara ][ Lugoj ] = 111;
 romania_map[ Urziceni ][ Bucharest ] = 85;
 romania_map[ Urziceni ][ Hirsova ] = 98;
 romania_map[ Urziceni ][ Vaslui ] = 142;
 romania_map[ Vaslui ][ Urziceni ] = 142;
 romania_map[ Vaslui ][ Iasi ] = 92;
 romania_map[ Zerind ][ Arad ] = 75;
 romania_map[ Zerind ][ Oradea ] = 71;

 city_names[ Arad ].assign( "Arad" );
 city_names[ Bucharest ].assign( "Bucharest" );
 city_names[ Craiova ].assign( "Craiova" );
 city_names[ Drobeta ].assign( "Drobeta" );
 city_names[ Eforie ].assign( "Eforie" );
 city_names[ Fagaras ].assign( "Fagaras" );
 city_names[ Giurgiu ].assign( "Giurgiu" );
 city_names[ Hirsova ].assign( "Hirsova" );
 city_names[ Iasi ].assign( "Iasi" );
 city_names[ Lugoj ].assign( "Lugoj" );
 city_names[ Mehadia ].assign( "Mehadia" );
 city_names[ Neamt ].assign( "Neamt" );
 city_names[ Oradea ].assign( "Oradea" );
 city_names[ Pitesti ].assign( "Pitesti" );
 city_names[ RimnicuVilcea ].assign( "RimnicuVilcea" );
 city_names[ Sibiu ].assign( "Sibiu" );
 city_names[ Timisoara ].assign( "Timisoara" );
 city_names[ Urziceni ].assign( "Urziceni" );
 city_names[ Vaslui ].assign( "Vaslui" );
 city_names[ Zerind ].assign( "Zerind" );

 CITIES start_city = Arad;
 if( argc == 2 ) {
  bool found = false;
  for( size_t i = 0 ; i < city_names.size() ; i++ ) {
   if( city_names[ i ] == argv[ 1 ] ) {
    start_city = ( CITIES ) i;
    found = true;
    break;
   }
  }
  if( !found ) {
   cout << "There is no city named " << argv[ 1 ] << " in the map!" << endl;
   return EXIT_FAILURE;
  }
 }
 AStarSearch< PathSearchNode > a_star_search;
 PathSearchNode start_path_search_node;
 start_path_search_node.city = start_city;
 PathSearchNode goal_path_search_node;
 goal_path_search_node.city = Bucharest;
 a_star_search.set_start_goal_states( start_path_search_node ,
                                      goal_path_search_node );
 unsigned int search_state;
 unsigned int search_steps = 0;
 do {
  search_state = a_star_search.search_step();
  search_steps++;
 } while( search_state ==
          AStarSearch< PathSearchNode >::SEARCH_STATE_SEARCHING );
 if( search_state == AStarSearch< PathSearchNode >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  PathSearchNode * path_search_node = a_star_search.get_solution_start();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  path_search_node->print_node_info();
  for( ;; ) {
   path_search_node = a_star_search.get_solution_next();
   if( !path_search_node )
    break;
   path_search_node->print_node_info();
   steps++;
  }
  cout << "Solution steps: " << steps << endl;
  a_star_search.free_solution_nodes();
 } else if( search_state ==
            AStarSearch< PathSearchNode >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( search_state ==
            AStarSearch< PathSearchNode >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << search_steps << endl;
 return EXIT_SUCCESS;
}


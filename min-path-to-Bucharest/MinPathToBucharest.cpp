/**
 * A* Search implementation to find the minimum path between two cities given
 * a map. The example is taken from the book Artificial Intelligence:
 * A Modern Approach, by Russel and Norvig, where a map of Romania is given.
 * The target node is Bucharest, and the user can specify the initial city
 * from which the search algorithm will start looking for the minimum path to
 * Bucharest.
 *
 * Usage: ./MinPathToBucharest {Arad|Bucharest|Craiova|Drobeta|Eforie|
 *                             |Fagaras|Giurgiu|Hirsova|Iasi|Lugoj|Mehadia|
 *                             |Neamt|Oradea|Pitesti|RimnicuVilcea|Sibiu|
 *                             |Timisoara|Urziceni|Vaslui|Zerind}
 *
 * Example: ./MinPathToBucharest Arad
 *
 * @author Donato Meoli
 */

#include "PathSearchState.h"

int main( int argc , char * argv[] ) {
 for( auto & i : romaniaMap ) {
  for( float & j : i ) {
   j = -1.0f;
  }
 }

 romaniaMap[ Arad ][ Sibiu ] = 140;
 romaniaMap[ Arad ][ Zerind ] = 75;
 romaniaMap[ Arad ][ Timisoara ] = 118;
 romaniaMap[ Bucharest ][ Giurgiu ] = 90;
 romaniaMap[ Bucharest ][ Urziceni ] = 85;
 romaniaMap[ Bucharest ][ Fagaras ] = 211;
 romaniaMap[ Bucharest ][ Pitesti ] = 101;
 romaniaMap[ Craiova ][ Drobeta ] = 120;
 romaniaMap[ Craiova ][ RimnicuVilcea ] = 146;
 romaniaMap[ Craiova ][ Pitesti ] = 138;
 romaniaMap[ Drobeta ][ Craiova ] = 120;
 romaniaMap[ Drobeta ][ Mehadia ] = 75;
 romaniaMap[ Eforie ][ Hirsova ] = 75;
 romaniaMap[ Fagaras ][ Bucharest ] = 211;
 romaniaMap[ Fagaras ][ Sibiu ] = 99;
 romaniaMap[ Giurgiu ][ Bucharest ] = 90;
 romaniaMap[ Hirsova ][ Eforie ] = 86;
 romaniaMap[ Hirsova ][ Urziceni ] = 98;
 romaniaMap[ Iasi ][ Vaslui ] = 92;
 romaniaMap[ Iasi ][ Neamt ] = 87;
 romaniaMap[ Lugoj ][ Timisoara ] = 111;
 romaniaMap[ Lugoj ][ Mehadia ] = 70;
 romaniaMap[ Mehadia ][ Lugoj ] = 70;
 romaniaMap[ Mehadia ][ Drobeta ] = 75;
 romaniaMap[ Neamt ][ Iasi ] = 87;
 romaniaMap[ Oradea ][ Zerind ] = 71;
 romaniaMap[ Oradea ][ Sibiu ] = 151;
 romaniaMap[ Pitesti ][ Bucharest ] = 101;
 romaniaMap[ Pitesti ][ RimnicuVilcea ] = 97;
 romaniaMap[ Pitesti ][ Craiova ] = 138;
 romaniaMap[ RimnicuVilcea ][ Pitesti ] = 97;
 romaniaMap[ RimnicuVilcea ][ Craiova ] = 146;
 romaniaMap[ RimnicuVilcea ][ Sibiu ] = 80;
 romaniaMap[ Sibiu ][ RimnicuVilcea ] = 80;
 romaniaMap[ Sibiu ][ Fagaras ] = 99;
 romaniaMap[ Sibiu ][ Oradea ] = 151;
 romaniaMap[ Sibiu ][ Arad ] = 140;
 romaniaMap[ Timisoara ][ Arad ] = 118;
 romaniaMap[ Timisoara ][ Lugoj ] = 111;
 romaniaMap[ Urziceni ][ Bucharest ] = 85;
 romaniaMap[ Urziceni ][ Hirsova ] = 98;
 romaniaMap[ Urziceni ][ Vaslui ] = 142;
 romaniaMap[ Vaslui ][ Urziceni ] = 142;
 romaniaMap[ Vaslui ][ Iasi ] = 92;
 romaniaMap[ Zerind ][ Arad ] = 75;
 romaniaMap[ Zerind ][ Oradea ] = 71;

 cityNames[ Arad ].assign( "Arad" );
 cityNames[ Bucharest ].assign( "Bucharest" );
 cityNames[ Craiova ].assign( "Craiova" );
 cityNames[ Drobeta ].assign( "Drobeta" );
 cityNames[ Eforie ].assign( "Eforie" );
 cityNames[ Fagaras ].assign( "Fagaras" );
 cityNames[ Giurgiu ].assign( "Giurgiu" );
 cityNames[ Hirsova ].assign( "Hirsova" );
 cityNames[ Iasi ].assign( "Iasi" );
 cityNames[ Lugoj ].assign( "Lugoj" );
 cityNames[ Mehadia ].assign( "Mehadia" );
 cityNames[ Neamt ].assign( "Neamt" );
 cityNames[ Oradea ].assign( "Oradea" );
 cityNames[ Pitesti ].assign( "Pitesti" );
 cityNames[ RimnicuVilcea ].assign( "RimnicuVilcea" );
 cityNames[ Sibiu ].assign( "Sibiu" );
 cityNames[ Timisoara ].assign( "Timisoara" );
 cityNames[ Urziceni ].assign( "Urziceni" );
 cityNames[ Vaslui ].assign( "Vaslui" );
 cityNames[ Zerind ].assign( "Zerind" );

 CITIES startCity = Arad;
 if( argc == 2 ) {
  bool found = false;
  for( size_t i = 0 ; i < cityNames.size() ; i++ ) {
   if( cityNames[ i ] == argv[ 1 ] ) {
    startCity = ( CITIES ) i;
    found = true;
    break;
   }
  }
  if( !found ) {
   cout << "There is no city named " << argv[ 1 ] << " in the map!" << endl;
   return EXIT_FAILURE;
  }
 }
 AStarSearch< PathSearchState > aStarSearch;
 PathSearchState startPathSearchState;
 startPathSearchState.city = startCity;
 PathSearchState goalPathSearchState;
 goalPathSearchState.city = Bucharest;
 aStarSearch.setStartAndGoalStates( startPathSearchState ,
                                    goalPathSearchState );
 unsigned int searchState;
 unsigned int searchSteps = 0;
 do {
  searchState = aStarSearch.searchStep();
  searchSteps++;
 } while( searchState ==
          AStarSearch< PathSearchState >::SEARCH_STATE_SEARCHING );
 if( searchState == AStarSearch< PathSearchState >::SEARCH_STATE_SUCCEEDED ) {
  cout << "Search found goal state..." << endl;
  PathSearchState * pathSearchState = aStarSearch.getSolutionStart();
  cout << "Displaying solution:" << endl;
  int steps = 0;
  pathSearchState->printNodeInfo();
  for( ;; ) {
   pathSearchState = aStarSearch.getSolutionNext();
   if( !pathSearchState )
    break;
   pathSearchState->printNodeInfo();
   steps++;
  }
  cout << "Solution step: " << steps << endl;
  aStarSearch.freeSolutionNodes();
 } else if( searchState ==
            AStarSearch< PathSearchState >::SEARCH_STATE_FAILED ) {
  cout << "Search terminated. Did not find goal state!" << endl;
 } else if( searchState ==
            AStarSearch< PathSearchState >::SEARCH_STATE_OUT_OF_MEMORY ) {
  cout << "Search terminated. Out of memory!" << endl;
 }
 cout << "Search steps: " << searchSteps << endl;
 return EXIT_SUCCESS;
}


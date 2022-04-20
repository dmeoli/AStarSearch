cmake_minimum_required(VERSION 3.9)
project(AStarSearch)

set(CMAKE_CXX_STANDARD 11)

set(A_STAR AStarSearch.h AStarState.h)

add_executable(8Puzzle ${A_STAR}
        8-puzzle/PuzzleState.h
        8-puzzle/8Puzzle.cpp)

add_executable(FindPath ${A_STAR}
        find-path/MapSearchState.h
        find-path/FindPath.cpp)

add_executable(MinPathToBucharest ${A_STAR}
        min-path-to-Bucharest/PathSearchState.h
        min-path-to-Bucharest/MinPathToBucharest.cpp)
cmake_minimum_required(VERSION 3.9)
project(AStarSearch)

set(CMAKE_CXX_STANDARD 11)

set(A_STAR AStarSearch.h AStarState.h)

add_executable(8Puzzle ${A_STAR}
        8-puzzle/EightPuzzleState.h
        8-puzzle/8Puzzle.cpp)

add_executable(15Puzzle ${A_STAR}
        15-puzzle/FifteenPuzzleState.h
        15-puzzle/15Puzzle.cpp)

add_executable(FindPath ${A_STAR}
        find-path/MapSearchNode.h
        find-path/FindPath.cpp)

add_executable(MinPathToBucharest ${A_STAR}
        min-path-to-Bucharest/PathSearchNode.h
        min-path-to-Bucharest/MinPathToBucharest.cpp)
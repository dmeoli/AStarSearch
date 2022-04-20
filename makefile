CXX = g++
CXX_FLAGS = -Wall -std=c++11

FF_ROOT = ./fastflow

OPT_FLAGS = -O3

all: 8Puzzle FindPath MinPathToBucharest #NQueens Maze

8Puzzle:
	$(CXX) $(CXX_FLAGS) -fopenmp -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o 8Puzzle 8-puzzle/8Puzzle.cpp

FindPath:
	$(CXX) $(CXX_FLAGS) -fopenmp -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o FindPath find-path/FindPath.cpp

MinPathToBucharest:
	$(CXX) $(CXX_FLAGS) -fopenmp -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o MinPathToBucharest min-path-to-Bucharest/MinPathToBucharest.cpp

#NQueens:
#	$(CXX) $(CXX_FLAGS) -fopenmp -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o NQueens n-queens/NQueens.cpp

#Maze:
#	$(CXX) $(CXX_FLAGS) -fopenmp -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o Maze maze/Maze.cpp

MazeGenerator:
	javac maze/MazeGenerator.java

clean:
	rm -rf 8Puzzle FindPath MinPathToBucharest #NQueens Maze
CXX = g++
CXX_FLAGS = -Wall -std=c++11 -o

all: 8Puzzle FindPath MinPathToBucharest

8Puzzle:
	$(CXX) $(CXX_FLAGS) 8Puzzle 8-puzzle/8Puzzle.cpp

FindPath:
	$(CXX) $(CXX_FLAGS) FindPath find-path/FindPath.cpp

MinPathToBucharest:
	$(CXX) $(CXX_FLAGS) MinPathToBucharest min-path-to-Bucharest/MinPathToBucharest.cpp

clean:
	rm -rf 8Puzzle FindPath MinPathToBucharest
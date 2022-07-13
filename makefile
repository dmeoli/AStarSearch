CXX = g++
CXX_FLAGS = -Wall -std=c++11 -g -O3

FF_ROOT = ./fastflow

TARGETS = 8Puzzle 15Puzzle FindPath MinPathToBucharest

all: $(TARGETS)

8Puzzle:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) -o 8Puzzle 8-puzzle/8Puzzle.cpp

15Puzzle:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) -o 15Puzzle 15-puzzle/15Puzzle.cpp

FindPath:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) -o FindPath find-path/FindPath.cpp

MinPathToBucharest:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) -o MinPathToBucharest min-path-to-Bucharest/MinPathToBucharest.cpp

clean:
	rm -f $(TARGETS)

.PHONY : all clean
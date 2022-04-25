CXX = g++
CXX_FLAGS = -Wall -std=c++11

FF_ROOT = ./fastflow

OPT_FLAGS = -O3

TARGETS = 8Puzzle 15Puzzle FindPath MinPathToBucharest

all: $(TARGETS)

8Puzzle:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o 8Puzzle 8-puzzle/8Puzzle.cpp

15Puzzle:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o 15Puzzle 15-puzzle/15Puzzle.cpp

FindPath:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o FindPath find-path/FindPath.cpp

MinPathToBucharest:
	$(CXX) $(CXX_FLAGS) -lpthread -ltbb -I $(FF_ROOT) $(OPT_FLAGS) -o MinPathToBucharest min-path-to-Bucharest/MinPathToBucharest.cpp

clean:
	rm -f $(TARGETS)
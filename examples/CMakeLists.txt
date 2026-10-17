# --------------------------------------------------------------------------- #
#    CMake file for AStarSearch/examples                                      #
#                                                                             #
#    The four small programs of the library: the 8-puzzle, the 15-puzzle, a   #
#    maze and the map of Romania.                                             #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

foreach (example 8Puzzle 15Puzzle FindPath MinPathToBucharest)
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE ${modNamespace}::${modName})
endforeach ()

# --------------------------------------------------------------------------- #

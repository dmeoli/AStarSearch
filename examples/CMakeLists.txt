# --------------------------------------------------------------------------- #
#    CMake file for AStarSearch/examples                                      #
#                                                                             #
#    The small programs of the library, most of them from the textbook of     #
#    Russell and Norvig, and their runs with a known answer as tests of       #
#    correctness, labelled AStarSearch.example.                               #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

foreach (example 8Puzzle 15Puzzle FindPath RouteFinding NQueens WaterJugs
        TravelingSalesman)
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE ${modNamespace}::${modName})
endforeach ()

# ----- Tests --------------------------------------------------------------- #
# Each test runs an example and looks for the answer that the book (or the
# literature) gives, e.g., the 26 moves of figure 3.28 of Russell and Norvig
# or the 418 km from Arad to Bucharest; TravelingSalesman checks itself
# against the dynamic program of Held and Karp.
if (BUILD_TESTING)
    function(example_test name regex)
        add_test(NAME ${modName}.example.${name} COMMAND ${ARGN})
        set_tests_properties(${modName}.example.${name} PROPERTIES
                LABELS "${modName};example")
        if (NOT regex STREQUAL "")
            set_tests_properties(${modName}.example.${name} PROPERTIES
                    PASS_REGULAR_EXPRESSION "${regex}")
        endif ()
    endfunction()

    example_test(8puzzle-h2 "moves: +26\n" 8Puzzle)
    example_test(8puzzle-h1 "moves: +26\n" 8Puzzle -h h1)
    example_test(8puzzle-easy "moves: +5\n" 8Puzzle easy)
    example_test(8puzzle-worst "moves: +31\n" 8Puzzle worst)
    example_test(8puzzle-nilsson-worst "moves: +30\n" 8Puzzle -g nilsson
            worst)
    example_test(8puzzle-table "\n24 " 8Puzzle -t 5)
    set(korf1 14 13 15 7 11 12 9 5 6 0 2 1 4 8 10 3)
    example_test(15puzzle-korf1 "moves: +57\n" 15Puzzle ${korf1})
    example_test(15puzzle-hda "moves: +57\n" 15Puzzle -t 4 ${korf1})
    example_test(route-bucharest "Bucharest \\(418 km\\)" RouteFinding)
    example_test(route-lugoj-neamt "Neamt \\(910 km\\)" RouteFinding Lugoj
            Neamt)
    example_test(nqueens-8 "expanded" NQueens 8)
    example_test(nqueens-3 "no placement of 3 queens" NQueens 3)
    example_test(waterjugs-book "moves: +3\n" WaterJugs)
    example_test(waterjugs-5-3 "moves: +6\n" WaterJugs 4 5 3)
    example_test(waterjugs-none "cannot measure 3" WaterJugs 3 2 6)
    example_test(tsp-10 "" TravelingSalesman 10 1)
    example_test(tsp-14 "" TravelingSalesman 14 3)
    example_test(findpath-4 "cost: +94\n" FindPath)
    example_test(findpath-8 "cost: +76\\.6" FindPath -8)
endif ()

# --------------------------------------------------------------------------- #

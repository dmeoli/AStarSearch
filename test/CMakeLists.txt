# --------------------------------------------------------------------------- #
#    CMake file for AStarSearch/test                                          #
#                                                                             #
#    In this file we define the tests of the library: one tester, test.cpp,   #
#    run once for each case, every case of a parallel runtime only if the     #
#    runtime is there, and the MPI case under mpiexec with 1, 2 and 4         #
#    processes.                                                               #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# ----- the tester ---------------------------------------------------------- #
add_executable(${modName}_test test.cpp)
target_link_libraries(${modName}_test PRIVATE ${modNamespace}::${modName})
if (MSVC)
    target_compile_options(${modName}_test PRIVATE /W4)
else ()
    target_compile_options(${modName}_test PRIVATE -Wall -Wextra -Wpedantic)
endif ()

if (TARGET ${modName}_OpenMP)
    target_link_libraries(${modName}_test PRIVATE ${modNamespace}::OpenMP)
    target_compile_definitions(${modName}_test PRIVATE ASTAR_HAS_OPENMP)
endif ()
if (TARGET ${modName}_FastFlow)
    target_link_libraries(${modName}_test PRIVATE ${modNamespace}::FastFlow)
    target_compile_definitions(${modName}_test PRIVATE ASTAR_HAS_FASTFLOW)
endif ()
if (TARGET ${modName}_MPI)
    target_link_libraries(${modName}_test PRIVATE ${modNamespace}::MPI)
    target_compile_definitions(${modName}_test PRIVATE ASTAR_HAS_MPI)
endif ()

set(dataDir ${PROJECT_SOURCE_DIR}/data)

# ----- the cases ----------------------------------------------------------- #
set(cases astar grid graph limits reopen hda-thread spa-thread pbnf-thread
          pase-thread)
if (TARGET ${modName}_OpenMP)
    list(APPEND cases hda-openmp spa-openmp pbnf-openmp)
endif ()
if (TARGET ${modName}_FastFlow)
    list(APPEND cases hda-fastflow farm-fastflow)
endif ()

foreach (case ${cases})
    add_test(NAME ${modName}.${case}
             COMMAND ${modName}_test ${case} ${dataDir})
endforeach ()

# The MPI case runs on 1, 2 and 4 processes; --oversubscribe lets Open MPI
# start more processes than the cores of a small runner.
if (TARGET ${modName}_MPI)
    set(mpiFlags ${MPIEXEC_PREFLAGS})
    if (MPIEXEC_EXECUTABLE MATCHES "mpirun|mpiexec" AND
            MPI_CXX_LIBRARY_VERSION_STRING MATCHES "Open MPI")
        list(APPEND mpiFlags --oversubscribe)
    endif ()
    foreach (np 1 2 4)
        add_test(NAME ${modName}.mpi-${np}
                 COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${np}
                         ${mpiFlags} $<TARGET_FILE:${modName}_test>
                         ${MPIEXEC_POSTFLAGS} mpi ${dataDir})
    endforeach ()
endif ()

# ----- the label of this library's tests ----------------------------------- #
# The label lets the CI select exactly these tests with `ctest -L AStarSearch`.
get_property(_tests DIRECTORY PROPERTY TESTS)
if (_tests)
    set_tests_properties(${_tests} PROPERTIES LABELS "${modName}"
                         TIMEOUT 900)
endif ()

# --------------------------------------------------------------------------- #

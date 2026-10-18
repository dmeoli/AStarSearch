# --------------------------------------------------------------------------- #
#    CMake file for PBeFS/tools                                               #
#                                                                             #
#    AStarBench, the benchmark of the shared-memory searches, and, if MPI is  #
#    there, AStarBenchMPI, the same source compiled for HDAStarMPI.           #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

add_executable(AStarBench AStarBench.cpp)
target_link_libraries(AStarBench PRIVATE ${modNamespace}::${modName})
if (TARGET ${modName}_OpenMP)
    target_link_libraries(AStarBench PRIVATE ${modNamespace}::OpenMP)
    target_compile_definitions(AStarBench PRIVATE PBEFS_HAS_OPENMP)
endif ()
if (TARGET ${modName}_FastFlow)
    target_link_libraries(AStarBench PRIVATE ${modNamespace}::FastFlow)
    target_compile_definitions(AStarBench PRIVATE PBEFS_HAS_FASTFLOW)
endif ()

if (TARGET ${modName}_MPI)
    add_executable(AStarBenchMPI AStarBench.cpp)
    target_link_libraries(AStarBenchMPI PRIVATE ${modNamespace}::MPI)
    target_compile_definitions(AStarBenchMPI PRIVATE PBEFS_BENCH_MPI)
endif ()

install(TARGETS AStarBench RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if (TARGET AStarBenchMPI)
    install(TARGETS AStarBenchMPI RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

# --------------------------------------------------------------------------- #

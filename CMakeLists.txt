# --------------------------------------------------------------------------- #
#    Main CMake file for AStarSearch                                          #
#                                                                             #
#    This file builds the library of AStarSearch (the searches, templates     #
#    over the Domain, are in its headers), its interface targets for the      #
#    parallel runtimes, the examples, the benchmark tools and the tests, and  #
#    installs them with a package configuration file, so that other CMake     #
#    projects can use the library with find_package(AStarSearch).             #
#                                                                             #
#        $ cmake -S <source-path> -B <build-path>                             #
#        $ cmake --build <build-path>                                         #
#        $ ctest --test-dir <build-path>                                      #
#        $ cmake --build <build-path> --target install                        #
#                                                                             #
#    The parallel runtimes are optional: OpenMP and MPI are used if found,    #
#    FastFlow if its headers are in FASTFLOW_ROOT (the fastflow submodule by  #
#    default); each can be turned off with the ASTAR_WITH_* options.          #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.15)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeriveVersion.cmake)
smspp_derive_version(ASTAR_VERSION)

project(AStarSearch
        VERSION ${ASTAR_VERSION}
        DESCRIPTION "Sequential and parallel A* search"
        HOMEPAGE_URL https://github.com/dmeoli/AStarSearch
        LANGUAGES CXX)

# These variables make the code harder to read but easier to change.
set(modName ${PROJECT_NAME})
set(modNamespace ${PROJECT_NAME})

# Find out if it's being called by another project.
get_directory_property(hasParent PARENT_DIRECTORY)

# This adds the cmake directory to the module search paths,
# allowing us to use our modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ----- Settings ------------------------------------------------------------ #
# This creates an ENABLE_TESTING option (default: ON) and enables the testing
# using the ctest executable.
# See: https://cmake.org/cmake/help/latest/manual/ctest.1.html
include(CTest)

# Sets the default build type (if none was specified).
# See: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
include(BuildType)

# The standard installation directories, used by the subdirectories too.
include(GNUInstallDirs)

option(ASTAR_WITH_OPENMP "Build the OpenMP runtime if OpenMP is found" ON)
option(ASTAR_WITH_MPI "Build the MPI search if MPI is found" ON)
option(ASTAR_WITH_FASTFLOW "Build the FastFlow runtime if found" ON)
option(ASTAR_BUILD_EXAMPLES "Build the examples" ON)
option(ASTAR_BUILD_TOOLS "Build the benchmark tools" ON)

set(FASTFLOW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/fastflow CACHE PATH
    "Directory holding the ff/ headers of FastFlow")

# ----- Requirements -------------------------------------------------------- #
find_package(Threads REQUIRED)

if (ASTAR_WITH_OPENMP)
    find_package(OpenMP COMPONENTS CXX)
endif ()

if (ASTAR_WITH_MPI)
    find_package(MPI COMPONENTS CXX)
endif ()

# FastFlow is header-only: its submodule is initialized if it is not there.
if (ASTAR_WITH_FASTFLOW AND NOT EXISTS ${FASTFLOW_ROOT}/ff/ff.hpp AND
        FASTFLOW_ROOT STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/fastflow")
    find_package(Git QUIET)
    if (GIT_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init fastflow
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
endif ()
if (ASTAR_WITH_FASTFLOW AND EXISTS ${FASTFLOW_ROOT}/ff/ff.hpp)
    set(ASTAR_HAS_FASTFLOW ON)
else ()
    set(ASTAR_HAS_FASTFLOW OFF)
endif ()

# ----- Configuration header ------------------------------------------------ #
# This will generate the AStarSearchConfig.h header in the build directory.
string(MAKE_C_IDENTIFIER "${modName}" modMacro)
configure_file(cmake/${modName}Config.h.in ${modName}Config.h)

# ----- Library ------------------------------------------------------------- #
# The searches are templates over the Domain and live in the headers; the
# library holds what does not depend on it. With the BUILD_SHARED_LIBS
# variable we can specify if the library will be STATIC or SHARED, and one
# more INTERFACE target for each optional runtime carries what that runtime
# needs.
add_library(${modName})
target_compile_features(${modName} PUBLIC cxx_std_20)

# The name of the shared library carries the version of its binary interface,
# which any minor release may change while the major version is 0, and then
# any major release; MSVC exports the symbols only if asked to.
if (PROJECT_VERSION_MAJOR EQUAL 0)
    set(soVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
else ()
    set(soVersion ${PROJECT_VERSION_MAJOR})
endif ()
set_target_properties(${modName} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${soVersion}
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Note: do not GLOB files here.
target_sources(${modName} PRIVATE
        src/Distribution.cpp
        src/Grid.cpp
        src/SearchUtils.cpp
        src/Solution.cpp)

target_include_directories(
        ${modName} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/${modNamespace}>)
target_link_libraries(${modName} PUBLIC Threads::Threads)
add_library(${modNamespace}::${modName} ALIAS ${modName})
set(installTargets ${modName})

if (OpenMP_CXX_FOUND)
    add_library(${modName}_OpenMP INTERFACE)
    target_link_libraries(${modName}_OpenMP INTERFACE
            ${modName} OpenMP::OpenMP_CXX)
    set_target_properties(${modName}_OpenMP PROPERTIES EXPORT_NAME OpenMP)
    add_library(${modNamespace}::OpenMP ALIAS ${modName}_OpenMP)
    list(APPEND installTargets ${modName}_OpenMP)
endif ()

if (MPI_CXX_FOUND)
    add_library(${modName}_MPI INTERFACE)
    target_link_libraries(${modName}_MPI INTERFACE ${modName} MPI::MPI_CXX)
    set_target_properties(${modName}_MPI PROPERTIES EXPORT_NAME MPI)
    add_library(${modNamespace}::MPI ALIAS ${modName}_MPI)
    list(APPEND installTargets ${modName}_MPI)
endif ()

# The headers of FastFlow are included as system headers, so that their
# warnings do not drown ours; installed, they go next to ours.
if (ASTAR_HAS_FASTFLOW)
    add_library(${modName}_FastFlow INTERFACE)
    target_include_directories(
            ${modName}_FastFlow SYSTEM INTERFACE
            $<BUILD_INTERFACE:${FASTFLOW_ROOT}>
            $<INSTALL_INTERFACE:include/${modNamespace}/fastflow>)
    target_link_libraries(${modName}_FastFlow INTERFACE ${modName})
    # FastFlow would pin its threads from the first core on, whatever the
    # CPUs given to the process: the workers are placed by pin_worker()
    target_compile_definitions(${modName}_FastFlow INTERFACE
            NO_DEFAULT_MAPPING)
    set_target_properties(${modName}_FastFlow PROPERTIES
            EXPORT_NAME FastFlow)
    add_library(${modNamespace}::FastFlow ALIAS ${modName}_FastFlow)
    list(APPEND installTargets ${modName}_FastFlow)
endif ()

message(STATUS "${modName} ${PROJECT_VERSION}: OpenMP ${OpenMP_CXX_FOUND}, "
        "MPI ${MPI_CXX_FOUND}, FastFlow ${ASTAR_HAS_FASTFLOW}")

# ----- Subdirectories ------------------------------------------------------ #
if (ASTAR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (ASTAR_BUILD_TOOLS)
    add_subdirectory(tools)
endif ()

if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

# ----- Install instructions ------------------------------------------------ #
# The following commands are used when installing the library
# and its CMake configuration files on the system.
install(TARGETS ${installTargets} EXPORT ${modName}Targets)

# Install the headers and the auto-generated configuration header.
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})
install(FILES ${PROJECT_BINARY_DIR}/${modName}Config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})
if (ASTAR_HAS_FASTFLOW)
    install(DIRECTORY ${FASTFLOW_ROOT}/ff
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace}/fastflow)
endif ()

# (Generate and) install the target import file, that allows other
# CMake projects to import the targets.
install(EXPORT ${modName}Targets
        NAMESPACE ${modNamespace}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Generate the package version file, that allows other
# CMake projects to know the version.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

# Generate the package configuration file, that allows other
# CMake projects to find the library with find_package().
configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/${modName}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Install the package version and configuration files.
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Install the README and LICENSE files.
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        ${CMAKE_CURRENT_LIST_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${modName})

# ----- Packaging support --------------------------------------------------- #
# The following commands allow one to build a proper package using CPack.
# See: https://cmake.org/cmake/help/latest/manual/cpack.1.html
if (NOT hasParent)
    set(CPACK_PACKAGE_VENDOR "Donato Meoli")
    set(CPACK_PACKAGE_CONTACT "Donato Meoli")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_LIST_DIR}/README.md")
    set(CPACK_SOURCE_IGNORE_FILES
            \\\\.git/
            \\\\.gitignore$
            \\\\.gitattributes$)
    include(CPack)
endif ()

# --------------------------------------------------------------------------- #
# Remove from the search paths the cmake directory we added at the beginning.
list(REMOVE_AT CMAKE_MODULE_PATH -1)

# --------------------------------------------------------------------------- #

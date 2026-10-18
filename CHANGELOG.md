# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- the sequential A* over any Domain, i.e., an initial state, a goal
  predicate and a procedure that generates the successors of a state with
  the cost of their edges and their heuristic estimates, with the OPEN list
  on two-level buckets for integer costs and on a 4-ary heap otherwise, the
  closed list in an open addressing table with the hash stored next to the
  pointer, the nodes in an arena on huge pages, the slots of the successors
  prefetched before their lookups, and the generation of the parent pruned
  for the domains that give the operator of a move
- the parallel searches of the survey of Fukunaga et al. (2017): SPA*, HDA*
  with the work distributions of Zobrist, abstract Zobrist, abstraction,
  perfect hashing, plain hashing and random hashing, SafePBNF, PA*SE and a
  master-worker farm with feedback, on C++ threads, OpenMP, FastFlow and
  MPI (HDAStarMPI, with the termination of Mattern)
- the domains of the sliding-tile puzzles (with the Manhattan and the
  pairwise heuristics), of the grids with 4 and 8 moves (random maps and the
  maps of Moving AI), of explicit graphs, and a domain with artificial work
  per expansion
- the best-first search with an incumbent, i.e., branch-and-bound, of a
  problem satisfying the concept BranchAndBound (root(), evaluate( n ) and
  branch( n , sink ), possibly a worker of its own per thread): the nodes
  evaluated when they are extracted, pruned when they are generated, when
  they are extracted and after their evaluation, the selections
  best-first, depth-first, best-estimate and plunging, the end when no open
  node can improve the incumbent beyond an absolute or relative tolerance,
  minimization and maximization, an initial incumbent, a callback on each
  new one, and node and time limits; sequential (BranchAndBoundSearch) and
  parallel with a shared queue (SharedBranchAndBound) or with distributed
  queues (DistributedBranchAndBound) on C++ threads, OpenMP and FastFlow,
  master-worker on a farm of FastFlow (FarmBranchAndBound), and with one
  MPI process per worker (MPIBranchAndBoundSearch), tested on
  the 0-1 knapsack against the dynamic program; the design is in
  docs/branch-and-bound.md
- SearchOptions::expected_nodes, which sizes the closed list at the start
- the examples after Russell and Norvig, whose results are tests: the
  8-puzzle (with the heuristics h1, h2 and that of Nilsson), the 15-puzzle,
  pathfinding on a weighted grid, route finding in Romania, the n queens,
  the water jugs and the travelling salesman with the bound of the minimum
  spanning tree
- the tester, with the instances of Korf, the grids and the explicit
  graphs, under AddressSanitizer, UndefinedBehaviorSanitizer and
  ThreadSanitizer in the CI
- tools/AStarBench and tools/AStarBenchMPI, one line of CSV per run, and
  tools/bench/campaign.sh and tools/bench/tables.py, the runs and the
  tables of the benchmarks
- the build with CMake (installed package found by
  find_package(AStarSearch)) and with the makefiles; the version from the
  git tag
- the documentation on the pages of the repository, a site of MkDocs
  Material written from the README by docs/pages.py with the reference of
  Doxygen (theme doxygen-awesome-css) under /api/, the line coverage on
  Coveralls, and the release workflow

### Changed

- the licence is the GNU Lesser General Public License version 3.0,
  instead of the MIT License

- the library is compiled (src/ holds what does not depend on the Domain)
  and written in C++20, instead of the single header of C++11 with linear
  searches in the OPEN and closed lists
- FastFlow is found in /opt/fastflow or on the include path, and CMake
  downloads a fixed commit when it is not there, instead of being a git
  submodule

[Unreleased]: https://github.com/dmeoli/AStarSearch/commits/main

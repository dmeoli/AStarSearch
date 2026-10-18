# Sequential and Parallel A* Search

[![CI](https://github.com/dmeoli/AStarSearch/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/dmeoli/AStarSearch/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/dmeoli/AStarSearch)](https://github.com/dmeoli/AStarSearch/releases)
[![Coverage Status](https://coveralls.io/repos/github/dmeoli/AStarSearch/badge.svg?branch=main)](https://coveralls.io/github/dmeoli/AStarSearch?branch=main)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://dmeoli.github.io/AStarSearch/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)](https://github.com/dmeoli/AStarSearch/actions/workflows/ci.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

AStarSearch is a C++20 library for the sequential and parallel A*
search [[9]](#ref-9) of an implicit state-space graph, i.e., a graph given by an
initial state, a goal predicate and a procedure that generates the
successors of a state (cf. [[10]](#ref-10)). With each successor the procedure also
gives the cost of the edge and an estimate h of the distance to a goal. The
parallel searches follow the taxonomy of the survey of Fukunaga et al. [[1]](#ref-1),
and they run on C++ threads, OpenMP, FastFlow [[12]](#ref-12) and MPI; hence, one can
compare the same algorithm across runtimes, and the same runtime across
algorithms.

## The search problem

We are concerned with finding a path of minimum cost from the initial state
to a goal state, in a graph with nonnegative edge costs, when h is
admissible (it never overestimates the cost of reaching a goal). For the
searches that do not reopen the closed nodes we also assume that h is
consistent, i.e., h(u) <= c(u, v) + h(v) for each edge (u, v). Under these
conditions the searches of the library return an optimal path (cf. [[2]](#ref-2) for
HDA* and [[3]](#ref-3) for Safe PBNF). Without consistency the sequential A* is still
exact, since it reopens a closed node reached through a cheaper path; the
parallel searches, however, may pay several re-expansions for the same node.

All the algorithms are templates over the problem, which is described by a
type satisfying the concept `Domain` of `include/AStarSearch/Domain.h`.
Such a type gives `state_type` and an arithmetic `cost_type`, together
with the methods
`initial_state()`, `is_goal( s )`, `heuristic( s )`, `hash( s )` and
`for_each_successor( s , h_s , f )`; the latter calls `f( t , c , h_t )` for
each successor t of s. Note that h(s) is passed in, so that h(t) can be
computed incrementally when the heuristic allows it (as the Manhattan
distance of the sliding-tile puzzle does). Since all calls are resolved at
compile time, no virtual function is paid in the inner loop, which is the
first design choice of the fast implementation of Burns et al. [[4]](#ref-4).

Some algorithms need more than a `Domain`, and each extra requirement is a
refinement of the concept. Thus, `FeatureDomain` describes a state as a set
of propositions (as Zobrist hashing requires), and `AbstractFeatureDomain`
adds a projection of the features, for Abstract Zobrist hashing [[5]](#ref-5).
`AbstractStateDomain` and `NBlockDomain` give an abstraction of the state
space (for AHDA* and PBNF [[3]](#ref-3)), `PairwiseDomain` a heuristic between any two
states (for PA*SE [[6]](#ref-6)), and `PerfectHashDomain` an injective map of the
states to the integers. As examples, the library provides the sliding-tile
puzzle of any size with the Manhattan distance (`SlidingTile.h`, which
satisfies all of the concepts) and pathfinding with 4 or 8 moves on a grid
with cell costs (`Grid.h`), read from a Moving AI map [[15]](#ref-15) or generated at
random. Also, `ExplicitGraph.h` holds a weighted graph given by its arcs,
which serves for the route-finding problem on the map of Romania of Russell
and Norvig [[13]](#ref-13).

## The algorithms

We start from the sequential search (`astar( d )` in `AStar.h`), which is A*
with the goal test at the expansion and a node reopened when a cheaper path
to it is found. Ties on f are broken in favour of the larger g, which on
domains with many states in the last f layer usually reaches the goal much
earlier. The parallel searches are the following, each one with the
reference for its correctness and its typical behavior.

- `SPAStar` (`SPAStar.h`) is the Simple Parallel A* of Algorithm 2 of [[1]](#ref-1),
  in which the workers share one OPEN and one CLOSED list, protected by
  locks. It is the baseline of the survey, and it is not expected to scale
  beyond a few threads, since each expansion goes through the shared lists.
- `HDAStar` (`HDAStar.h`) is Hash Distributed A* [[2]](#ref-2): a distribution
  function assigns each state to a worker, which keeps OPEN and CLOSED for
  the states it owns and receives, packed in batches, those generated by
  the others. The search ends when no worker has a node with f smaller than
  the incumbent and no batch is travelling. The distribution is a template
  parameter (`Distribution.h`), and the library provides those compared by
  Jinnai and Fukunaga [[5]](#ref-5), i.e., Zobrist hashing (HDA*[Z]), Abstract
  Zobrist hashing (AZHDA*), Zobrist hashing of an abstract state (AHDA*
  [[3]](#ref-3)) and the perfect hash modulo the number of workers (HDA*[P], the
  "bad" case of [[3]](#ref-3)). Furthermore, it provides the hash of the closed list
  and, as the baseline without duplicate detection across workers, a random
  assignment.
- `HDAStarMPI` (`HDAStarMPI.h`) is the same search with one MPI process per
  worker, a nonblocking exchange of the batches and the termination
  detection of Mattern [[11]](#ref-11) (four counters reduced with `MPI_Iallreduce`).
  At the end, the path is rebuilt state by state, each one sent by the
  process that holds it.
- `SafePBNF` (`SafePBNF.h`) is the livelock-free Parallel Best-NBlock-First
  search of Burns et al. [[3]](#ref-3), in which an abstraction partitions the state
  space into nblocks. A worker expands for a while the nodes of an nblock
  whose duplicate detection scope (its abstract successors) is not in use
  by another worker; hence, no lock is taken on the single node.
- `PASE` (`PASE.h`) is the Parallel A* for Slow Expansions of Phillips et
  al. [[6]](#ref-6), which expands in parallel only the states that the pairwise
  heuristic proves not to be reachable through a cheaper path from a state
  still in OPEN. It may pay off only when an expansion costs much more than
  the bookkeeping; `SlowDomain.h` adds a given amount of artificial work per
  expansion to study this regime.
- `FarmAStar` (`FarmAStar.h`) is a centralized parallel A* written as a
  FastFlow farm with feedback, i.e., the master-worker scheme of [[1]](#ref-1): a
  master holds OPEN and CLOSED, and the workers generate the successors of
  the nodes it sends them.

The shared-memory searches are templates also over the runtime that starts
the workers, i.e., `ThreadRuntime`, `OpenMPRuntime` (`OpenMPRuntime.h`) or
`FastFlowRuntime` (`FastFlowRuntime.h`); the latter runs HDA* as a mesh of
FastFlow single-producer single-consumer queues. Their options are in
`ParallelOptions`: the number of workers, the size of a batch, the number of
expansions between two flushes of a partially filled batch and the seed of
the distribution.

## Branch-and-bound

The same machinery runs a best-first search with an incumbent, i.e., a
Branch-and-Bound (B&B), on a problem satisfying the concept
`BranchAndBound` of `BranchAndBound.h`. Such a problem gives a copyable
`node_type`, a `sense` (minimization or maximization), the root node, and
two methods: `evaluate( n )` returns the bound of the node n, whether it is
infeasible and possibly a feasible value, while `branch( n , sink )` gives
the children of n, each with a bound that is cheap to compute.
Alternatively, `make_worker( i )` gives each worker an evaluator of its
own (e.g., with a copy of a model and of a solver). Note that a B&B tree has no duplicates;
hence, there is no closed list. A node is evaluated only when it is
extracted, which pays off when the bound is "expensive" (say, a Linear
Program), and it is pruned when it is generated, when it is extracted and
after its evaluation. Also, the nodes can be extracted best-first,
depth-first, by an estimate of the problem, or by plunging (a dive into
the best child, then best-first). The search ends when no open node can
improve the incumbent beyond an absolute or relative tolerance (usually
well before the queue is empty); it reports the bound and the gap also
when a node or time limit stops it.

The search is sequential (`branch_and_bound( p , options )`), parallel with
a queue shared by the workers as in SPA* (`SharedBranchAndBound`) or with a
queue per worker and the children sent to their owners as in HDA*
(`DistributedBranchAndBound`), both on the three runtimes, master-worker on
a FastFlow farm (`FarmBranchAndBound`), or with one MPI process per worker
(`MPIBranchAndBoundSearch`, whose nodes travel as bytes). SafePBNF and PA*SE
have no counterpart, since they exist to handle duplicates and
re-expansions, which a tree does not have. Its design is discussed in
[docs/branch-and-bound.md](docs/branch-and-bound.md), and the tests check
the variants on the 0-1 knapsack against the dynamic program.

## The data structures

OPEN is either a 4-ary heap, for any cost type, or a two-level bucket queue
(on f first, then on g) for integer costs, which [[4]](#ref-4) reports to be somewhat
faster than a heap on the sliding-tile puzzles. Neither one supports
decrease-key: an entry made stale by a cheaper path is skipped when it
surfaces. CLOSED (`NodeTable.h`) is an open-addressing hash table that
stores the hash of a state next to its node, so that most probes do not
compare the states, and spreads the slots by Fibonacci hashing; the nodes
are allocated from an arena. Both the table and the arena are requested
from the operating system as transparent huge pages, where available. In
the sequential search the slots of the successors are also prefetched
before being probed, since on large instances most probes are cache misses
(the parallel searches do the same with the batches they receive). Each
search returns a `Solution` holding the status (solved, unsolvable or
stopped at the node limit), the cost, the path and the counters of the
search; for the parallel ones, these include the load balance and the
communication overhead of [[2]](#ref-2).

## Building and using the library

The library needs only a C++20 compiler, together with CMake 3.21 or GNU
make. OpenMP, MPI and FastFlow are optional, and each of them is used if it
is found. FastFlow is header-only: CMake looks for it in `FASTFLOW_ROOT`
(`/opt/fastflow` by default) and on the include path, and if it is not
there downloads the commit `ASTAR_FASTFLOW_COMMIT` (unless
`ASTAR_FETCH_FASTFLOW` is `OFF`), while the makefiles take its headers from
`FF_ROOT` (`/opt/fastflow` by default). With CMake one has

```sh
git clone https://github.com/dmeoli/AStarSearch.git
cmake -S AStarSearch -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
cmake --install build --prefix <prefix>
```

where `ASTAR_WITH_OPENMP`, `ASTAR_WITH_MPI` and `ASTAR_WITH_FASTFLOW` (all
`ON` by default) can exclude a runtime, and `ASTAR_BUILD_TESTS`,
`ASTAR_BUILD_EXAMPLES` and `ASTAR_BUILD_TOOLS` (`ON` only when AStarSearch
is the top-level project) the tests, the examples and the benchmark. A CMake project then
uses the library with

```cmake
find_package(AStarSearch REQUIRED)
target_link_libraries(myTarget PRIVATE AStarSearch::AStarSearch)
```

adding `AStarSearch::OpenMP`, `AStarSearch::MPI` or `AStarSearch::FastFlow`
for the corresponding runtime. Since the searches are templates over the
`Domain`, they are in the headers, and the library (`src`) holds only what
does not depend on it, i.e., the grid maps, the random tables of Zobrist
hashing, the measures of a search and the parts that depend on the
operating system. Without CMake, the `makefile` in the root builds the
objects and exports them together with the include directive and the list
of the headers, so that the makefile of a program that uses the library
needs only to include it; in `examples`, `test`
and `tools` a makefile builds the corresponding programs, and in `test` the
targets `check` and `check_mpi` also run the tests.

As an example, a minimal program that solves the instance 1 of Korf [[7]](#ref-7)
sequentially, and then with HDA* and Abstract Zobrist hashing on 8 threads,
reads

```cpp
#include "AStarSearch/AStarSearch.h"
using namespace AStar_di_unipi_it;

int main( void )
{
 const FifteenPuzzle d( FifteenPuzzle::parse(
                         "14 13 15 7 11 12 9 5 6 0 2 1 4 8 10 3" ) );
 const auto s = astar( d );
 ParallelOptions o;
 o.threads = 8;
 const AbstractZobristDistribution< FifteenPuzzle > az( d );
 const auto p = HDAStar< FifteenPuzzle , decltype( az ) >( d , az , o )
                 .solve();
 return( ( s.cost == 57 ) && ( p.cost == 57 ) ? 0 : 1 );
 }
```

The documentation is at
[dmeoli.github.io/AStarSearch](https://dmeoli.github.io/AStarSearch/): a
site of Sphinx with the theme of Read the Docs, made of the sections of this
README, with the reference of the classes and of the functions that Exhale
and Breathe build from the comments of the headers (through Doxygen).
Locally, `pip install -r docs/requirements.txt`, then
`python3 docs/pages.py && sphinx-build -b html docs build-docs/html`.

## The examples and the benchmark

The programs in `examples` are, for the most part, the search problems of
chapter 3 of Russell and Norvig [[13]](#ref-13), each one with its own
`Domain` written in the file. `8Puzzle` solves the instance of figure 3.28
of the book, or an instance at a given distance from the goal, with the
number of misplaced tiles (h1), the Manhattan distance (h2) or the sequence
score of Nilsson [[8]](#ref-8), which is not admissible; with `-t` it
repeats the comparison of h1 and h2 of figure 3.29 (nodes generated and
effective branching factor). `15Puzzle` solves an instance of Korf,
sequentially or with HDA* (`-t <threads>`); `RouteFinding` finds a road
between two cities of the map of Romania of the book; `FindPath` finds a
path on a weighted grid, random or read from a Moving AI map, and draws it.
`VacuumWorld` is the vacuum world of section 3.2.1 on a row of n cells, and
`NQueens` places n queens with the incremental formulation of the same
section, checking the placement apart from the search. The last three come
from the exercises of the book: `MissionariesCannibals` carries m
missionaries and c cannibals over a river with a boat of b seats (exercise
3.9), `WaterJugs` measures a quantity of water with jugs (exercise 3.6(d)),
and `TravelingSalesman` solves the traveling salesperson problem with the
minimum spanning tree heuristic (exercise 3.30), checking the tour against
the dynamic program of Held and Karp [[14]](#ref-14). Each run with a known
answer (e.g., the 26 moves of figure 3.28, the 418 km from Arad to Bucharest
and the 11 crossings of the missionaries) is also a test of `ctest`,
labelled `example`. In `tools`, `AStarBench` runs any search, runtime and
distribution of the library on the 100 instances of Korf
(`data/15-puzzle/korf100.txt`), on the 8-puzzle and on the grids, and it
prints a line of CSV per run with the cost, the counters and the time;
`AStarBenchMPI` is its MPI counterpart, to be started with `mpirun`. If the
environment variable `ASTAR_PIN` is set, the i-th worker is pinned to the
i-th CPU of the affinity mask of the process (as given, e.g., by `numactl
--physcpubind` or `taskset`). On machines with several sockets this is
usually needed for the timings to be repeatable.

## Testing

The tester `test/test.cpp` checks every search against references that do
not come from the library, i.e., a breadth-first search of the whole state
space of the 8-puzzle, the optimal costs of the 100 instances of Korf
[[7]](#ref-7), Dijkstra's algorithm on random grids and the distances on the
map of Romania; for each solution it checks both the cost and the path,
which must start at the initial state, end at a goal and be made of edges
of the `Domain` whose costs add up to the one reported. Its cases (the
sequential A*, then each parallel search on each runtime with 1 to 8
workers, and HDA* over MPI with 1, 2 and 4 processes) are listed in
`test/tests.txt`, which both CMake and the makefiles read, and the case
`registry` checks that the list and the cases of the tester are the same;
`ctest` runs them together with the examples of known answer. The CI runs
them on Linux with gcc and clang, on macOS and on Windows (without
FastFlow and MPI there), with the makefiles, and once more under
AddressSanitizer with UndefinedBehaviorSanitizer and under ThreadSanitizer,
which checks the synchronization of the searches in shared memory; the
line coverage of the tests is on Coveralls.

## Citing

If AStarSearch is useful in a publication, one can cite it as

```bibtex
@software{AStarSearch,
  author = {Meoli, Donato},
  title  = {{AStarSearch}: sequential and parallel {A*} search},
  url    = {https://github.com/dmeoli/AStarSearch},
  year   = {2026}
}
```

## License

This software is released under the [GNU Lesser General Public License
version 3.0](https://opensource.org/licenses/lgpl-3.0.html) (see the file
[LICENSE](LICENSE)). Its sequential version was first written for the
course of Knowledge Engineering of prof.
[Floriana Esposito](https://www.uniba.it/it/docenti/esposito-floriana) at
the University of Bari, and the parallel ones for the course of Parallel and
Distributed Systems of prof.
[Marco Danelutto](https://www.unipi.it/en/about-us/organisation/people/marco-danelutto-4278-en/)
and prof.
[Massimo Torquati](https://www.unipi.it/en/about-us/organisation/people/massimo-torquati-78079-en/)
at the University of Pisa.

## References

<a id="ref-1"></a>[1] A. Fukunaga, A. Botea, Y. Jinnai and A. Kishimoto, [*A Survey of Parallel A\**](https://arxiv.org/abs/1708.05296), arXiv:1708.05296, 2017.

<a id="ref-2"></a>[2] A. Kishimoto, A. Fukunaga and A. Botea, [*Evaluation of a simple, scalable, parallel best-first search strategy*](https://doi.org/10.1016/j.artint.2012.10.007), Artificial Intelligence 195, 222-248, 2013.

<a id="ref-3"></a>[3] E. Burns, S. Lemons, W. Ruml and R. Zhou, [*Best-first heuristic search for multicore machines*](https://doi.org/10.1613/jair.3094), Journal of Artificial Intelligence Research 39, 689-743, 2010.

<a id="ref-4"></a>[4] E. Burns, M. Hatem, M. J. Leighton and W. Ruml, [*Implementing fast heuristic search code*](https://doi.org/10.1609/socs.v3i1.18245), Proceedings of the Symposium on Combinatorial Search (SoCS) 3, 25-32, 2012.

<a id="ref-5"></a>[5] Y. Jinnai and A. Fukunaga, [*On hash-based work distribution methods for parallel best-first search*](https://doi.org/10.1613/jair.5225), Journal of Artificial Intelligence Research 60, 491-548, 2017.

<a id="ref-6"></a>[6] M. Phillips, M. Likhachev and S. Koenig, [*PA\*SE: Parallel A\* for slow expansions*](https://doi.org/10.1609/icaps.v24i1.13652), Proceedings of the International Conference on Automated Planning and Scheduling (ICAPS) 24, 208-216, 2014.

<a id="ref-7"></a>[7] R. E. Korf, [*Depth-first iterative-deepening: an optimal admissible tree search*](https://doi.org/10.1016/0004-3702(85)90084-0), Artificial Intelligence 27, 97-109, 1985.

<a id="ref-8"></a>[8] N. J. Nilsson, *Principles of Artificial Intelligence*, Tioga, 1980.

<a id="ref-9"></a>[9] P. E. Hart, N. J. Nilsson and B. Raphael, [*A formal basis for the heuristic determination of minimum cost paths*](https://doi.org/10.1109/TSSC.1968.300136), IEEE Transactions on Systems Science and Cybernetics 4(2), 100-107, 1968.

<a id="ref-10"></a>[10] S. Edelkamp and S. Schroedl, [*Heuristic Search: Theory and Applications*](https://doi.org/10.1016/C2009-0-16511-X), Morgan Kaufmann, 2012.

<a id="ref-11"></a>[11] F. Mattern, [*Algorithms for distributed termination detection*](https://doi.org/10.1007/BF01782776), Distributed Computing 2(3), 161-175, 1987.

<a id="ref-12"></a>[12] M. Aldinucci, M. Danelutto, P. Kilpatrick and M. Torquati, [*FastFlow: high-level and efficient streaming on multicore*](https://doi.org/10.1002/9781119332015.ch13), in Programming Multi-core and Many-core Computing Systems, Wiley, 261-280, 2017.

<a id="ref-13"></a>[13] S. Russell and P. Norvig, [*Artificial Intelligence: A Modern Approach*](https://aima.cs.berkeley.edu/), 3rd edition, Prentice Hall, 2010.

<a id="ref-14"></a>[14] M. Held and R. M. Karp, [*A dynamic programming approach to sequencing problems*](https://doi.org/10.1137/0110015), Journal of the Society for Industrial and Applied Mathematics 10(1), 196-210, 1962.

<a id="ref-15"></a>[15] N. R. Sturtevant, [*Benchmarks for grid-based pathfinding*](https://doi.org/10.1109/TCIAIG.2012.2197681), IEEE Transactions on Computational Intelligence and AI in Games 4(2), 144-148, 2012.

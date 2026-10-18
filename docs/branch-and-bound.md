# Best-first search with an incumbent: design note

We are concerned with extending the searches of the library to
Branch-and-Bound (B&B), i.e., to the minimization of a function over a
finite (but possibly huge) set that is recursively partitioned, each part
(a *node*) carrying a lower bound on the values it contains and, possibly,
a feasible solution. In fact, A* is a special case of this
scheme, where the bound of a node is f = g + h and a goal gives a feasible
solution of value g.
However, the general case differs in five respects that the `Domain`
concept does not cover. First, the tree of a B&B has no duplicates, and
therefore no closed list is needed. Second, the search does not stop at the
first feasible solution, but when no open node can improve the best one
found so far (the *incumbent*), or when the gap between the two is within a
tolerance. Third, the bound of a node is usually "expensive" (e.g., the
optimal value of a Linear Program), and hence one wants to compute it when
the node is extracted, the node inheriting the bound of its parent until
then. Fourth, the order of extraction is a policy (best-first, depth-first,
best-estimate, or a depth-first dive from the best node). Fifth, the
objective may be maximized, and the bounds are real numbers compared with
tolerances. The structure of this note is as follows: we first give the
concept of a B&B problem, then the sequential search, then how the parallel
algorithms of the library map onto it (and which ones do not), and finally
the interface that a B&B solver built on a modeling system would use.

## The concept

A type P is a `BranchAndBound` problem if it provides the copyable type
`node_type`, a constant `sense` (`Sense::minimize` or `Sense::maximize`),
and three methods, all the values being doubles. The method
`root()` returns the node of the whole set. The method `evaluate( n )`
computes the bound of the node n, possibly storing in n what the branching
needs (say, the fractional solution of a relaxation), and returns an
`Evaluation` made of the bound, a flag for an infeasible node and an
optional feasible value, of which n is the solution. Also,
`branch( n , sink )` calls `sink( child , estimate )` for the children of
the evaluated node n, where `estimate` is a valid bound of the child that
is "cheap" to compute, typically the bound of n; the search uses it to
order the child and to prune it before its evaluation.

Optionally, P may provide `hash( n )` for the distribution of the nodes
among the workers, `depth( n )` for the depth-first policies (otherwise the
search counts the levels), and `make_worker( i )`. The latter returns, for
the worker i of a parallel search, an object with the same `evaluate` and
`branch`: this is needed when the evaluation keeps a state (e.g., a model
and a solver that are modified at each node), since then each worker has
its copy and P is only read. A maximization problem is solved by negating
the values internally; thus, the comparisons below are those of a
minimization.

Note that the `Domain` concept and the searches that use it are left
untouched: the B&B layer is a new concept with its searches (`BranchAndBound.h` and
`ParallelBranchAndBound.h`), which share with A* the runtimes, the
placement of the workers (cf. `pin_worker()`) and the detection of the
termination. Yet, they do not share the node table or the OPEN lists of A*,
which are specialized for integer costs and for the lazy deletion of
duplicates. We remark that A* is not rewritten on top of the new layer,
since this would add an indirection to its inner loop for features that it
does not use.

## The sequential search

We keep the open nodes in a pool with a free list (a node is
destroyed when it is branched, while in A* a generated node stays in the
closed list) and a priority queue of pairs (key, index). Its key depends on the policy: the bound for best-first, minus the depth for depth-first, and
a problem-given estimate of the best feasible value in the subtree for
best-estimate. In the dive (*plunging*), the children of the node just
branched are extracted depth-first as long as their bound is within a
given fraction of the gap from the best bound, after which the search goes
back to best-first. At an extraction, a node whose inherited bound cannot
improve the incumbent by more than the tolerance is dropped without being
evaluated. Otherwise it is evaluated, and it is dropped if it is infeasible
or if its bound cannot improve the incumbent; its feasible value, if any,
updates the incumbent and calls the `on_incumbent` callback of the options
(which gives the anytime behavior). Then the node is branched, and a child
whose estimate cannot improve the incumbent is dropped at once.

The search ends when the queue is empty, or when the best bound among the
open nodes is within the tolerance of the incumbent, which we take as
max(abs_tol, rel_tol |z|) for the incumbent value z. With a policy other
than best-first the best open bound is not at the top of the queue, and
therefore a second heap of the bounds (with lazy deletion) keeps it; in
this way the termination test and the final gap are exact for all the
policies. The `Solution` reports the incumbent and its node, the best
bound, the gap, the nodes evaluated, branched and pruned (at generation and
at extraction), and the status, i.e., optimal within the tolerance,
infeasible, or stopped by a node or time limit (where the gap is still
valid). An initial incumbent value, e.g., from a heuristic, can be given
in the options.

## The parallel searches

We now turn to the parallel searches, which differ in how they share the
open nodes. Since a B&B tree has no duplicates, the algorithms whose purpose is
the balance of the load carry over, while those whose purpose is the
detection of duplicates or the avoidance of re-expansions do not.

- The shared queue of SPA* gives the classical centralized parallel B&B:
  the workers extract from one queue under a lock and evaluate the nodes
  outside of it. The contention is rather low when the evaluation dominates (as
  is usually the case in B&B), and the order of extraction is close to the
  sequential one.
- The distribution of HDA* gives a decentralized B&B with work sharing.
  Each worker has a queue, the children are sent in batches to the owner
  given by the hash of the node (or chosen at random when P gives no hash),
  and the termination is that of HDA*, i.e., a counter of the nodes in
  flight on the threads and the algorithm of Mattern over MPI. The
  incumbent is an atomic variable (a reduction over MPI). The distribution
  has no duplicates to detect here, and its only role is to balance the
  load; hence, Zobrist and random hashing are of use, while the
  abstraction-based distributions are not.
- The farm with feedback (FastFlow) gives the master-worker B&B: the
  master holds the queue and the incumbent, and the workers evaluate and
  branch the nodes it sends, returning the children. This scheme is
  probably the best suited to an expensive evaluation, since the order of
  extraction is the one of the policy and the master is idle most of the
  time.
- SafePBNF and PA*SE do not apply. The nblocks of PBNF make the detection
  of duplicates local, and the independence test of PA*SE avoids the
  re-expansions; a tree has neither, and PA*SE without duplicates would be
  a shared queue with a costly test at each extraction.

Over MPI, the distributed search has to send the nodes between processes,
and it therefore requires P to provide `serialize( n , out )` and
`deserialize( in )` (or a trivially copyable node). Each process then has a
queue, the incumbent value is sent to all the processes when it improves,
the node of the incumbent stays on the process that found it until the end
(when it is sent to all), and the end is detected with the method of
Mattern, as in HDA* over MPI; the node limit is split evenly among the
processes.

## The interface for a B&B solver

A B&B solver built on a modeling system would provide a problem whose
`node_type` is the list of the branching decisions from the root (e.g.,
the bounds changed on the variables, which is "small" and hence cheap to
copy and to send). Its `make_worker( i )` would return an object holding a
copy of the model and of the solver of the relaxation. The `evaluate` of a
worker applies the decisions of the node to its model (possibly undoing
only those that differ from the previous node, which makes a dive cheap),
solves the relaxation, and returns its value as the bound, its
infeasibility, and its value as a feasible one if the solution is
integral. Its `branch` chooses a fractional variable in the stored
solution and gives the two children, with the bound of the parent as their
estimate. Such a solver then calls one of

    auto s = branch_and_bound( problem , options );
    auto s = SharedBranchAndBound< P , Runtime >( problem , options ).solve();
    auto s = DistributedBranchAndBound< P , Runtime >( problem , options )
              .solve();
    auto s = FarmBranchAndBound< P >( problem , options ).solve();
    auto s = MPIBranchAndBoundSearch< P >( problem , options , comm ).solve();

and reads the incumbent, the bound and the gap from the `Solution`. In
the options one gives the policy, the tolerances, the initial incumbent
and the callback, and for the parallel searches the number of workers and
the batch size; the scheme is the class, and the runtime (C++ threads,
OpenMP or FastFlow) its second parameter. We expect the choice among the
schemes to depend somewhat on the cost of an evaluation w.r.t. that of
moving a node, which only a measurement on the instances of interest can
settle.

## The test

As a test of the extension we use the 0-1 knapsack problem with the bound
of the fractional relaxation of Dantzig. Its items are sorted by decreasing
ratio of profit to weight, and a node fixes the first k of them; its bound
is that of the greedy fractional completion, the greedy integer completion
gives a feasible value, and the branching fixes the critical item to 1 and
to 0. Hence, one can check the optimum against the dynamic program on the capacity,
on random instances of the uncorrelated, weakly correlated and strongly
correlated classes of Pisinger with up to a few hundred items. We run the policies sequentially and the parallel schemes with 1 to 8
workers, as a maximization (which exercises the change of sense), and with
both a zero and a positive relative tolerance; in the latter case the value
found has to be within the tolerance of the optimum.

/*--------------------------------------------------------------------------*/
/*--------------------------- File AStarSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file that includes all of AStarSearch save the parts that need an
 * external library: OpenMPRuntime.h (OpenMP), FastFlowRuntime.h and
 * FarmAStar.h (FastFlow), HDAStarMPI.h (MPI).
 *
 * The library searches a Domain [see Domain.h], i.e., an implicit graph
 * given by an initial state, a goal test and an expansion procedure, for a
 * path of least cost, with:
 *
 * - AStar, the sequential A* [see AStar.h];
 *
 * - SPAStar, Simple Parallel A*, with one OPEN list shared by the workers
 *   [see SPAStar.h];
 *
 * - HDAStar, Hash Distributed A*, in which a distribution function
 *   [see Distribution.h] assigns every state to the worker that owns it,
 *   on the C++ threads, OpenMP or FastFlow [see HDAStar.h], and HDAStarMPI
 *   on the processes of MPI [see HDAStarMPI.h];
 *
 * - SafePBNF, Parallel Best-NBlock-First search [see SafePBNF.h];
 *
 * - PASE, Parallel A* for Slow Expansions [see PASE.h];
 *
 * - FarmAStar, a master-worker A* on a FastFlow farm [see FarmAStar.h].
 *
 * The domains of the library are SlidingTile (the (W x H - 1)-puzzle),
 * SequenceScore (the same with the heuristic of Nilsson), Grid (pathfinding
 * on a GridMap with 4 or 8 moves) and ExplicitGraph (a weighted graph given
 * by adjacency lists); SlowDomain slows down the expansions of any of them.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINITIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AStarSearch
 #define __AStarSearch
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AStar.h"

#include "Distribution.h"

#include "Domain.h"

#include "ExplicitGraph.h"

#include "Grid.h"

#include "HDAStar.h"

#include "PASE.h"

#include "SPAStar.h"

#include "SafePBNF.h"

#include "SequenceScore.h"

#include "SlidingTile.h"

#include "SlowDomain.h"

/*--------------------------------------------------------------------------*/

#endif  /* __AStarSearch */

/*--------------------------------------------------------------------------*/
/*------------------------- End File AStarSearch.h -------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ File astar.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file that includes the graph searches of PBeFS save the parts
 * that need an external library: FarmAStar.h (FastFlow) and HDAStarMPI.h
 * (MPI).
 *
 * The searches look for a path of least cost in a Domain [see Domain.h],
 * i.e., an implicit graph given by an initial state, a goal test and an
 * expansion procedure, with:
 *
 * - AStar, the sequential A* [see AStar.h];
 *
 * - SPAStar, Simple Parallel A*, with one OPEN list shared by the workers
 *   [see SPAStar.h];
 *
 * - HDAStar, Hash Distributed A*, in which a distribution function
 *   [see Distribution.h] assigns every state to the worker that owns it,
 *   on a runtime of the core [see core.h], and HDAStarMPI on the processes
 *   of MPI [see HDAStarMPI.h];
 *
 * - SafePBNF, Parallel Best-NBlock-First search [see SafePBNF.h];
 *
 * - PASE, Parallel A* for Slow Expansions [see PASE.h];
 *
 * - FarmAStar, a master-worker A* on a FastFlow farm [see FarmAStar.h].
 *
 * The domains are SlidingTile (the (W x H - 1)-puzzle), SequenceScore (the
 * same with the heuristic of Nilsson), Grid (pathfinding on a GridMap with
 * 4 or 8 moves) and ExplicitGraph (a weighted graph given by adjacency
 * lists); SlowDomain slows down the expansions of any of them.
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

#ifndef __PBeFS_astar
 #define __PBeFS_astar
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "PBeFS/core.h"

#include "PBeFS/astar/AStar.h"

#include "PBeFS/astar/Distribution.h"

#include "PBeFS/astar/Domain.h"

#include "PBeFS/astar/ExplicitGraph.h"

#include "PBeFS/astar/Grid.h"

#include "PBeFS/astar/HDAStar.h"

#include "PBeFS/astar/PASE.h"

#include "PBeFS/astar/SPAStar.h"

#include "PBeFS/astar/SafePBNF.h"

#include "PBeFS/astar/SequenceScore.h"

#include "PBeFS/astar/SlidingTile.h"

#include "PBeFS/astar/SlowDomain.h"

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_astar */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File astar.h ----------------------------*/
/*--------------------------------------------------------------------------*/

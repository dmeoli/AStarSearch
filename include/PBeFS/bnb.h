/*--------------------------------------------------------------------------*/
/*------------------------------- File bnb.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file that includes the tree searches of PBeFS, i.e., the
 * best-first searches with an incumbent (Branch-and-Bound) of a problem
 * satisfying the concept BranchAndBound, save the parts that need an
 * external library: FarmBranchAndBound.h (FastFlow) and MPIBranchAndBound.h
 * (MPI). They are the sequential BranchAndBoundSearch [see
 * BranchAndBound.h], SharedBranchAndBound and DistributedBranchAndBound
 * on a runtime of the core [see ParallelBranchAndBound.h and core.h], and
 * DeterministicBranchAndBound, by synchronous rounds whose outcome does not
 * depend on the timing of the workers [see DeterministicBranchAndBound.h].
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

#ifndef __PBeFS_bnb
 #define __PBeFS_bnb
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "PBeFS/core.h"

#include "PBeFS/bnb/BranchAndBound.h"

#include "PBeFS/bnb/ParallelBranchAndBound.h"

#include "PBeFS/bnb/DeterministicBranchAndBound.h"

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_bnb */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File bnb.h -----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ File core.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file that includes the core of PBeFS, i.e., what does not depend
 * on the kind of search: the helpers [see SearchUtils.h], the runtime of
 * the C++ threads [see ThreadRuntime.h], and the messages between the
 * workers with the detection of the end [see ThreadMailbox.h]. The other runtimes need an
 * external library, and they are included one by one: OpenMPRuntime.h
 * (OpenMP), FastFlowRuntime.h (FastFlow) and MPIMailbox.h (MPI).
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

#ifndef __PBeFS_core
 #define __PBeFS_core
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "PBeFS/core/SearchUtils.h"

#include "PBeFS/core/ThreadMailbox.h"

#include "PBeFS/core/ThreadRuntime.h"

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_core */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File core.h -----------------------------*/
/*--------------------------------------------------------------------------*/

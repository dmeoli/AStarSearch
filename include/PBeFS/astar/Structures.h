/*--------------------------------------------------------------------------*/
/*--------------------------- File Structures.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the choice of the set of data structures of A*: the arena
 * of the nodes (NodePool), the closed list (NodeTable) and the OPEN lists
 * (HeapOpenList and BucketOpenList). The set is that of the library unless
 * one of the macros PBEFS_STRUCTURES_STD, PBEFS_STRUCTURES_BOOST and
 * PBEFS_STRUCTURES_ADS is defined, which CMake does by the option
 * PBEFS_STRUCTURES (own, std, boost or ads) and the makefile by
 * PBeFSSTRUCT; then Node.h, NodeTable.h and OpenList.h include the
 * structures of the standard library, of Boost or of ADS from
 * structures/std/, structures/boost/ or structures/ads/ in place of their
 * own, with the same interface. The searches are the same with every set,
 * and so is the order of their expansions but for the ties of f and g in
 * OPEN, which the sets break differently; the sets exist to measure the
 * searches on each of them, and that of the library is the default.
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

#ifndef __PBeFS_Structures
 #define __PBeFS_Structures
                      /* self-identification: #endif at the end of the file */

#if ( defined( PBEFS_STRUCTURES_STD ) + defined( PBEFS_STRUCTURES_BOOST ) + \
      defined( PBEFS_STRUCTURES_ADS ) ) > 1
 #error "PBeFS: more than one set of data structures is chosen"
#endif

#if ! ( defined( PBEFS_STRUCTURES_STD ) || defined( PBEFS_STRUCTURES_BOOST ) \
        || defined( PBEFS_STRUCTURES_ADS ) )
 /// defined when the data structures are those of the library
 #define PBEFS_OWN_STRUCTURES
#endif

/*--------------------------------------------------------------------------*/

#endif  /* __PBeFS_Structures */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Structures.h --------------------------*/
/*--------------------------------------------------------------------------*/

##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of the tools of AStarSearch: AStarBench, with the threads,      #
#   OpenMP and FastFlow, and AStarBenchMPI, the same source compiled with    #
#   mpicxx for HDAStarMPI                                                    #
#                                                                            #
#                                Donato Meoli                                #
#                         Dipartimento di Informatica                        #
#                             Universita' di Pisa                            #
#                                                                            #
##############################################################################

# basic directory
DIR = .

# common flags
COMMON_SW = -std=c++20 -Wall -Wextra

# debug switches
SW_DEBUG = -g -O0 $(COMMON_SW)
# production switches
SW_RELEASE = -O3 $(COMMON_SW) -DNDEBUG

# compilers
CC = g++
MPICC = mpicxx

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: release

# debug target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

debug: SW = $(SW_DEBUG)
debug: $(DIR)/AStarBench

# release target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

release: SW = $(SW_RELEASE)
release: $(DIR)/AStarBench

# mpi target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

mpi: SW = $(SW_RELEASE)
mpi: $(DIR)/AStarBenchMPI

# clean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)/*.o $(DIR)/*~ $(DIR)/AStarBench $(DIR)/AStarBenchMPI

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -

# AStarSearch
AStSDR = ..
include $(AStSDR)/makefile-c

# the programs- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

$(DIR)/AStarBench: $(DIR)/AStarBench.cpp $(AStH) $(AStOBJ)
	$(CC) -o $@ $< $(AStOBJ) $(AStINC) $(AStDEF) $(AStLIB) $(SW)

$(DIR)/AStarBenchMPI: $(DIR)/AStarBench.cpp $(AStH) $(AStMPIH) $(AStOBJ)
	$(MPICC) -o $@ $< $(AStOBJ) $(AStINC) -DASTAR_BENCH_MPI -pthread $(SW)

# distclean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

distclean: clean

# phony targets - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

.PHONY: debug release mpi clean distclean

############################ End of makefile #################################

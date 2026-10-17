##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for AStarSearch)                                       #
#                                                                            #
#   make builds the tester with the threads, OpenMP and FastFlow, make mpi   #
#   the one with MPI too (compiled with mpicxx), make check runs all the     #
#   cases as ctest does.                                                     #
#                                                                            #
#                                Donato Meoli                                #
#                         Dipartimento di Informatica                        #
#                             Universita' di Pisa                            #
#                                                                            #
##############################################################################

# module name
NAME = AStarSearch_test

# basic directory
DIR = .

# common flags
COMMON_SW = -std=c++20 -Wall -Wextra -Wpedantic

# debug switches
SW_DEBUG = -g -O0 $(COMMON_SW)
# debug switches with address sanitizer
SW_DEBUG_ASAN = -g3 -O1 $(COMMON_SW) -fsanitize=undefined -fsanitize=address \
                -fno-omit-frame-pointer
# production switches
SW_RELEASE = -O3 $(COMMON_SW) -DNDEBUG

# compiler
CC = g++
MPICC = mpicxx

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: release

# debug target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

debug: SW = $(SW_DEBUG)
debug: $(DIR)/$(NAME)

# debug_asan target - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

debug_asan: SW = $(SW_DEBUG_ASAN)
debug_asan: $(DIR)/$(NAME)

# release target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

release: SW = $(SW_RELEASE)
release: $(DIR)/$(NAME)

# mpi target: the tester with MPI too - - - - - - - - - - - - - - - - - - - -

mpi: SW = $(SW_RELEASE)
mpi: $(DIR)/$(NAME)_mpi

# check target: all the cases - - - - - - - - - - - - - - - - - - - - - - - -

CASES = astar grid graph limits reopen hda-thread spa-thread pbnf-thread \
        pase-thread hda-openmp spa-openmp pbnf-openmp hda-fastflow \
        farm-fastflow

check: release
	@for c in $(CASES); do $(DIR)/$(NAME) $$c ../data | tail -1 || exit 1; done

check_mpi: mpi
	@for n in 1 2 4; do mpiexec -n $$n $(DIR)/$(NAME)_mpi mpi ../data \
	| tail -1 || exit 1; done

# clean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)/*.o $(DIR)/*~ $(DIR)/$(NAME) $(DIR)/$(NAME)_mpi

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libraries + -L< libdirs >
# *H   is the list of all include files
# *INC is the -I< include directories >

# AStarSearch
AStSDR = ..
include $(AStSDR)/makefile-c

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ = $(AStOBJ)

# libraries
MLIB = $(AStLIB)

$(DIR)/$(NAME): $(DIR)/test.cpp $(AStH) $(MOBJ)
	$(CC) -o $@ $(DIR)/test.cpp $(MOBJ) $(AStINC) -I$(DIR) $(AStDEF) \
	$(MLIB) $(SW)

$(DIR)/$(NAME)_mpi: $(DIR)/test.cpp $(AStH) $(AStMPIH) $(MOBJ)
	$(MPICC) -o $@ $(DIR)/test.cpp $(MOBJ) $(AStINC) -I$(DIR) $(AStDEF) \
	-DASTAR_HAS_MPI $(MLIB) $(SW)

# the configuration header that CMake writes in the build directory
$(DIR)/AStarSearchConfig.h:
	printf '#define AStarSearch_VERSION "%s"\n' \
	"$$(git -C $(AStSDR) describe --tags --abbrev=0 2>/dev/null || echo 0.0.0)" \
	> $@

$(DIR)/$(NAME) $(DIR)/$(NAME)_mpi: $(DIR)/AStarSearchConfig.h

# distclean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

distclean: clean
	rm -f $(DIR)/AStarSearchConfig.h

# phony targets - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

.PHONY: debug debug_asan release mpi check check_mpi clean distclean

############################ End of makefile #################################

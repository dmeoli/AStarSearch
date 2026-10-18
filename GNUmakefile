###############################################################################
#                               GNUmakefile                                   #
#                                                                             #
#   The targets of the whole repository, read by GNU make before the file     #
#   makefile, which stays the makefile of the library that the makefiles of   #
#   examples/, test/ and tools/ include.                                      #
#                                                                             #
#   make             builds the examples, the tools and the tester            #
#   make test        runs the cases of test/tests.txt (MPI apart)             #
#   make test_mpi    runs the cases with MPI under mpiexec                    #
#   make bench       builds the benchmark tools                               #
#   make install     builds lib/libAStarSearch.a and copies it, with the      #
#                    headers, into $(PREFIX)/lib and $(PREFIX)/include        #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
###############################################################################

PREFIX ?= /usr/local

all:
	$(MAKE) -C examples
	$(MAKE) -C tools
	$(MAKE) -C test

test:
	$(MAKE) -C test check

test_mpi:
	$(MAKE) -C test check_mpi

bench:
	$(MAKE) -C tools release mpi

# the library, from the objects of the makefile of the library

AStSDR = .
CC = g++
SW = -O3 -std=c++20 -Wall -Wextra -DNDEBUG
include makefile

lib/libAStarSearch.a: $(AStOBJ)
	mkdir -p lib
	ar rcs $@ $(AStOBJ)

install: lib/libAStarSearch.a
	mkdir -p $(PREFIX)/include $(PREFIX)/lib
	cp -r include/AStarSearch $(PREFIX)/include/
	cp lib/libAStarSearch.a $(PREFIX)/lib/

clean::
	$(MAKE) -C examples clean
	$(MAKE) -C tools clean
	$(MAKE) -C test clean
	rm -rf lib

.PHONY: all test test_mpi bench install clean

############################# End of GNUmakefile ##############################

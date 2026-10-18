##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of AStarSearch                                                  #
#                                                                            #
#   The searches of AStarSearch are templates over the Domain, and they are  #
#   in the headers; the objects hold the parts that do not depend on it.     #
#                                                                            #
#   Input:  $(CC)      = compiler command                                    #
#           $(SW)      = compiler options                                    #
#           $(AStSDR)  = the directory where AStarSearch is                  #
#                                                                            #
#   Output: $(AStOBJ)  = the final object(s) / library                       #
#           $(AStH)    = the .h files to include                             #
#           $(AStINC)  = the -I$( include directory )                        #
#                                                                            #
#                                Donato Meoli                                #
#                         Dipartimento di Informatica                        #
#                             Universita' di Pisa                            #
#                                                                            #
##############################################################################

# macros to be exported - - - - - - - - - - - - - - - - - - - - - - - - - - -

AStOBJ = $(AStSDR)/obj/Distribution.o \
         $(AStSDR)/obj/Grid.o \
         $(AStSDR)/obj/SearchUtils.o \
         $(AStSDR)/obj/Solution.o

AStINC = -I$(AStSDR)/include

AStH   = $(AStSDR)/include/AStarSearch/AStar.h \
         $(AStSDR)/include/AStarSearch/AStarSearch.h \
         $(AStSDR)/include/AStarSearch/BranchAndBound.h \
         $(AStSDR)/include/AStarSearch/Distribution.h \
         $(AStSDR)/include/AStarSearch/Domain.h \
         $(AStSDR)/include/AStarSearch/ExplicitGraph.h \
         $(AStSDR)/include/AStarSearch/Grid.h \
         $(AStSDR)/include/AStarSearch/HDAStar.h \
         $(AStSDR)/include/AStarSearch/Node.h \
         $(AStSDR)/include/AStarSearch/NodeTable.h \
         $(AStSDR)/include/AStarSearch/OpenList.h \
         $(AStSDR)/include/AStarSearch/PASE.h \
         $(AStSDR)/include/AStarSearch/ParallelBranchAndBound.h \
         $(AStSDR)/include/AStarSearch/SPAStar.h \
         $(AStSDR)/include/AStarSearch/SafePBNF.h \
         $(AStSDR)/include/AStarSearch/SearchUtils.h \
         $(AStSDR)/include/AStarSearch/SequenceScore.h \
         $(AStSDR)/include/AStarSearch/SlowDomain.h \
         $(AStSDR)/include/AStarSearch/SlidingTile.h \
         $(AStSDR)/include/AStarSearch/Solution.h

# the headers of the optional runtimes- - - - - - - - - - - - - - - - - - - -

AStOMPH = $(AStSDR)/include/AStarSearch/OpenMPRuntime.h
AStFFH  = $(AStSDR)/include/AStarSearch/FastFlowRuntime.h \
          $(AStSDR)/include/AStarSearch/FarmAStar.h \
          $(AStSDR)/include/AStarSearch/FarmBranchAndBound.h
AStMPIH = $(AStSDR)/include/AStarSearch/HDAStarMPI.h

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(AStOBJ) $(AStSDR)/*~ $(AStSDR)/include/AStarSearch/*~ $(AStSDR)/src/*~

# dependencies: every .o from its .cpp + every recursively included .h- - - -

$(AStSDR)/obj/Distribution.o: $(AStSDR)/src/Distribution.cpp $(AStH)
	$(CC) -c $(AStSDR)/src/Distribution.cpp -o $@ $(AStINC) $(SW)

$(AStSDR)/obj/Grid.o: $(AStSDR)/src/Grid.cpp $(AStH)
	$(CC) -c $(AStSDR)/src/Grid.cpp -o $@ $(AStINC) $(SW)

$(AStSDR)/obj/SearchUtils.o: $(AStSDR)/src/SearchUtils.cpp $(AStH)
	$(CC) -c $(AStSDR)/src/SearchUtils.cpp -o $@ $(AStINC) $(SW)

$(AStSDR)/obj/Solution.o: $(AStSDR)/src/Solution.cpp $(AStH)
	$(CC) -c $(AStSDR)/src/Solution.cpp -o $@ $(AStINC) $(SW)

############################ End of makefile #################################

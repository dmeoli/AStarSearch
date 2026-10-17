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

AStH   = $(AStSDR)/include/AStar.h \
         $(AStSDR)/include/AStarSearch.h \
         $(AStSDR)/include/Distribution.h \
         $(AStSDR)/include/Domain.h \
         $(AStSDR)/include/ExplicitGraph.h \
         $(AStSDR)/include/Grid.h \
         $(AStSDR)/include/HDAStar.h \
         $(AStSDR)/include/Node.h \
         $(AStSDR)/include/NodeTable.h \
         $(AStSDR)/include/OpenList.h \
         $(AStSDR)/include/PASE.h \
         $(AStSDR)/include/SPAStar.h \
         $(AStSDR)/include/SafePBNF.h \
         $(AStSDR)/include/SearchUtils.h \
         $(AStSDR)/include/SequenceScore.h \
         $(AStSDR)/include/SlowDomain.h \
         $(AStSDR)/include/SlidingTile.h \
         $(AStSDR)/include/Solution.h

# the headers of the optional runtimes- - - - - - - - - - - - - - - - - - - -

AStOMPH = $(AStSDR)/include/OpenMPRuntime.h
AStFFH  = $(AStSDR)/include/FastFlowRuntime.h \
          $(AStSDR)/include/FarmAStar.h
AStMPIH = $(AStSDR)/include/HDAStarMPI.h

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(AStOBJ) $(AStSDR)/*~ $(AStSDR)/include/*~ $(AStSDR)/src/*~

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

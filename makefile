# Variables to control Makefile operation

DYNAMICPATH= dynamictuple/
RLPATH= rltuple/
TSS = partitionsort/OVS/
TM = partitionsort/TupleMerge/
PS = partitionsort/PartitionSort/
HYBRIDTSS = partitionsort/HybridTSS/
PSCONVERT = partitionsort/
IOPATH=io/
STRUCTURE=tuplestructure/
Util = partitionsort/Utilities/


VPATH = $(IOPATH) $(DYNAMICPATH) $(STRUCTURE) $(RLPATH) $(TSS) $(TM) $(PS) $(HYBRIDTSS) $(Util) $(PSCONVERT)

CXX = g++
CXXFLAGS = -O3 -std=c++14 -fpermissive

# Targets needed to bring the executable up to date

main: main.o dynamictuple.o rltuple.o dynamictuple-ranges.o \
		dynamictuple-dims.o elementary.o tuple.o hash.o hashtable.o \
  		hashnode.o io.o trie.o cmap.o TupleSpaceSearch.o \
		classification-main-ps.o ElementaryClasses.o ps-io.o Simulation.o\
		SlottedTable.o TupleMergeOnline.o \
		HybridTSS.o SubHybridTSS.o\
		misc.o OptimizedMITree.o PartitionSort.o red_black_tree.o SortableRulesetPartitioner.o stack.o IntervalUtilities.o
	
	$(CXX) $(CXXFLAGS) -o main *.o $(LIBS) -lwsock32

# -------------------------------------------------------------------

main.o: main.cpp hash.h dynamictuple.h
	$(CXX) $(CXXFLAGS) -c main.cpp


rltuple.o : rltuple.cpp rltuple.h elementary.h tuple.h
	$(CXX) $(CXXFLAGS) -c $(RLPATH)rltuple.cpp

dynamictuple.o : dynamictuple.cpp dynamictuple.h elementary.h tuple.h dynamictuple-dims.h dynamictuple-ranges.h 
	$(CXX) $(CXXFLAGS) -c $(DYNAMICPATH)dynamictuple.cpp

dynamictuple-dims.o : dynamictuple-dims.cpp dynamictuple-dims.h 
	$(CXX) $(CXXFLAGS) -c $(DYNAMICPATH)dynamictuple-dims.cpp

dynamictuple-ranges.o : dynamictuple-ranges.cpp dynamictuple-ranges.h 
	$(CXX) $(CXXFLAGS) -c $(DYNAMICPATH)dynamictuple-ranges.cpp


elementary.o : elementary.cpp elementary.h
	$(CXX) $(CXXFLAGS) -c elementary.cpp

tuple.o : tuple.h tuple.cpp elementary.h
	$(CXX) $(CXXFLAGS) -c $(STRUCTURE)tuple.cpp

hash.o : hash.h
	$(CXX) $(CXXFLAGS) -c $(STRUCTURE)hash.cpp

hashtable.o : hashtable.h
	$(CXX) $(CXXFLAGS) -c $(STRUCTURE)hashtable.cpp

hashnode.o : hashnode.h
	$(CXX) $(CXXFLAGS) -c $(STRUCTURE)hashnode.cpp

io.o : io.h
	$(CXX) $(CXXFLAGS) -c $(IOPATH)io.cpp

trie.o : trie.h
	$(CXX) $(CXXFLAGS) -c $(IOPATH)trie.cpp

# TSS
cmap.o : cmap.h pshash.h random.h
	$(CXX) $(CXXFLAGS) -c $(TSS)cmap.cpp

TupleSpaceSearch.o : TupleSpaceSearch.h
	$(CXX) $(CXXFLAGS) -c $(TSS)TupleSpaceSearch.cpp

# TM
SlottedTable.o : SlottedTable.h
	$(CXX) $(CXXFLAGS) -c $(TM)SlottedTable.cpp

TupleMergeOnline.o : TupleMergeOnline.h
	$(CXX) $(CXXFLAGS) -c $(TM)TupleMergeOnline.cpp

# PS
misc.o : misc.h
	$(CXX) $(CXXFLAGS) -c $(PS)misc.cpp

OptimizedMITree.o : OptimizedMITree.h
	$(CXX) $(CXXFLAGS) -c $(PS)OptimizedMITree.cpp

red_black_tree.o : red_black_tree.h
	$(CXX) $(CXXFLAGS) -c $(PS)red_black_tree.cpp

SortableRulesetPartitioner.o : SortableRulesetPartitioner.h
	$(CXX) $(CXXFLAGS) -c $(PS)SortableRulesetPartitioner.cpp

stack.o : stack.h
	$(CXX) $(CXXFLAGS) -c $(PS)stack.cpp

PartitionSort.o : PartitionSort.h
	$(CXX) $(CXXFLAGS) -c $(PS)PartitionSort.cpp

# HYBRIDTSS
SubHybridTSS.o : SubHybridTSS.h
	$(CXX) $(CXXFLAGS) -c $(HYBRIDTSS)SubHybridTSS.cpp

HybridTSS.o : HybridTSS.h
	$(CXX) $(CXXFLAGS) -c $(HYBRIDTSS)HybridTSS.cpp

# Util
IntervalUtilities.o : IntervalUtilities.h
	$(CXX) $(CXXFLAGS) -c $(Util)IntervalUtilities.cpp

# PSCONVERT
ElementaryClasses.o : ElementaryClasses.h
	$(CXX) $(CXXFLAGS) -c $(PSCONVERT)ElementaryClasses.cpp

ps-io.o : ps-io.h
	$(CXX) $(CXXFLAGS) -c $(PSCONVERT)ps-io.cpp

Simulation.o : Simulation.h
	$(CXX) $(CXXFLAGS) -c $(PSCONVERT)Simulation.cpp

classification-main-ps.o : classification-main-ps.h
	$(CXX) $(CXXFLAGS) -c $(PSCONVERT)classification-main-ps.cpp

.PHONY: clean
.PHONY: uninstall

clean:
	rm -f *.o

uninstall: clean
	rm main

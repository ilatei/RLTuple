#ifndef  CLASSIFICATIONMAINPS_H
#define  CLASSIFICATIONMAINPS_H

#include "../elementary.h"
#include "../io/io.h"
#include "ps-io.h"
#include "OVS/TupleSpaceSearch.h"
#include "PartitionSort/PartitionSort.h"
#include "TupleMerge/TupleMergeOnline.h"
#include "HybridTSS/HybridTSS.h"

using namespace std;

int ClassificationMainPS(CommandStruct command, ProgramState *program_state, vector<Rule*> &rules, 
                          vector<Trace*> &traces, map<double,int> trace_interval, vector<int> &ans, int trials);

#endif
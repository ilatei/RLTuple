#include "classification-main-ps.h"

using namespace std;


// TupleInfo main_tuple_info;
// extern int rangecuts_split_rules_num;

int ClassificationMainPS(CommandStruct command, ProgramState *program_state, vector<Rule*> &rules, 
                          vector<Trace*> &traces, map<double,int> trace_interval, vector<int> &ans, int trials) {
    vector<PSRule> ps_rules = GeneratePSRules(rules);
    // PrintByteCutsRules(bc_rules);
    vector<PSPacket> packets = GeneratePSPackets(traces);

    // PerformPartitionSort();
    if (command.method_name == "PSTSS") {
        PriorityTupleSpaceSearch ptss;
        PerformOnlyPacketClassification(ptss, ps_rules, packets, trace_interval, trials, program_state);
    } else if (command.method_name == "PartitionSort") {
        PartitionSort partitionsort;
        PerformOnlyPacketClassification(partitionsort, ps_rules, packets, trace_interval, trials, program_state);
    } else if (command.method_name == "TupleMerge") {
        unordered_map<std::string, std::string> args;
        TupleMergeOnline tuplemerge(args);
        PerformOnlyPacketClassification(tuplemerge, ps_rules, packets, trace_interval, trials, program_state);
    }
    return 0;
}
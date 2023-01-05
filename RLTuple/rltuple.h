#ifndef  RLTUPLE_H
#define  RLTUPLE_H

#include "../elementary.h"
#include "../tuplestructure/tuple.h"
#include <unordered_map>

using namespace std;

class RlTuple : public Classifier {
public:
    
    int Create(vector<Rule*> &rules, bool insert);
    int InsertRule(Rule *rule);
    int InsertRuleTuple(Rule *rule, Tuple* tuple);
    int DeleteRule(Rule *rule);
    double evaluate();
    unordered_map<double, int> evaluateWithWeight(unordered_map<int,int>& weight, string F = "throughput");
    vector<int> evaluateByaccess();
    int Lookup(Trace *trace, int priority);
    vector<int> LookupACcnt(Trace *trace, int priority);
    int LookupAccess(Trace *trace, int priority, Rule *ans_rule, ProgramState *program_state);
    void step(vector<Rule*> &rules);
    void addTuple(vector<uint32_t> _used_field, vector<uint32_t> _used_field_lenth);
    vector<int> getObs(int N, pair<vector<uint32_t>, vector<uint32_t>> current);

    int TupleNum();
    int Reconstruct();
    uint64_t MemorySize();
    int CalculateState(ProgramState *program_state);
    int GetRules(vector<Rule*> &rules);
    int Free(bool free_self);
    int Test(void *ptr);

    string method_name;
	int threshold;  // port_hashtable

    bool use_port_hash_table;

    Tuple **tuples_arr;
	map<uint32_t, Tuple*> tuples_map;
    unordered_map<uint32_t, Tuple*> id2tuple;
    unordered_map<Rule*, Tuple*> rule2tuple;
    int tuples_num;
    int max_tuples_num;
    int rules_num;
    int max_priority;

    double dt_time;

    void InsertTuple(Tuple *tuple);
    void SortTuples();
};

#endif
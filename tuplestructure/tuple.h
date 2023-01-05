#ifndef  Tuple_H
#define  Tuple_H

#include "../elementary.h"
#include "hash.h"
#include "hashtable.h"

using namespace std;

class Tuple {
public:

    int max_priority;
    int rules_num;
    vector<uint32_t> used_field;
    vector<uint32_t> prefix_len;
    vector<uint32_t> prefix_len_zero;
    vector<uint32_t> prefix_mask;

    HashTable hash_table;
    bool use_port_hash_table;

    Tuple(vector<uint32_t> fields, vector<uint32_t> used_len, bool _use_port_hash_table);
    int InsertRule(Rule *rule);
    int InsertRuleRL(Rule *rule);
    int InsertRuleRLWithCollide(Rule *rule, int collide);
    int DeleteRule(Rule *rule);
    int DeleteRuleRL(Rule *rule);
    uint64_t MemorySize();
    int CalculateState(ProgramState *program_state);
    int GetRules(vector<Rule*> &rules);
    int Free(bool free_self);
    int Test(void *ptr);
};

#endif
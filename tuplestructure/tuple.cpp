#include "tuple.h"

using namespace std;

Tuple::Tuple(vector<uint32_t> fields, vector<uint32_t> used_len, bool _use_port_hash_table) {
    used_field = fields;
    prefix_len = used_len;
    
    for (int i = 0; i < used_field.size(); i++){
        prefix_len_zero.push_back(RULE_FILED_MAXLENTH[used_field[i]] - used_len[i]);
    }
    
    for (int i = 0; i < used_field.size(); ++i)
        prefix_mask.push_back(~((1ULL << (RULE_FILED_MAXLENTH[used_field[i]] - prefix_len[i])) - 1));
    max_priority = 0;
    rules_num = 0;
    use_port_hash_table = _use_port_hash_table;
    hash_table.Init(32, false, use_port_hash_table);
}

//hash
int Tuple::InsertRule(Rule *rule) {

    uint32_t src_ip = ((uint64_t)rule->range[0][0]) >> prefix_len_zero[0];
    uint32_t dst_ip = ((uint64_t)rule->range[1][0]) >> prefix_len_zero[1];
    uint64_t key = ((uint64_t)src_ip << 32) | dst_ip;
    uint32_t hash = Hash32_2(src_ip, dst_ip);

    if (hash_table.InsertRule(rule, key, hash) > 0)
        return 1;
    ++rules_num;
    max_priority = hash_table.max_priority;
    return 0;
}

int Tuple::InsertRuleRL(Rule *rule) {
    uint64_t key = rule->range[used_field[0]][0] >> prefix_len_zero[0];
    uint32_t hash = key;
    for (int i = 1; i < used_field.size(); i++){
        auto val = rule->range[used_field[i]][0] >>prefix_len_zero[i];
        hash = Hash32_2(hash, val);
        key = (key << prefix_len[i]) | val;
    }

    // printf("%d %u %llu\n", rule->priority, hash, key);

    if (hash_table.InsertRule(rule, key, hash) > 0)
        return 1;
    ++rules_num;
    max_priority = hash_table.max_priority;
    return 0;
}

int Tuple::InsertRuleRLWithCollide(Rule *rule, int collide) {
    uint64_t key = rule->range[used_field[0]][0] >> prefix_len_zero[0];
    uint32_t hash = key;
    for (int i = 1; i < used_field.size(); i++){
        auto val = rule->range[used_field[i]][0] >>prefix_len_zero[i];
        hash = Hash32_2(hash, val);
        key = (key << prefix_len[i]) | val;
    }

    // printf("%d %u %llu\n", rule->priority, hash, key);

    if (hash_table.InsertRuleCollide(rule, key, hash, collide) == -1)
        return -1;
    ++rules_num;
    max_priority = hash_table.max_priority;
    return 0;
}

int Tuple::DeleteRule(Rule *rule) {
    uint32_t src_ip = ((uint64_t)rule->range[0][0]) >> prefix_len_zero[0];
    uint32_t dst_ip = ((uint64_t)rule->range[1][0]) >> prefix_len_zero[1];
    uint64_t key = ((uint64_t)src_ip << 32) | dst_ip;
    uint32_t hash = Hash32_2(src_ip, dst_ip);
    
    if (hash_table.DeleteRule(rule, key, hash) > 0)
        return 1;
    --rules_num;
    max_priority = hash_table.max_priority;
    return 0;
}

int Tuple::DeleteRuleRL(Rule *rule) {
    uint64_t key = rule->range[used_field[0]][0] >> prefix_len_zero[0];
    uint32_t hash = key;
    for (int i = 1; i < used_field.size(); i++){
        auto val = rule->range[used_field[i]][0] >>prefix_len_zero[i];
        hash = Hash32_2(hash, val);
        key = (key << prefix_len[i]) | val;
    }
    
    if (hash_table.DeleteRule(rule, key, hash) > 0)
        return 1;
    --rules_num;
    max_priority = hash_table.max_priority;
    return 0;
}

uint64_t Tuple::MemorySize() {
    uint64_t memory_size = sizeof(Tuple);
    memory_size += hash_table.MemorySize() - sizeof(HashTable);
    return memory_size;
}

int Tuple::GetRules(vector<Rule*> &rules) {
    hash_table.GetRules(rules);
    return 0;
}

int Tuple::Free(bool free_self) {
    hash_table.Free(false);
    if (free_self)
        free(this);
    return 0;
}

int Tuple::CalculateState(ProgramState *program_state) {
    //printf("%d %d : rules_num = %d max_priority = %d\n", prefix_len[0], prefix_len[1], rules_num, max_priority);
    hash_table.CalculateState(program_state);
    return 0;
}

int Tuple::Test(void *ptr) {
    return 0;
}
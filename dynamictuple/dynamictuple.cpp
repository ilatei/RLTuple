#include "dynamictuple.h"
#include <chrono>


using namespace std;

int DynamicTuple::Create(vector<Rule*> &rules, bool insert) {
    pthread_mutex_init(&lookup_mutex, NULL);
    pthread_mutex_init(&update_mutex, NULL);
    // tuple_ranges
    tuple_ranges.clear();
    dt_time = 0;
    std::chrono::duration<double,std::milli> elapsed_milliseconds;
    auto start = std::chrono::_V2::steady_clock::now();

    tuple_ranges = DynamicTupleRanges(rules, dt_time, tuple_ranges);
    int tuple_ranges_num = tuple_ranges.size();
    // PrintTupleRanges(tuple_ranges);
    for (int i = 0; i < tuple_ranges_num; ++i)
        for (int x = tuple_ranges[i].x1; x <= tuple_ranges[i].x2; ++x)
            for (int y = tuple_ranges[i].y1; y <= tuple_ranges[i].y2; ++y) {
                prefix_down[x][y][0] = tuple_ranges[i].x1;
                prefix_down[x][y][1] = tuple_ranges[i].y1;
            }

    for (int i = 1; i <= 32; ++i)
        prefix_mask[i] = prefix_mask[i - 1] | (1U << (32 - i));
    
    auto end = std::chrono::_V2::steady_clock::now();
    elapsed_milliseconds = end - start;
    // printf("dynamic tuple initial time %f\n", elapsed_milliseconds);

    tuples_num = 0;
    max_tuples_num = 16;
    tuples_arr = (Tuple**)malloc(sizeof(Tuple*) * max_tuples_num);
    for (int i = 0; i < max_tuples_num; ++i)
        tuples_arr[i] = NULL;
    tuples_map.clear();

    if (insert) {
        int rules_num = rules.size();
        for (int i = 0; i < rules_num; ++i)
            InsertRule(rules[i]);
    }
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        printf("%d %d:%d rules\n", tuple->prefix_len[0], tuple->prefix_len[1], tuple->rules_num);
    }
    return 0;
}

void DynamicTuple::InsertTuple(Tuple *tuple) {
    if (tuples_num == max_tuples_num) {
		Tuple **new_tuples_arr = (Tuple**)malloc(sizeof(Tuple*) * max_tuples_num * 2);
		for (int i = 0; i < max_tuples_num; ++i)
			new_tuples_arr[i] = tuples_arr[i];
        for (int i = max_tuples_num; i < max_tuples_num * 2; ++i)
            new_tuples_arr[i] = NULL;
        max_tuples_num *= 2;
        free(tuples_arr);
        tuples_arr = new_tuples_arr;
    }
	tuples_arr[tuples_num++] = tuple;
}

void DynamicTuple::SortTuples() {
    sort(tuples_arr, tuples_arr + tuples_num, [](Tuple *tuple1, Tuple *tuple2){return tuple1->max_priority > tuple2->max_priority;});
}

int DynamicTuple::InsertRule(Rule *rule) {
    uint32_t prefix_src = prefix_down[rule->prefix_len[0]][rule->prefix_len[1]][0];
    uint32_t prefix_dst = prefix_down[rule->prefix_len[0]][rule->prefix_len[1]][1];
    uint32_t prefix_pair = (uint32_t)prefix_src  << 6 | prefix_dst;
    map<uint32_t, Tuple*>::iterator iter = tuples_map.find(prefix_pair);
    Tuple *tuple = NULL;
    if (iter != tuples_map.end()) {
        tuple = iter->second;
    } else {
        vector<uint32_t> used_field = {0, 1};
        vector<uint32_t> used_field_lenth = {prefix_src, prefix_dst};
        tuple = new Tuple(used_field, used_field_lenth, use_port_hash_table);
        InsertTuple(tuple);
        tuples_map[prefix_pair] = tuple;
    }
    if (tuple->InsertRule(rule) > 0)
        return 1;
    
    ++rules_num;
    if (rule->priority == tuple->max_priority)
        SortTuples();
    max_priority = max(max_priority, rule->priority);
    return 0;
}

Tuple* DynamicTuple::getTupleFromPrefix(int src_pre, int dst_pre){
    uint32_t prefix_src = prefix_down[src_pre][dst_pre][0];
    uint32_t prefix_dst = prefix_down[src_pre][dst_pre][1];
    uint32_t prefix_pair = (uint32_t)prefix_src  << 6 | prefix_dst;
    map<uint32_t, Tuple*>::iterator iter = tuples_map.find(prefix_pair);
    Tuple *tuple = NULL;
    if (iter != tuples_map.end()) {
        return iter->second;
    } else {
        return NULL;
    }
}


int Dis_tr(Tuple* tuple, Rule* rule){
    int tuple_x = tuple->prefix_len[0];
    int tuple_y = tuple->prefix_len[1];
    int rule_x = rule->prefix_len[0];
    int rule_y = rule->prefix_len[1];
    if(tuple_x > rule_x || tuple_y > rule_y){
        return 9999;
    }
    else{
        return (rule_x - tuple_x) + (rule_y - tuple_y);
    }
}


void DynamicTuple::addTuple(vector<int> tuple_xy){
    for (int i = 0; i < tuples_num; ++i) {
        if(tuple_xy[0] == tuples_arr[i]->prefix_len[0] && tuple_xy[1] == tuples_arr[i]->prefix_len[1]){
            printf("already ex\n");
            return;
        }
    }
    vector<uint32_t> used_field = {0, 1};
    vector<uint32_t> used_field_lenth = {uint32_t(tuple_xy[0]), uint32_t(tuple_xy[1])};
    auto tuple = new Tuple(used_field, used_field_lenth, use_port_hash_table);
    InsertTuple(tuple);
}

void DynamicTuple::step(vector<Rule*> &rules){
    // map<uint32_t, Tuple*>::iterator iter = tuples_map.find(prefix_pair);
    Tuple *tuple = NULL;

    tuples_map.clear();
    for (auto rule : rules){
        int distance = 9999;
        uint32_t prefix_pair = (uint32_t)rule->prefix_len[0]  << 6 | rule->prefix_len[1];
        map<uint32_t, Tuple*>::iterator iter = tuples_map.find(prefix_pair);
        if (iter != tuples_map.end()) {
            tuple = iter->second;
        } else {
            for (int i = 0; i < tuples_num; ++i) {
                Tuple *tuple1 = tuples_arr[i];
                int distance1 = Dis_tr(tuple1, rule);
                if (distance1 < distance){
                    distance = distance1;
                    tuple = tuple1;
                }
            }
            if(tuple == NULL){
                printf("err no tuple available!\n");
            }
            tuples_map[prefix_pair] = tuple;

        }
        tuple->InsertRule(rule);
        ++rules_num;
        if (rule->priority == tuple->max_priority)
            SortTuples();
        max_priority = max(max_priority, rule->priority);
    }

    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple1 = tuples_arr[i];
        printf("%d,%d,%d\n", tuple1->prefix_len[0], tuple1->prefix_len[1], tuple1->rules_num);
    }
}

int DynamicTuple::InsertRuleTuple(Rule *rule, Tuple* tuple) {
    if (tuple == NULL){
        return 1;
    }
    if (tuple->InsertRule(rule) > 0)
        return 1;
    ++rules_num;
    if (rule->priority == tuple->max_priority)
        SortTuples();
    max_priority = max(max_priority, rule->priority);
    return 0;
}



int DynamicTuple::DeleteRule(Rule *rule) {
    uint32_t prefix_src = prefix_down[rule->prefix_len[0]][rule->prefix_len[1]][0];
    uint32_t prefix_dst = prefix_down[rule->prefix_len[0]][rule->prefix_len[1]][1];
    uint32_t prefix_pair = (uint32_t)prefix_src << 6 | prefix_dst;
    map<uint32_t, Tuple*>::iterator iter = tuples_map.find(prefix_pair);
    Tuple *tuple = NULL;
	if(iter != tuples_map.end())
		tuple = iter->second;
	else
		return 1;

    if (tuple->DeleteRule(rule) > 0)
        return 1;
    
    --rules_num;
    if (tuple->rules_num == 0) {
        //printf("delete tuple\n");
		tuple->max_priority = 0;
        SortTuples();
        tuples_arr[--tuples_num] = NULL;
        tuples_map.erase(prefix_pair);
        tuple->Free(true);
        return 0;
    }
    if (rule->priority > tuple->max_priority)
        SortTuples();
    if (rule->priority == max_priority) {
        if (tuples_num == 0)
            max_priority = 0;
        else
            max_priority = tuples_arr[0]->max_priority;
    }
    return 0;
}

int DynamicTuple::DeleteRuleTuple(Rule *rule, Tuple* tuple) {
    if (tuple == NULL)
        return 1;
    if (tuple->DeleteRule(rule) > 0)
        return 1;
    
    --rules_num;
    if (tuple->rules_num == 0) {
        //printf("delete tuple\n");
		tuple->max_priority = 0;
        SortTuples();
        tuples_arr[--tuples_num] = NULL;
        tuple->Free(true);
        return 2;
    }
    if (rule->priority > tuple->max_priority)
        SortTuples();
    if (rule->priority == max_priority) {
        if (tuples_num == 0)
            max_priority = 0;
        else
            max_priority = tuples_arr[0]->max_priority;
    }
    return 0;
}

vector<int> DynamicTuple::evaluateByaccess(){
    int N_tuple = 0;
    int N_group_inner = 0;
    double N_group_outer = 0;
    int N_rule_inner = 0;
    
    vector<double> T_use;
    // for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
    //     Tuple *tuple = tuples_arr[tuple_index];
    //     T_use.push_back((double)tuple->hash_table.hash_node_num / (tuple->hash_table.mask + 1));
    // }

    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        int cnt = 0;
        Tuple *tuple = tuples_arr[tuple_index];
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;
        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            auto group_node = hash_node_arr[group_index];
            if(group_node != NULL){
                cnt ++;
            }
            while(group_node != NULL){
                group_node = group_node->next;
            }
        }
        T_use.push_back((double)cnt/ (tuple->hash_table.mask + 1));
    }

    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        N_tuple += tuple->max_priority;
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;
        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            int level = 1;
            auto group_node = hash_node_arr[group_index];
            while(group_node != NULL){
                N_group_inner += group_node->rules_num * level;
                N_rule_inner += group_node->rules_num * (group_node->rules_num + 1) / 2;
                ++ level;
                auto rule_node = group_node->rule_node;
                while(rule_node != NULL){
                    for (int tuple_index1 = 0; tuple_index1 < tuples_num; ++tuple_index1) {
                        if(rule_node->priority < tuples_arr[tuple_index1]->max_priority && tuple_index1 != tuple_index){
                            N_group_outer +=  T_use[tuple_index1];
                        }
                    }
                    rule_node = rule_node->next;
                }
                group_node = group_node->next;
            }
        }
    }
    // printf("%d %lf %d\n", N_tuple, N_group, N_rule);
    return std::vector<int>{N_tuple, N_group_inner, int(N_group_outer), N_rule_inner};
}


double DynamicTuple::evaluate(){
    uint64_t check_hash_cost = 5;
    uint64_t check_group_cost = 2;
    uint64_t check_rule_cost = 3;
    uint64_t N_tuple = 0, N_rule = 0;
    double N_group = 0;
    vector<double> T_use;
    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        T_use.push_back((double)tuple->hash_table.hash_node_num / (tuple->hash_table.mask + 1));
        // printf("%lf ", T_use[tuple_index]);
    }
    // printf("\n");

    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        N_tuple += tuple->max_priority;
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;
        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            int level = 1;
            auto group_node = hash_node_arr[group_index];
            while(group_node != NULL){
                N_group += group_node->rules_num * level;
                N_rule += group_node->rules_num * (group_node->rules_num + 1) / 2;
                ++ level;
                auto rule_node = group_node->rule_node;
                while(rule_node != NULL){
                    for (int tuple_index1 = 0; tuple_index1 < tuples_num; ++tuple_index1) {
                        if(rule_node->priority < tuples_arr[tuple_index1]->max_priority  && tuple_index1 != tuple_index){
                            N_group +=  T_use[tuple_index1];
                        }
                    }
                    rule_node = rule_node->next;
                }
                group_node = group_node->next;
            }
        }
    }
    // printf("%d %lf %d\n", N_tuple, N_group, N_rule);
    return check_hash_cost * N_tuple + N_group * check_group_cost + N_rule * check_rule_cost;
}


int DynamicTuple::Lookup(Trace *trace, int priority) {
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;
        uint32_t hash1 = ((uint64_t)trace->key[0]) >> tuple->prefix_len_zero[0];
        uint32_t hash2 = ((uint64_t)trace->key[1]) >> tuple->prefix_len_zero[1];
        uint64_t key = (uint64_t)hash1 << 32 | hash2;

        hash1 ^= hash1 >> 16; hash1 *= 0x85ebca6b; hash1 ^= hash1 >> 13; hash1 *= 0xc2b2ae35;
        hash2 ^= hash2 >> 16; hash2 *= 0x85ebca6b; hash2 ^= hash2 >> 13; hash2 *= 0xc2b2ae35;
        hash1 ^= hash2; hash1 ^= hash1 >> 16;

        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash1 & tuple->hash_table.mask];
        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;
            if (hash_node->key == key) {
                if (hash_node->has_port_hash_table) {
                    for (int i = 0; i < 2; ++i) {
                        HashTable *port_hash_table = hash_node->port_hash_table[i];
                        if (port_hash_table == NULL || priority >= port_hash_table->max_priority)
                            continue;
                        uint64_t key2 = trace->key[i + 2];
                        uint32_t hash3 = key2;
                        hash3 *= 0x85ebca6b; hash3 ^= hash3 >> 16; hash3 *= 0xc2b2ae35; hash3 ^= hash3 >> 16;
                        HashNode *hash_node2 = port_hash_table->hash_node_arr[hash3 & port_hash_table->mask];
                        while (hash_node2) {
                            if (priority >= hash_node2->max_priority)
                                break;
                            if (hash_node2->key == key2) {
                                RuleNode *rule_node = hash_node2->rule_node;
                                while (rule_node) {
                                    if (priority >= rule_node->priority)
                                        break;
                                    if (MatchRuleTrace(rule_node->rule, trace)) {
                                        priority = rule_node->priority;
                                         break;
                                    }
                                    rule_node = rule_node->next;  
                                }
                                break;
                            }
                            hash_node2 = hash_node2->next;
                        }
                    }
                }
                RuleNode *rule_node = hash_node->rule_node;
                while (rule_node) {
                    if (priority >= rule_node->priority)
                        break;
                    if (MatchRuleTrace(rule_node->rule, trace)) {
                        priority = rule_node->priority;
                        break;
                    }
                    rule_node = rule_node->next;  
                };
                break;
            }
            hash_node = hash_node->next;
        }
    }
    return priority;
}

int DynamicTuple::LookupAccess(Trace *trace, int priority, Rule *ans_rule, ProgramState *program_state) {
    program_state->AccessClear();
    
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;
        program_state->access_tuples.AddNum();
        uint32_t hash1 = ((uint64_t)trace->key[0]) >> tuple->prefix_len_zero[0];
        uint32_t hash2 = ((uint64_t)trace->key[1]) >> tuple->prefix_len_zero[1];
        uint64_t key = (uint64_t)hash1 << 32 | hash2;

        hash1 ^= hash1 >> 16; hash1 *= 0x85ebca6b; hash1 ^= hash1 >> 13; hash1 *= 0xc2b2ae35;
        hash2 ^= hash2 >> 16; hash2 *= 0x85ebca6b; hash2 ^= hash2 >> 13; hash2 *= 0xc2b2ae35;
        hash1 ^= hash2; hash1 ^= hash1 >> 16;
        program_state->access_tables.AddNum();

        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash1 & tuple->hash_table.mask];
        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;
            program_state->access_nodes.AddNum();
            // if (ans_rule) {
            //     if (hash_node->key == key) {
            //         if (ans_rule->priority <= hash_node->max_priority)
            //             ++program_state->low_priority_matching_access;
            //         else 
            //             ++program_state->high_priority_matching_access;
            //     } else {
            //         if (ans_rule->priority <= hash_node->max_priority)
            //             ++program_state->low_priority_collision_access;
            //         else 
            //             ++program_state->high_priority_collision_access;
            //     }
            // }
            if (hash_node->key == key) {
                if (hash_node->has_port_hash_table) {
                    for (int i = 0; i < 2; ++i) {
                        HashTable *port_hash_table = hash_node->port_hash_table[i];
                        if (port_hash_table == NULL || priority >= port_hash_table->max_priority)
                            continue;
                        program_state->access_tables.AddNum();
                        uint64_t key2 = trace->key[i + 2];
                        uint32_t hash3 = key2;
                        hash3 *= 0x85ebca6b; hash3 ^= hash3 >> 16; hash3 *= 0xc2b2ae35; hash3 ^= hash3 >> 16;
                        HashNode *hash_node2 = port_hash_table->hash_node_arr[hash3 & port_hash_table->mask];
                        while (hash_node2) {
                            if (priority >= hash_node2->max_priority)
                                break;
                            program_state->access_nodes.AddNum();
                            if (hash_node2->key == key2) {
                                RuleNode *rule_node = hash_node2->rule_node;
                                while (rule_node) {
                                    if (priority >= rule_node->rule->priority)
                                        break;
                                    program_state->access_rules.AddNum();
                                    if (MatchRuleTrace(rule_node->rule, trace)) {
                                        priority = rule_node->priority;
                                        break;
                                    }
                                    rule_node = rule_node->next;  
                                };
                                break;
                            }
                            hash_node2 = hash_node2->next;
                        }
                    }
                }
                RuleNode *rule_node = hash_node->rule_node;
                while (rule_node) {
                    if (priority >= rule_node->rule->priority)
                        break;
                    program_state->access_rules.AddNum();
                    // if (ans_rule) {
                    //     if (ans_rule->priority <= rule_node->rule->priority)
                    //         ++program_state->low_priority_rule_access;
                    //     else
                    //         ++program_state->high_priority_rule_access;
                    // }
                    if (MatchRuleTrace(rule_node->rule, trace)) {
                        priority = rule_node->priority;
                        break;
                    }
                    rule_node = rule_node->next;  
                };
                break;
            }
            hash_node = hash_node->next;
        }
    }
    program_state->AccessCal();
	
    return priority;
}

int DynamicTuple::TupleNum(){
    return tuples_num;
}


int DynamicTuple::Reconstruct() {
    if (method_name != "DynamicTuple_Basic" && method_name != "DynamicTuple")
        return 0;
    pthread_mutex_lock(&update_mutex);
    vector<Rule*> rules;
    GetRules(rules);

    vector<TupleRange> old_tuple_ranges;
    old_tuple_ranges = tuple_ranges;
    int old_tuple_ranges_num = old_tuple_ranges.size();

    tuple_ranges = DynamicTupleRanges(rules, dt_time, old_tuple_ranges);
    // PrintTupleRanges(tuple_ranges);

    int tuple_ranges_num = tuple_ranges.size();
    for (int i = 0; i < tuple_ranges_num; ++i)
        for (int x = tuple_ranges[i].x1; x <= tuple_ranges[i].x2; ++x)
            for (int y = tuple_ranges[i].y1; y <= tuple_ranges[i].y2; ++y) {
                prefix_down[x][y][0] = tuple_ranges[i].x1;
                prefix_down[x][y][1] = tuple_ranges[i].y1;
            }

    map<uint32_t, int> tuple_ranges_map;
    map<uint32_t, int> tuple_reconstruct_map;
    for (int i = 0; i < tuple_ranges_num; ++i) {
        uint32_t range = tuple_ranges[i].x1 << 18 | tuple_ranges[i].y1 << 12 |
                         tuple_ranges[i].x2 << 6 | tuple_ranges[i].y2;
        tuple_ranges_map[range] = 1;
    }
    for (int i = 0; i < old_tuple_ranges_num; ++i) {
        uint32_t range = old_tuple_ranges[i].x1 << 18 | old_tuple_ranges[i].y1 << 12 |
                         old_tuple_ranges[i].x2 << 6 | old_tuple_ranges[i].y2;
        if (tuple_ranges_map.find(range) == tuple_ranges_map.end()) {
            uint32_t prefix_pair = old_tuple_ranges[i].x1 << 6 | old_tuple_ranges[i].y1;
            // printf("tuple_reconstruct %d %d\n", old_tuple_ranges[i].x1, old_tuple_ranges[i].y1);
            tuple_reconstruct_map[prefix_pair] = 1;
        }
    }


    pthread_mutex_lock(&lookup_mutex);

    int reconstruct_tuples_num = 0;;
    rules.clear();
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        uint32_t prefix_src = tuple->prefix_len[0];
        uint32_t prefix_dst = tuple->prefix_len[1];
        uint32_t prefix_pair = (uint32_t)prefix_src << 6 | prefix_dst;

        if (tuple_reconstruct_map.find(prefix_pair) != tuple_reconstruct_map.end()) {
            // printf("tuple_reconstruct %d %d\n", prefix_src, prefix_dst);
            tuple->GetRules(rules);
            tuple->Free(true);
            tuples_map.erase(prefix_pair);

            tuples_arr[i] = tuples_arr[tuples_num - 1];
            tuples_arr[tuples_num - 1] = NULL;
            --tuples_num;
            --i;
            ++reconstruct_tuples_num;
        }
    }
    SortTuples();
    // printf("reconstruct_tuples_num %d rules_num %ld\n", reconstruct_tuples_num, rules.size());
    int rules_num = rules.size();
    for (int i =  0; i < rules_num; ++i)
        InsertRule(rules[i]);

    pthread_mutex_unlock(&lookup_mutex);
    pthread_mutex_unlock(&update_mutex);

    // exit(1);
    return 0;
}

uint64_t DynamicTuple::MemorySize() {
    uint64_t memory_size = sizeof(DynamicTuple);
    memory_size += sizeof(Tuple*) * max_tuples_num;
    memory_size += 64 * tuples_num; // map
    for (int i = 0; i < tuples_num; ++i) {
        memory_size += tuples_arr[i]->MemorySize();
        //printf("%d\n", tuples_arr[i]->MemorySize());
        //return memory_size;
    }
    return memory_size;
}

int DynamicTuple::GetRules(vector<Rule*> &rules) {
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->GetRules(rules);
    return 0;
}

int DynamicTuple::Free(bool free_self) {
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->Free(true);
    free(tuples_arr);
    tuples_map.clear();
    if (free_self)
        free(this);
    return 0;
}

int DynamicTuple::CalculateState(ProgramState *program_state) {
    program_state->dt_time += dt_time;
    program_state->tuples_num = tuples_num;
    program_state->tuples_sum += tuples_num;

    //printf("tuples_num %d\n", tuples_num);
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->CalculateState(program_state);
    return 0;
}

int DynamicTuple::Test(void *ptr) {
    return 0;
}

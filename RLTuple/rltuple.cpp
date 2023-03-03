#include "rltuple.h"
#include <chrono>


using namespace std;

pair<int,int> getListInfo(Tuple *tuple){
    if(tuple->rules_num == 0){
        return make_pair(0,0);
    }
    int max_length = 0;
    auto hash_table = tuple->hash_table;
    auto hash_node_arr = hash_table.hash_node_arr;
    for(int group_index=0; group_index<= hash_table.mask; ++group_index){
        auto group_node = hash_node_arr[group_index];
        if(group_node != NULL)
            max_length = (max_length >= group_node->rules_num) ? max_length : group_node->rules_num;
    }
    return make_pair(int(tuple->rules_num / hash_table.hash_node_num), max_length);
}

int RlTuple::Create(vector<Rule*> &rules, bool insert) {

    pthread_mutex_init(&lookup_mutex, NULL);
    pthread_mutex_init(&update_mutex, NULL);

    dt_time = 0;
    std::chrono::duration<double,std::milli> elapsed_milliseconds;
    auto start = std::chrono::_V2::steady_clock::now();

    
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
    return 0;
}

void RlTuple::InsertTuple(Tuple *tuple) {
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

void RlTuple::SortTuples() {
    sort(tuples_arr, tuples_arr + tuples_num, [](Tuple *tuple1, Tuple *tuple2){return tuple1->max_priority > tuple2->max_priority;});
}

// int RlTuple::InsertRule(Rule *rule) {
//     Tuple *tuple2insert = NULL;

//     if(id2tuple.find(rule->priority) != id2tuple.end()){
//         tuple2insert = id2tuple[rule->priority];
//         tuple2insert->InsertRuleRL(rule);
//     }
//     else{
//         int collide_thresh = 10;
//         for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
//             Tuple *tuple = tuples_arr[tuple_index];
        
//             int field_index = 0;
//             for(field_index = 0; field_index < tuple->prefix_len.size();){
//                 int usedF = tuple->used_field[field_index];
//                 if(rule->prefix_len[usedF] < tuple->prefix_len[field_index]){
//                     break;
//                 }
//                 field_index ++;
//             }
//             if(field_index != tuple->prefix_len.size()){
//                 continue;
//             }
//             auto flag = tuple->InsertRuleRLWithCollide(rule, collide_thresh);
//             // tuple2insert = tuple;
//             // tuple2insert->InsertRuleRL(rule);
//             if(flag == 0){
//                 tuple2insert = tuple;
//                 break;
//             }
//         }
//     }

//     //unlikely
//     if(tuple2insert == NULL){
//         for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
//             Tuple *tuple = tuples_arr[tuple_index];
//             if(tuple->prefix_len[0] == 0){
//                 tuple2insert = tuple;
//                 break;
//             }
//         }
//         if(tuple2insert == NULL){
//             vector<uint32_t> used_field = {0};
//             vector<uint32_t> used_field_lenth = {0};
//             tuple2insert = new Tuple(used_field, used_field_lenth, use_port_hash_table);
//             InsertTuple(tuple2insert);
//             // tuples_map[prefix_pair] = tuple;
//         }
//         tuple2insert->InsertRuleRL(rule);
//     }


//     ++rules_num;
//     if (rule->priority == tuple2insert->max_priority)
//         SortTuples();
//     max_priority = max(max_priority, rule->priority);
//     id2tuple[rule->priority] = tuple2insert;
//     // printf("rule %d insert to %d\n", rule->priority, tuple2insert->rules_num);

//     // auto hash_table = tuple2insert->hash_table;
//     // auto hash_node_arr = hash_table.hash_node_arr;
//     // for(int group_index=0; group_index<= hash_table.mask; ++group_index){
//     //     auto group_node = hash_node_arr[group_index];
//     //     while(group_node != NULL){
//     //         printf("%d:", group_index);
//     //         auto rule_node = group_node->rule_node;
//     //         while(rule_node != NULL){
//     //             printf("%d,", rule_node->priority);
//     //             rule_node = rule_node->next;
//     //         }
//     //         group_node = group_node->next;
//     //         printf("\n");
//     //     }
//     // }

//     return 0;
// }


int RlTuple::InsertRule(Rule *rule) {
    vector <pair<int, Tuple*>> tuples2insert;
    Tuple *tuple2insert = NULL;
    Tuple *default_tuple = NULL;
    if(id2tuple.find(rule->priority) != id2tuple.end()){
        tuple2insert = id2tuple[rule->priority];
        tuple2insert->InsertRuleRL(rule);
        ++rules_num;
        if (rule->priority == tuple2insert->max_priority)
            SortTuples();
        max_priority = max(max_priority, rule->priority);
    
        return 0;
    }
    else{
        int collide_thresh = 15;
        for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
            int available_bits = 0;
            Tuple *tuple = tuples_arr[tuple_index];
        
            int field_index = 0;
            for(; field_index < tuple->prefix_len.size();){
                int usedF = tuple->used_field[field_index];
                if(rule->prefix_len[usedF] < tuple->prefix_len[field_index]){
                    break;
                }

                available_bits += rule->prefix_len[usedF] - tuple->prefix_len[field_index];
                // if(tuple->prefix_len[field_index] == 48){
                //     available_bits += 24;
                // }
                // else{
                //     available_bits += tuple->prefix_len[field_index];
                // }
                field_index ++;
            }
            if(field_index == tuple->prefix_len.size()){
                tuples2insert.push_back(make_pair(available_bits / tuple->prefix_len.size(), tuple));
                if(tuple->prefix_len[0] == 0){
                    default_tuple = tuple;
                }
            }
        }
        sort(tuples2insert.begin(), tuples2insert.end(), 
                    [](pair<int, Tuple*> tuple1, pair<int, Tuple*> tuple2){return tuple1.first < tuple2.first;});
        for(auto t : tuples2insert){
            auto flag = t.second->InsertRuleRLWithCollide(rule, collide_thresh);
            if(flag == 0){
                tuple2insert = t.second;
                break;
            }
        }
    }

    //unlikely
    if(tuple2insert == NULL){
        if(default_tuple == NULL){
            vector<uint32_t> used_field = {0};
            vector<uint32_t> used_field_lenth = {0};
            default_tuple = new Tuple(used_field, used_field_lenth, use_port_hash_table);
            InsertTuple(default_tuple);
        }
        if (default_tuple->InsertRuleRL(rule) > 0)
            return 1;
        tuple2insert = default_tuple;
    }

    ++rules_num;
    if (rule->priority == tuple2insert->max_priority)
        SortTuples();
    max_priority = max(max_priority, rule->priority);
    id2tuple[rule->priority] = tuple2insert;
    return 0;
}


// int RlTuple::InsertRule(Rule *rule) {
//     Tuple *tuple2insert = NULL;
//     if(id2tuple.find(rule->priority) != id2tuple.end()){
//         tuple2insert = id2tuple[rule->priority];
//     }
//     else{
//         int distance = 9999;
//         for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
//             int now_dis = 0;
//             Tuple *tuple = tuples_arr[tuple_index];
        
//             int field_index = 0;
//             for(; field_index < tuple->prefix_len.size();){
//                 int usedF = tuple->used_field[field_index];
//                 if(rule->prefix_len[usedF] < tuple->prefix_len[field_index]){
//                     break;
//                 }
//                 now_dis += (rule->prefix_len[usedF] - tuple->prefix_len[field_index]);
//                 field_index ++;
//             }
//             if(field_index == tuple->prefix_len.size()){
//                 now_dis = now_dis / field_index;
//                 if(distance > now_dis){
//                     distance = now_dis;
//                     tuple2insert = tuple;
//                 }
//             }
//         }
//     }

//     //unlikely
//     if(tuple2insert == NULL){
//         vector<uint32_t> used_field = {0};
//         vector<uint32_t> used_field_lenth = {0};
//         tuple2insert = new Tuple(used_field, used_field_lenth, use_port_hash_table);
//         InsertTuple(tuple2insert);
//         // tuples_map[prefix_pair] = tuple;
//     }

//     if (tuple2insert->InsertRuleRL(rule) > 0)
//         return 1;
//     ++rules_num;
//     if (rule->priority == tuple2insert->max_priority)
//         SortTuples();
//     max_priority = max(max_priority, rule->priority);
//     id2tuple[rule->priority] = tuple2insert;
//     return 0;
// }

vector<int> RlTuple::getObs(int N, pair<vector<uint32_t>, vector<uint32_t>> current){
    vector<int> obs;
    int current_index = -1;
    int i, j;

    for (i = 0 ; i < tuples_num; ++i) {
        auto tuple =  tuples_arr[i];
        if(tuple->used_field == current.first && tuple->prefix_len == current.second){
            current_index = i;
            for(j = 0; j< tuple->used_field.size() && j < 2; j++){
                obs.push_back(tuple->used_field[j]);
                obs.push_back(tuple->prefix_len[j]);
            }
            while(j < 2){
                obs.push_back(0);
                obs.push_back(0);
                j ++;
            }
            auto infos = getListInfo(tuple);
            obs.push_back(infos.first);
            obs.push_back(infos.second);
            N --;
            break;
        }
    }

    for (i = 0 ; i < tuples_num; ++i) {
        if(i == current_index){
            continue;
        }
        auto tuple =  tuples_arr[i];
        if(N == 0){
            break;
        }
        for(j = 0; j< tuple->used_field.size() && j < 2; j++){
            obs.push_back(tuple->used_field[j]);
            obs.push_back(tuple->prefix_len[j]);
        }
        while(j < 2){
            obs.push_back(0);
            obs.push_back(0);
            j ++;
        }
        auto infos = getListInfo(tuple);
        obs.push_back(infos.first);
        obs.push_back(infos.second);
        N --;
    }
    while(N --){
        for(j = 0; j < 6 ;j++){
            obs.push_back(0);
        }
    }
    return obs;
}

void RlTuple::addTuple(vector<uint32_t> _used_field, vector<uint32_t> _used_field_lenth){
    // for (int i = 0; i < tuples_num; ++i) {
    //     if(tuples_arr[i]->used_field == _used_field && tuples_arr[i]->prefix_len == _used_field_lenth){
    //         printf("already ex\n");
    //         return;
    //     }
    // }
    vector<uint32_t> used_field = _used_field;
    vector<uint32_t> used_field_lenth = _used_field_lenth;
    auto tuple = new Tuple(used_field, used_field_lenth, use_port_hash_table);
    InsertTuple(tuple);
}

void RlTuple::step(vector<Rule*> &rules){
    
    for (auto rule : rules){
        InsertRule(rule);
    }

    // for (int i = 0; i < tuples_num; ++i) {
    //     Tuple *tuple1 = tuples_arr[i];
    //     for(auto prefixL : tuple1->prefix_len){
    //         printf("%d ", prefixL);
    //     }
    //     printf("%d\n", tuple1->rules_num);
    // }
}

int RlTuple::InsertRuleTuple(Rule *rule, Tuple* tuple) {
    if (tuple == NULL){
        return 1;
    }
    if (tuple->InsertRuleRL(rule) > 0)
        return 1;
    ++rules_num;
    if (rule->priority == tuple->max_priority)
        SortTuples();
    max_priority = max(max_priority, rule->priority);
    return 0;
}

int RlTuple::DeleteRule(Rule *rule) {
    // for(auto keys: id2tuple){
    //     printf("%d %d", keys.first, keys.second->max_priority);
    // }
    auto tuple = id2tuple[rule->priority];
    if (tuple == NULL)
        return 1;
    if (tuple->DeleteRuleRL(rule) > 0)
        return 1;
    
    --rules_num;
    if (tuple->rules_num == 0) {
        // printf("delete tuple by tuple id\n");
		tuple->max_priority = 0;
        SortTuples();
        // tuples_arr[--tuples_num] = NULL;
        // tuple->Free(true);
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

vector<int> RlTuple::evaluateByaccess(){
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

unordered_map<double, int> RlTuple::evaluateWithWeightEmpty(unordered_map<int,int> &weights, vector<Rule*> rules){
    unordered_map<double, int> lookup_distribution;
    uint64_t check_hash_cost = 5;
    uint64_t check_group_cost = 2;
    uint64_t check_rule_cost = 3;
    for(int i=1; i<=rules.size(); i++){
        double lookup_time;
        lookup_time = 1 * check_hash_cost + 1 * check_group_cost + i * check_rule_cost;
        if(lookup_distribution.find(lookup_time) != lookup_distribution.end()){
            lookup_distribution[lookup_time] += weights[rules[i-1]->priority];
        }
        else{
            lookup_distribution[lookup_time] = weights[rules[i-1]->priority];
        }
    }
    return lookup_distribution;
}


unordered_map<double, int> RlTuple::evaluateWithWeight(unordered_map<int,int> &weights, double trade_off){
    unordered_map<double, int> lookup_distribution;
    uint64_t check_hash_cost = 5;
    uint64_t check_group_cost = 2;
    uint64_t check_rule_cost = 3;
    double N_group_L = 0;
    vector<double> N_group_LL, occupation;
    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;

        int totol_l = 0, successive_l = 0;

        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            auto group_node = hash_node_arr[group_index];
            while(group_node != NULL){
                successive_l ++;
                group_node = group_node->next;
            }

            totol_l += successive_l * successive_l;
            successive_l = 0;
        }
        auto _occupation = (double)totol_l / tuple->hash_table.hash_node_num;
        // auto _occupation = (double)totol_l / (tuple->hash_table.mask + 1);
        // auto _occupation = (double)tuple->hash_table.hash_node_num / (tuple->hash_table.mask + 1);
        occupation.push_back(_occupation);
        N_group_L += _occupation;
        N_group_LL.push_back(N_group_L);
    }

    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;
        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            int level_group = 1;
            auto group_node = hash_node_arr[group_index];
            while(group_node != NULL){
                int level_rule = 1;
                
                auto rule_node = group_node->rule_node;
                int tuple_index1 = tuple_index + 1;
                while(rule_node != NULL){
                    int weight = weights[rule_node->priority];
                    for ( ; tuple_index1 < tuples_num; tuple_index1 ++) {
                        if(rule_node->priority >= tuples_arr[tuple_index1]->max_priority){
                            break;
                        }
                    }
                    // N_tuple += tuple_index1; //tuple
                    // N_group += (N_group_LL[tuple_index1 - 1] - occupation[tuple_index]);  // outer_group
                    // N_group += level_group ; // inner_group
                    // N_rule += level_rule; // inner_rule
                    double lookup_time_0,lookup_time_1;
                    lookup_time_0 = tuple_index1 * check_hash_cost +\
                                    (N_group_LL[tuple_index1 - 1] - occupation[tuple_index] + level_group) * check_group_cost + \
                                    (level_rule + N_group_LL[tuple_index1 - 1] - occupation[tuple_index]) * check_rule_cost;
                   
                    lookup_time_1 = check_hash_cost +\
                                    level_group * check_group_cost + \
                                level_rule * check_rule_cost;
                    
                    double lookup_time = (1-trade_off) * lookup_time_0 + trade_off * lookup_time_1;
                    
                    if(lookup_distribution.find(lookup_time) != lookup_distribution.end()){
                        lookup_distribution[lookup_time] += weight;
                    }
                    else{
                        lookup_distribution[lookup_time] = weight;
                    }


                    rule_node = rule_node->next;
                    ++ level_rule;
                }
                ++ level_group;
                group_node = group_node->next;
            }
        }
    }
    // auto cost_all = check_hash_cost * N_tuple + N_group * check_group_cost + N_rule * check_rule_cost;
    // printf("evaluateWithWeight : %d %lf %d %lf\n", N_tuple, N_group, N_rule, cost_all);
    // return check_hash_cost * N_tuple + N_group * check_group_cost + N_rule * check_rule_cost;
    return lookup_distribution;
}

double RlTuple::evaluate(){
    // 0.0689
    // 0.0166
    // 0.0059
    double check_hash_cost = 5;
    double check_group_cost = 2;
    double check_rule_cost = 3;
    int N_tuple = 0, N_rule = 0;
    double N_group = 0;

    vector<double> T_use;
    for (int tuple_index = 0; tuple_index < tuples_num; ++tuple_index) {
        Tuple *tuple = tuples_arr[tuple_index];
        
        auto hash_table = tuple->hash_table;
        auto hash_node_arr = hash_table.hash_node_arr;

        int totol_l = 0, successive_l = 0;

        for(int group_index=0; group_index<= hash_table.mask; ++group_index){
            auto group_node = hash_node_arr[group_index];
            while(group_node != NULL){
                successive_l ++;
                group_node = group_node->next;
            }

            totol_l += successive_l * successive_l;
            successive_l = 0;
        }
        auto _occupation = (double)totol_l / tuple->hash_table.hash_node_num;
        // auto _occupation = (double)totol_l / (tuple->hash_table.mask + 1);
        // auto _occupation = (double)tuple->hash_table.hash_node_num / (tuple->hash_table.mask + 1);
        T_use.push_back(_occupation);
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
                N_group += group_node->rules_num * level;
                N_rule += group_node->rules_num * (group_node->rules_num + 1) / 2;
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
                ++ level;
            }
        }
    }
    auto cost_all = check_hash_cost * N_tuple + N_group * check_group_cost + N_rule * check_rule_cost;
    printf("evaluate : %d %lf %d %lf\n", N_tuple, N_group, N_rule, cost_all);
    return cost_all;
}

vector<int> RlTuple::LookupACcnt(Trace *trace, int priority) {
    int last_pri = 0;
    int N_tuple = 0;
    int N_group_inner = 0, N_group_all = 0;
    int N_rule_inner = 0, N_rule_all = 0;
    int N_group_temp = 0, N_rule_temp = 0;
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;

        N_tuple ++;

        uint64_t key = trace->key[tuple->used_field[0]] >> tuple->prefix_len_zero[0];
        uint32_t hash = key;
        for (int j = 1; j < tuple->used_field.size(); j++){
            auto val2calhash = trace->key[tuple->used_field[j]] >> tuple->prefix_len_zero[j];
            hash = Hash32_2(hash, val2calhash);
            key = (key << tuple->prefix_len[j]) | val2calhash;
        }

        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash & tuple->hash_table.mask];
        
        N_group_temp = 0;
        N_rule_temp = 0;

        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;

            N_group_all ++;
            N_group_temp ++;

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
                                };
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

                    N_rule_all ++;
                    N_rule_temp ++;

                    if (rule_node->src_ip_begin <= trace->key[0] && trace->key[0] <= rule_node->src_ip_end &&
                        rule_node->dst_ip_begin <= trace->key[1] && trace->key[1] <= rule_node->dst_ip_end &&
                        rule_node->src_port_begin <= trace->key[2] && trace->key[2] <= rule_node->src_port_end &&
                        rule_node->dst_port_begin <= trace->key[3] && trace->key[3] <= rule_node->dst_port_end &&
                        rule_node->protocol_begin <= trace->key[4] && trace->key[4] <= rule_node->protocol_end) {
                        priority = rule_node->priority;

                        if(last_pri != priority){
                            N_group_inner = N_group_temp;
                            N_rule_inner = N_rule_temp;
                            N_group_temp = 0;
                            N_rule_temp = 0;
                        }

                        last_pri = priority;

                        break;
                    }
                    rule_node = rule_node->next;  
                };
                break;
            }
            hash_node = hash_node->next;
        }
    }
    return vector<int> {priority, N_tuple, N_group_inner, N_group_all, N_rule_inner, N_rule_all};
}

int RlTuple::TupleNum(){
    int i = 0;
    for (; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if(tuple->rules_num == 0){
            break;
        }
    }
    return i;
}

int RlTuple::Lookup(Trace *trace, int priority) {
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;

        uint64_t key = trace->key[tuple->used_field[0]] >> tuple->prefix_len_zero[0];
        uint32_t hash = key;
        for (int j = 1; j < tuple->used_field.size(); j++){
            auto val2calhash = trace->key[tuple->used_field[j]] >> tuple->prefix_len_zero[j];
            hash = Hash32_2(hash, val2calhash);
            key = (key << tuple->prefix_len[j]) | val2calhash;
        }


        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash & tuple->hash_table.mask];
        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;
            if (hash_node->key == key) {
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
    // if(priority == 0){
    //     for(auto keys: id2tuple){
    //         printf("%d %d\n", keys.first, keys.second->max_priority);
    //     }
    //     auto hash_table = id2tuple[trace->rule_id]->hash_table;
    //     auto hash_node_arr = hash_table.hash_node_arr;
    //     for(int group_index=0; group_index<= hash_table.mask; ++group_index){
    //         auto group_node = hash_node_arr[group_index];
    //         while(group_node != NULL){
    //             printf("%d:", group_index);
    //             auto rule_node = group_node->rule_node;
    //             while(rule_node != NULL){
    //                 printf("%d,", rule_node->priority);
    //                 rule_node = rule_node->next;
    //             }
    //             group_node = group_node->next;
    //             printf("\n");
    //         }
    //     }
    // }
    return priority;
}

int RlTuple::LookupAccess(Trace *trace, int priority, Rule *ans_rule, ProgramState *program_state) {
    program_state->AccessClear();
    
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;
        program_state->access_tuples.AddNum();
        
        uint64_t key = trace->key[tuple->used_field[0]] >> tuple->prefix_len_zero[0];
        uint32_t hash = key;
        for (int j = 1; j < tuple->used_field.size(); j++){
            auto val2calhash = trace->key[tuple->used_field[j]] >> tuple->prefix_len_zero[j];
            hash = Hash32_2(hash, val2calhash);
            key = (key << tuple->prefix_len[j]) | val2calhash;
        }
        
        program_state->access_tables.AddNum();

        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash & tuple->hash_table.mask];
        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;
            program_state->access_nodes.AddNum();
            if (hash_node->key == key) {
                RuleNode *rule_node = hash_node->rule_node;
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
            hash_node = hash_node->next;
        }
    }
    program_state->AccessCal();
	
    return priority;
}

int RlTuple::LookupAccessWithClock(Trace *trace, int priority) {
    int n_tuple = 0, n_group = 0, n_rule = 0;
    for (int i = 0; i < tuples_num; ++i) {
        Tuple *tuple = tuples_arr[i];
        if (priority >= tuple->max_priority)
            break;
        n_tuple ++;
        
        uint64_t key = trace->key[tuple->used_field[0]] >> tuple->prefix_len_zero[0];
        uint32_t hash = key;
        for (int j = 1; j < tuple->used_field.size(); j++){
            auto val2calhash = trace->key[tuple->used_field[j]] >> tuple->prefix_len_zero[j];
            hash = Hash32_2(hash, val2calhash);
            key = (key << tuple->prefix_len[j]) | val2calhash;
        }

        HashNode *hash_node = tuple->hash_table.hash_node_arr[hash & tuple->hash_table.mask];
        while (hash_node) {
            if (priority >= hash_node->max_priority)
                break;
            n_group ++;
            if (hash_node->key == key) {
                RuleNode *rule_node = hash_node->rule_node;
                while (rule_node) {
                    if (priority >= rule_node->rule->priority)
                        break;
                    n_rule ++;
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

int RlTuple::Reconstruct() {
    printf("reconstruct rltuple not implement!\n");
    return 0;
}

uint64_t RlTuple::MemorySize() {
    uint64_t memory_size = sizeof(RlTuple);
    memory_size += sizeof(Tuple*) * max_tuples_num;
    memory_size += 64 * tuples_num; // map
    for (int i = 0; i < tuples_num; ++i) {
        memory_size += tuples_arr[i]->MemorySize();
        //printf("%d\n", tuples_arr[i]->MemorySize());
        //return memory_size;

    }
    return memory_size;
}

int RlTuple::GetRules(vector<Rule*> &rules) {
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->GetRules(rules);
    return 0;
}

int RlTuple::Free(bool free_self) {
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->Free(true);
    free(tuples_arr);
    tuples_map.clear();
    if (free_self)
        free(this);
    return 0;
}

int RlTuple::CalculateState(ProgramState *program_state) {
    program_state->dt_time += dt_time;
    program_state->tuples_num = tuples_num;
    program_state->tuples_sum += tuples_num;

    //printf("tuples_num %d\n", tuples_num);
    for (int i = 0; i < tuples_num; ++i)
        tuples_arr[i]->CalculateState(program_state);
    return 0;
}

int RlTuple::Test(void *ptr) {
    return 0;
}

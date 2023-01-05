#include "elementary.h"

using namespace std;

void CommandStruct::Init() {
	run_mode = "";
	method_name = "";

	rules_file = "";
	traces_file = "";
	ans_file = "";
	output_file = "";

	rules_shuffle = 0;
    lookup_round = 1;
    force_test = 0;
	print_mode = 0;
}
CommandStruct command_empty;

bool SameRule(Rule *rule1, Rule *rule2) {
	for (int i = 0; i < RULE_FIELD_NUM; ++i)
		for (int j = 0; j < 2; ++j)
			if (rule1->range[i][j] != rule2->range[i][j])
				return false;
	for (int i = 0; i < RULE_FIELD_NUM; ++i)
		if (rule1->prefix_len[i] != rule2->prefix_len[i])
			return false;
	if (rule1->priority != rule2->priority)
		return false;
	return true;
}

bool CmpRulePriority(Rule *rule1, Rule *rule2) {
    return rule1->priority > rule2->priority;
}

bool CmpPtrRulePriority(Rule *rule1, Rule *rule2) {
    return rule1->priority > rule2->priority;
}

bool MatchRuleTrace(Rule *rule, Trace *trace) {
	for (int i = 0; i < RULE_FIELD_NUM; i++)
		if (trace->key[i] < rule->range[i][0] || trace->key[i] > rule->range[i][1])
			return false;
	return true;
}

uint64_t GetRunTimeUs(timeval timeval_start, timeval timeval_end) {  // us
	return 1000000 * (timeval_end.tv_sec - timeval_start.tv_sec)+ timeval_end.tv_usec - timeval_start.tv_usec;
}

uint64_t GetAvgTime(vector<uint64_t> &lookup_times) {
	int num = lookup_times.size();
	if (num == 0)
		return 0;
	sort(lookup_times.begin(), lookup_times.end());
	int l = num / 4;
	int r = num - l;
	//printf("GetAvgTime num %d l %d r %d\n", num, l, r);
	uint64_t sum = 0;
	for (int i = l; i < r; ++i)
		sum += lookup_times[i];
	return sum / (r - l);
}

int Popcnt(uint64_t num) {
	return __builtin_popcountll(num);
}

void ProgramState::AccessClear() {
    access_tuples.ClearNum();
    access_tables.ClearNum();
    access_nodes.ClearNum();
    access_rules.ClearNum();

}
void ProgramState::AccessCal() {
	max_access_all = max(max_access_all, access_tuples.num + access_tables.num + access_nodes.num + access_rules.num);
	max_access_tuple_node_rule = max(max_access_tuple_node_rule, access_tuples.num + access_nodes.num + access_rules.num);
	max_access_node_rule = max(max_access_node_rule, access_nodes.num + access_rules.num);

    access_tuples.Update();
    access_tables.Update();
    access_nodes.Update();
    access_rules.Update();
}


void setRuleFieldByname(struct Rule* r, string filed_name, uint64_t filed_left, uint64_t filed_right, int prefix){
	int filed_index = 0;
	if(filed_name == "nw_src"){
		filed_index = 0;
	}
	else if(filed_name == "nw_dst"){
		filed_index = 1;
	}
	else if(filed_name == "tp_src"){
		filed_index = 2;
	}
	else if(filed_name == "tp_dst"){
		filed_index = 3;
	}
	else if(filed_name == "nw_proto"){
		filed_index = 4;
	}
	else if(filed_name == "in_port"){
		filed_index = 5;
	}
	else if(filed_name == "dl_src"){
		filed_index = 6;
	}
	else if(filed_name == "dl_dst"){
		filed_index = 7;
	}
	else if(filed_name == "eth_type"){
		filed_index = 8;
	}
	else if(filed_name == "dl_vlan"){
		filed_index = 9;
	}
	else if(filed_name == "dl_vlan_pcp"){
		filed_index = 10;
	}
	else if(filed_name == "nw_tos"){
		filed_index = 11;
	}
	r->range[filed_index][0] = filed_left;
	r->range[filed_index][1] = filed_right;
	r->prefix_len[filed_index] = prefix;
}

void fillEmptyField(struct Rule * rule){
	for(int i =0 ;i <RULE_FIELD_NUM; i++){
		if(rule->range[i][1] == 0){
			rule->range[i][1] = (1ULL << RULE_FILED_MAXLENTH[i]) - 1;
		}
	}
}
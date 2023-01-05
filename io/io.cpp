#include "io.h"

using namespace std;

vector<string> StrSplit(const string& str, const string& pattern) {
    vector<string> ret;
    if(pattern.empty())
        return ret;
    int start = 0, index = str.find_first_of(pattern, 0);
    while(index != str.npos) {
        if(start != index)
            ret.push_back(str.substr(start, index-start));
        start = index + 1;
        index = str.find_first_of(pattern, start);
    }
    if(!str.substr(start).empty())
        ret.push_back(str.substr(start));
    return ret;
}

int Count1(uint64_t num) {
	int ans = 0;
	while (num) {
		ans += num & 1;
		num /= 2;
	}
	return ans;
}

uint64_t GetMac(string str) {
    auto mac = StrSplit(str.append("\0"), ":");
    uint64_t n1 = stoi(mac[0],0,16), n2 = stoi(mac[1],0,16), n3 = stoi(mac[2],0,16), n4 = stoi(mac[3],0,16), n5 = stoi(mac[4],0,16), n6 = stoi(mac[5],0,16);
	return (n1 << 40) | (n2 << 32) | (n3 << 24) | (n4 << 16) | (n5 << 8) | n6;
}

uint32_t GetIp(string str){
    uint32_t num1, num2, num3, num4;
	sscanf(str.c_str(),"%u.%u.%u.%u", &num1, &num2, &num3, &num4);
	return (num1 << 24) | (num2 << 16) | (num3 << 8) | num4;
}

string GetIpStr(uint32_t ip) {
    char str[100];
    sprintf(str, "%d.%d.%d.%d", ip >> 24, ip >> 16 & 255, ip >> 8 & 255, ip & 255);
    return str;
}

void PrintIp(uint32_t ip) {
	printf("%d.%d.%d.%d", ip >> 24, ip >> 16 & 255, ip >> 8 & 255, ip & 255);
}

void PrintRule(Rule *rule) {
	PrintIp(rule->range[0][0]);
	printf("/%d ", rule->prefix_len[0]);
	PrintIp(rule->range[1][0]);
	printf("/%d ", rule->prefix_len[1]);
    for (int i = 2; i < 5; ++i)
        printf("%d:%d ", rule->range[i][0], rule->range[i][1]);
	printf("priority %d\n", rule->priority);
}

vector<Rule*> ReadRules(string rules_file, int rules_shuffle) {
    vector<Rule*> rules;
	int rules_num = 0;
    char buf[1025];

	FILE *fp = fopen(rules_file.c_str(), "rb");
	if (!fp)
		printf("Cannot open the file %s\n", rules_file.c_str());
    int priority = 0;
    while (fgets(buf,1000,fp)!=NULL)
        ++priority;
    
    fp = fopen(rules_file.c_str(), "rb");
	while (fgets(buf,1000,fp)!=NULL) { 
        string str = buf;
        vector<string> vc = StrSplit(str, ", \r\n");

        Rule *rule = (Rule*)malloc(sizeof(Rule));
        memset(rule, 0, sizeof(Rule));
        fillEmptyField(rule);

        for(auto s : vc){
            vector<string> left_right = StrSplit(s, "=");
            string left = left_right[0];
            string right = left_right[1];
            if(left == "nw_src" || left == "nw_dst"){
                //ip
                vector<string> left_right_ip_prefix = StrSplit(right, "/");
                uint32_t ip_left = GetIp(left_right_ip_prefix[0]);
                int prefix = atoi(left_right_ip_prefix[1].c_str());

                uint32_t mask = (1LL << (32 - prefix)) - 1;
                ip_left -= ip_left & mask;
			    uint32_t ip_right = ip_left | mask;

                setRuleFieldByname(rule, left, ip_left, ip_right, prefix);
            }
            else if(left  == "tp_src" || left == "tp_dst"){
                // range
                vector<string> left_right_range = StrSplit(right, ":");
                int port_start = atoi(left_right_range[0].c_str());
                int port_end = (left_right_range.size() == 1) ? port_start : atoi(left_right_range[1].c_str());
                setRuleFieldByname(rule, left, port_start, port_end, GetPortMaskSimple(port_start, port_end));
            }
            else if (left == "nw_proto" || left == "nw_tos"){
                setRuleFieldByname(rule, left, atoi(right.c_str()), atoi(right.c_str()), 8);
            }
            else if (left == "dl_src" || left == "dl_dst"){
                //dl_dst=fa:16:3e:d8:f3:46
                vector<string> left_right_mac_prefix = StrSplit(right, "/");
                uint64_t mac_left = GetMac(left_right_mac_prefix[0]);
                
                int prefix = (left_right_mac_prefix.size() == 1) ? 48 : atoi(left_right_mac_prefix[1].c_str());

                uint64_t mask = (1LL << (48 - prefix)) - 1;
                mac_left -= mac_left & mask;
			    uint64_t mac_right = mac_left | mask;

                setRuleFieldByname(rule, left, mac_left, mac_right, prefix);
            }
            else if (left == "in_port"){
                setRuleFieldByname(rule, left, atoi(right.c_str()), atoi(right.c_str()), 32);
            }
            else if (left == "dl_vlan"){
                setRuleFieldByname(rule, left, atoi(right.c_str()), atoi(right.c_str()), 12);
            }
            else if (left == "dl_vlan_pcp"){
                setRuleFieldByname(rule, left, atoi(right.c_str()), atoi(right.c_str()), 3);
            }
            else if (left == "eth_type"){
                setRuleFieldByname(rule, left, strtoul(right.c_str(),0,0), strtoul(right.c_str(),0,0), 16);
            }
            else {
                printf("rule err!\n");
            }
        }

        // priority
        rule->priority = priority--;

        rules.push_back(rule);
        ++rules_num;
    }
    if (rules_shuffle > 0) {
        random_shuffle(rules.begin(),rules.end());
    }
    //rules.resize(10);
	//printf("rules_num = %ld\n", rules.size());
    return rules;
}

int port_bit_mask[17][2] ={ { 0, 0xffff },
    { 0x1, 0xfffe }, { 0x3, 0xfffc }, { 0x7, 0xfff8 }, { 0xf, 0xfff0 },
    { 0x1f, 0xffe0 }, { 0x3f, 0xffc0 }, { 0x7f, 0xff80 },
    { 0xff, 0xff00 }, { 0x1ff, 0xfe00 }, { 0x3ff, 0xfc00 },
    { 0x7ff, 0xf800 }, { 0xfff, 0xf000 }, { 0x1fff, 0xe000 },
    { 0x3fff, 0xc000 }, { 0x7fff, 0x8000 }, { 0xffff, 0 }
};

struct PrefixRange {
    uint32_t low;
    uint32_t high;
    uint32_t prefix_len;
};

int GetPortMaskSimple(int port_start, int port_end) {
    if (port_start == port_end){
        return 16;
    }
    return 0;
}


vector<PrefixRange> GetPortMask(int port_start, int port_end) {
    vector<PrefixRange> prefix;
    PrefixRange prefix_range;
    for (int pos = port_start; pos <= port_end;) {
        int i;
        for (i = 1; i < 17; ++i) {
            int pos2 = pos + port_bit_mask[i][0];
            if (pos2 > port_end || (pos & port_bit_mask[i][1]) != (pos2 & port_bit_mask[i][1]))
                break;
        }
        --i;
        prefix_range.low = pos;
        prefix_range.high = pos + port_bit_mask[i][0];
        prefix_range.prefix_len = 16 - i;
        prefix.push_back(prefix_range);
        pos += (port_bit_mask[i][0] + 1);
    }
    return prefix;
}

vector<Rule*> RulesPortPrefix(vector<Rule*> &rules, bool free_rules) {
    vector<Rule*> prefix_rules;
    int rules_num = rules.size();
    for (int i = 0; i < rules_num; ++i) {
        vector<PrefixRange> src_port_prefix = GetPortMask(rules[i]->range[2][0], rules[i]->range[2][1]);
        vector<PrefixRange> dst_port_prefix = GetPortMask(rules[i]->range[3][0], rules[i]->range[3][1]);
        int src_port_prefix_num = src_port_prefix.size();
        int dst_port_prefix_num = dst_port_prefix.size();
        for (int j = 0; j < src_port_prefix_num; ++j)
            for (int k = 0; k < dst_port_prefix_num; ++k) {
                Rule *rule = (Rule*)malloc(sizeof(Rule));
                *rule = *rules[i];

                rule->range[2][0] = src_port_prefix[j].low;
                rule->range[2][1] = src_port_prefix[j].high;
                rule->prefix_len[2] = src_port_prefix[j].prefix_len;

                rule->range[3][0] = dst_port_prefix[k].low;
                rule->range[3][1] = dst_port_prefix[k].high;
                rule->prefix_len[3] = dst_port_prefix[k].prefix_len;

                prefix_rules.push_back(rule);
            }
    }
    if (free_rules)
        FreeRules(rules);
    return prefix_rules;
}

vector<Rule*> UniqueRules(vector<Rule*> &rules) {
    vector<Rule*> unique_rules;
    map<Rule, int> match;
    int rules_num = rules.size();
    Rule rule;
    for (int i = 0; i < rules_num; ++i) {
        rule = *rules[i];
        rule.priority = 0;
        if (match.find(rule) == match.end()) {
            match[rule] = 1;
            unique_rules.push_back(rules[i]);
        }
    }
    // printf("UniqueRules %ld to %ld\n", rules.size(), unique_rules.size());
    match.clear();
    return unique_rules;
}

vector<Rule*> UniqueRulesIgnoreProtocol(vector<Rule*> &rules) {
    vector<Rule*> unique_rules;
    map<Rule, int> match;
    int rules_num = rules.size();
    Rule rule;
    for (int i = 0; i < rules_num; ++i) {
        rule = *rules[i];
        rule.range[4][0] = 0;
        rule.range[4][1] = 0;
        rule.prefix_len[4] = 0;
        rule.priority = 0;
        if (match.find(rule) == match.end()) {
            match[rule] = 1;
            unique_rules.push_back(rules[i]);
        }
    }
    // printf("UniqueRulesIgnoreProtocol %ld to %ld\n", rules.size(), unique_rules.size());
    match.clear();
    return unique_rules;
}

void PrintRules(vector<Rule*> &rules, string rules_file, bool print_priority) {
    int rules_num = rules.size();
    FILE *fp = fopen(rules_file.c_str(), "w");
    for (int i = 0; i < rules_num; ++i) {
        fprintf(fp, "@");
        for (int j = 0; j < 2; ++j) {
            fprintf(fp, "%s/%d\t", GetIpStr(rules[i]->range[j][0]).c_str(), rules[i]->prefix_len[j]);
        }
        for (int j = 2; j < 4; ++j)
            fprintf(fp, "%d : %d\t", rules[i]->range[j][0], rules[i]->range[j][1]);
        if (rules[i]->range[4][0] == rules[i]->range[4][1])
            fprintf(fp, "0x%02x/0xFF\t", rules[i]->range[4][0]);
        else
            fprintf(fp, "0x%02x/0x00\t", rules[i]->range[4][0]);
        fprintf(fp, "0x0000/0x0000\t");
        if (print_priority)
            fprintf(fp, "%d\t", rules[i]->priority);
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void PrintRulesPrefix(vector<Rule*> &rules, string rules_file, bool print_priority) {
    int rules_num = rules.size();
    FILE *fp = fopen(rules_file.c_str(), "w");
    for (int i = 0; i < rules_num; ++i) {
        fprintf(fp, "@");
        for (int j = 0; j < 2; ++j) {
            fprintf(fp, "%s/%d\t", GetIpStr(rules[i]->range[j][0]).c_str(), rules[i]->prefix_len[j]);
        }
        for (int j = 2; j < 4; ++j)
            fprintf(fp, "0x%04x/%d\t", rules[i]->range[j][0], rules[i]->prefix_len[j]);
        fprintf(fp, "0x%02x/%d\t", rules[i]->range[4][0], rules[i]->prefix_len[4]);
        // fprintf(fp, "0x0000/0x0000\t");
        if (print_priority)
            fprintf(fp, "%d\t", rules[i]->priority);
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void PrintRulesMegaFlow(vector<Rule> &rules, string rules_file, bool print_priority) {
    int rules_num = rules.size();
    FILE *fp = fopen(rules_file.c_str(), "w");
    for (int i = 0; i < rules_num; ++i) {
        fprintf(fp, "@");
        for (int j = 0; j < 2; ++j) {
            fprintf(fp, "%s/%d\t", GetIpStr(rules[i].range[j][0]).c_str(), rules[i].prefix_len[j]);
        }
        for (int j = 2; j < 4; ++j)
            fprintf(fp, "0x%04x/%d\t", rules[i].range[j][0], rules[i].prefix_len[j]);
        
        if (print_priority)
            fprintf(fp, "%d\t", rules[i].priority);
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void CheckMegaFlowRules(vector<Rule> &rules) {
    int rules_num = rules.size();
    for (int i = 0; i < rules_num; ++i) {
        for (int j = i + 1; j < rules_num; ++j) {
            bool contain = true;
            for (int k = 0; k < 4; ++k) {
                int min_prefix_len = min(rules[i].prefix_len[k], rules[j].prefix_len[k]);
                if (k == 2 || k == 3)
                    min_prefix_len += 16;
                if (rules[i].range[k][0] >> (32 - min_prefix_len) != 
                    rules[j].range[k][0] >> (32 - min_prefix_len))
                    contain = false;
            }
            if (contain)
                printf("contain %d %d\n", i, j);
        }
    }
}

Rule rule_empty;
void MegaFlowRules(vector<Rule*> &rules, vector<Trace*> &traces, string rules_file) {
    Trie src_ip_trie;
    Trie dst_ip_trie;
    Trie src_port_trie;
    Trie dst_port_trie;
    src_ip_trie.Init();
    dst_ip_trie.Init();
    src_port_trie.Init();
    dst_port_trie.Init();
    int rules_num = rules.size();
    int traces_num = traces.size();
    for (int i = 0; i < rules_num; ++i) {
        src_ip_trie.InsertRule(rules[i]->range[0][0], rules[i]->prefix_len[0], rules[i]->priority);
        dst_ip_trie.InsertRule(rules[i]->range[1][0], rules[i]->prefix_len[1], rules[i]->priority);
        src_port_trie.InsertRule(rules[i]->range[2][0], rules[i]->prefix_len[2] + 16, rules[i]->priority);
        dst_port_trie.InsertRule(rules[i]->range[3][0], rules[i]->prefix_len[3] + 16, rules[i]->priority);
        // printf("%s\n", GetIpStr(rules[i]->range[0][0]).c_str());
    }
    vector<Rule> megaflow_rules;
    map<Rule, int> match;
    // for (int i = 0; i < rules_num; ++i) {
    //     uint32_t src_ip = rules[i]->range[0][0];
    //     uint32_t dst_ip = rules[i]->range[1][0];
    //     uint32_t dst_port = rules[i]->range[3][0];
    for (int i = 0; i < traces_num; ++i) {
        // printf("%d\n", i);
        uint32_t src_ip = traces[i]->key[0];
        uint32_t dst_ip = traces[i]->key[1];
        uint32_t src_port = traces[i]->key[2];
        uint32_t dst_port = traces[i]->key[3];


        int src_ip_len = src_ip_trie.Lookup(src_ip, 32);
        int dst_ip_len = dst_ip_trie.Lookup(dst_ip, 32);
        int src_port_len = src_port_trie.Lookup(src_port, 32);
        int dst_port_len = dst_port_trie.Lookup(dst_port, 32);
        // printf("%d %d %d\n", src_ip_len, dst_ip_len, dst_port_len - 16);
        // printf("%d %d\n", i + 1, prefix_len);

        Rule rule = rule_empty;
        rule.range[0][0] = (src_ip >> (32 - src_ip_len)) << (32 - src_ip_len);
        rule.prefix_len[0] = src_ip_len;
        rule.range[1][0] = (dst_ip >> (32 - dst_ip_len)) << (32 - dst_ip_len);
        rule.prefix_len[1] = dst_ip_len;
        rule.range[2][0] = (src_port >> (32 - src_port_len)) << (32 - src_port_len);
        rule.prefix_len[2] = src_port_len - 16;
        rule.range[3][0] = (dst_port >> (32 - dst_port_len)) << (32 - dst_port_len);
        rule.prefix_len[3] = dst_port_len - 16;

        if (match.find(rule) == match.end()) {
            match[rule] = 1;
            // rule.priority = rules[i]->priority;
            megaflow_rules.push_back(rule);
        }
    }
    // printf("CheckMegaFlowRules\n");
    CheckMegaFlowRules(megaflow_rules);
    // printf("PrintRulesMegaFlow\n");
    PrintRulesMegaFlow(megaflow_rules, rules_file, false);
    // exit(1);
}

void GenerateTSEMegaflowRules() {
    // printf("GenerateTSEMegaflowRules\n");

    vector<Rule> megaflow_rules;
    for (int src_ip_len = 32; src_ip_len >= 1; --src_ip_len)
        for (int dst_ip_len = 32; dst_ip_len >= 1; --dst_ip_len)
            for (int src_port_len = 16; src_port_len >= 1; --src_port_len)
                for (int dst_port_len = 16; dst_port_len >= 1; --dst_port_len) {
                    Rule rule = rule_empty;
                    rule.range[0][0] = 1ULL << (32 - src_ip_len);
                    rule.prefix_len[0] = src_ip_len;
                    rule.range[1][0] = 1ULL << (32 - dst_ip_len);
                    rule.prefix_len[1] = dst_ip_len;
                    rule.range[2][0] = 1ULL << (16 - src_port_len);
                    rule.prefix_len[2] = src_port_len;
                    rule.range[3][0] = 1ULL << (16 - dst_port_len);
                    rule.prefix_len[3] = dst_port_len;
                    megaflow_rules.push_back(rule);
                }
    // printf("PrintRulesMegaFlow\n");

    PrintRulesMegaFlow(megaflow_rules, "data/tse_megaflow_rules", false);
    random_shuffle(megaflow_rules.begin(), megaflow_rules.end());
    PrintRulesMegaFlow(megaflow_rules, "data/tse_megaflow_rules_shuffle", false);
}

void PrintMegaFlowPacket(string rules_file) {
    FILE *fp = fopen(rules_file.c_str(), "w");
    for (uint32_t src_ip = 0xFFFFFFFFU; src_ip > 0; src_ip >>= 1)
        for (uint32_t dst_ip = 0xFFFFFFFFU; dst_ip > 0; dst_ip >>= 1)
            for (uint16_t src_port = 0xFFFF; src_port > 0; src_port >>= 1)
                for (uint16_t dst_port = 0xFFFF; dst_port > 0; dst_port >>= 1)
                    fprintf(fp, "0x%08x 0x%08x 0x%04x 0x%04x\n", src_ip, dst_ip, src_port, dst_port);
    fclose(fp);
}

int FreeRules(vector<Rule*> &rules) {
    int rules_num = rules.size();
    for (int i = 0; i < rules_num; ++i)
        free(rules[i]);
    rules.clear();
    return 0;
}

vector<Trace*> ReadTraces(string traces_file) {
    vector<Trace*> traces;
	int traces_num = 0;
    char buf[1025];

	FILE *fp = fopen(traces_file.c_str(), "rb");
	if (!fp)
		printf("Cannot open the file %s\n", traces_file.c_str());
	while (fgets(buf,1000,fp)!=NULL) { 
        string str = buf;
        vector<string> vc = StrSplit(str, "\t");
        Trace *trace = (Trace*)malloc(sizeof(Trace));
        for (int i = 0; i < RULE_FIELD_NUM; ++i)
            trace->key[i] = strtoull(vc[i].c_str(), NULL, 0);

        trace->rule_id = atoi(vc[RULE_FIELD_NUM].c_str());
        traces.push_back(trace);
        ++traces_num;
        //for (int i = 0; i < 5; ++i) printf("%u ", trace.key[i]); printf("\n");
        //if (traces_num >= 3) break;
    }
	//printf("traces_num = %ld\n", traces.size());
    return traces;
}

void SaveTraces(vector<Trace*> traces, string traces_file){
    FILE *fp = fopen(traces_file.c_str(), "wb");
	if (!fp)
		printf("Cannot open the file %s\n", traces_file.c_str());
    for(auto t : traces){
        for(int i=0; i< RULE_FIELD_NUM; i++){
            fprintf(fp, "%s\t", to_string(t->key[i]).c_str());
        }
        fprintf(fp, "%s\n", to_string(t->rule_id).c_str());
    }
    fclose(fp);
}

int MyPareto(float a, float b){
  static std::random_device e; 
  static std::uniform_real_distribution<double> u(0, 1);
  if (b == 0) return 1;

  double p = u(e);
  double x = (double)b / pow((double)(1 - p),(double)(1/(double)a));
  int Num = (int)ceil(x);
  return Num;
}

pair<pair<vector<Trace*>,vector<Trace*>>, unordered_map<int, int>> GenerateTraces(vector<Rule*> &rules, int size, bool pareto) {
    int rules_num = rules.size();
    unordered_map<int, int> rule_distrubution;
    vector<Trace*> tracesPerRule;
    for (int i = 0; i < rules_num; ++i) {
        Trace *trace = (Trace*)malloc(sizeof(Trace));
        for (int j = 0; j < RULE_FIELD_NUM; ++j)
            trace->key[j] = uint64_t(rules[i]->range[j][0] + (rules[i]->range[j][1] - rules[i]->range[j][0]) * 0.5);
        trace->rule_id = rules[i]->priority;
        tracesPerRule.push_back(trace);
    }
    vector<Trace*> traces;
    int index = 0;
    while(traces.size() < size){
        int copies;
        if(pareto){
            copies = MyPareto(1, 0.1);
        }
        else{
            copies = int(size / rules.size());
        }
        copies = copies <= (size - traces.size()) ? copies : (size - traces.size());
        auto rule = rules[(index++) % rules_num];
        Trace *trace = (Trace*)malloc(sizeof(Trace));
        for (int j = 0; j < RULE_FIELD_NUM; ++j)
            trace->key[j] = uint64_t(rule->range[j][0] + (rule->range[j][1] - rule->range[j][0]) * 0.5);
        trace->rule_id = rule->priority;

        for(int i=0; i<copies; i++)
            traces.push_back(trace);
        if(rule_distrubution.find(rule->priority)!=rule_distrubution.end()){
            rule_distrubution[rule->priority] += copies;
        }
        else{
            rule_distrubution[rule->priority] = copies;
        }
    }
    return make_pair(make_pair(traces, tracesPerRule), rule_distrubution);
}

int FreeTraces(vector<Trace*> &traces) {
    int traces_num = traces.size();
    for (int i = 0; i < traces_num; ++i)
        free(traces[i]);
    traces.clear();
    return 0;
}

bool SameTrace(Trace *trace1, Trace *trace2) {
	for (int i = 0; i < 5; i++)
		if (trace1->key[i] != trace2->key[i])
			return false;
	return true;
}

vector<int> GenerateAns(vector<Rule*> &rules, vector<Trace*> &traces, int force_test) {
    vector<int> ans;
    if (force_test == 0)
        return ans;
    int rules_num = rules.size();
    int traces_num = traces.size();
	for (int i = 0; i < traces_num; ++i) {
		if (i > 0 && SameTrace(traces[i - 1], traces[i])) {
            ans.push_back(ans[i - 1]);
			continue;
		}
        int priority = 0;
		for (int j = 0; j < rules_num; ++j) {
			if (force_test == 2 && j % 4 == 0) continue;
			if (rules[j]->priority > priority && MatchRuleTrace(rules[j], traces[i])) {
				priority = rules[j]->priority;
			}
		}
        ans.push_back(priority);
	    //printf("%d %d\n", i, priority);
	}
	//printf("ans_num = %ld\n", ans.size());
    return ans;
}

void PrintAns(string output_file, vector<int> &ans) {
    int ans_num = ans.size();
	FILE *fp = fopen(output_file.c_str(), "wb");
	if (!fp)
		printf("Cannot open the file %s\n", output_file.c_str());
    for (int i = 0; i < ans_num; ++i)
        fprintf(fp, "%d\n", ans[i]);
}

unordered_map<string, string> ParseArgs(int argc, char* argv[]) {
	unordered_map<string, string> results;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		vector<string> tokens;
		Split(arg, '=', tokens);
		if (tokens.size() == 2) {
			results[tokens[0]] = tokens[1];
		} else if (tokens.size() == 1) {
			results[tokens[0]] = "1";
		} else {
			printf("Wrong number of tokens! %s\n", arg.c_str());
		}
	}
	return results;
}

const string& GetOrElse(const unordered_map<string, string> &m, const string& key, const string& def) {
	if (m.find(key) == m.end()) return def;
	else return m.at(key);
}

void Split(const string &s, char delim, vector<string>& tokens) {
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		tokens.push_back(item);
	}
}

map<double, int> getWeibullInterval(int size){
    std::mt19937 gen(1701);
    std::weibull_distribution<> distr(1.0, 1.0);
    std::map<double, int> histogram;
    for (int i = 0; i < size; ++i) {
        auto interval = distr(gen);
        if(interval <= 10)
        ++histogram[distr(gen)];
    }
    return histogram;
}
#include "ps-io.h"

using namespace std;

vector<PSRule> GeneratePSRules(vector<Rule*> &rules) {
	vector<PSRule> ps_rules;
	int rules_num = rules.size();
	for (int i = 0; i < rules_num; ++i) {
		PSRule rule;
		rule.priority = rules[i]->priority;
		rule.markedDelete = 0;
		for (int j = 0; j < PS_RULE_DIM; ++j) {
			rule.range[j][0] = rules[i]->range[j][0];
			rule.range[j][1] = rules[i]->range[j][1];
			rule.prefix_length[j] = rules[i]->prefix_len[j];
		}
		ps_rules.push_back(rule);
	}
	return ps_rules;
}

vector<PSPacket> GeneratePSPackets(vector<Trace*> &traces) {
	vector<PSPacket> packets;
	int traces_num = traces.size();
	for (int i = 0; i < traces_num; ++i) {
		PSPacket packet;
		for (int j = 0; j < PS_RULE_DIM; ++j)
			packet.push_back(traces[i]->key[j]);
		packets.push_back(packet);
	}
	return packets;
}
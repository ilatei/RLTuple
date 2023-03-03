/*
 * MIT License
 *
 * Copyright (c) 2016, 2017 by S. Yingchareonthawornchai and J. Daly at Michigan State University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include <intrin.h>
#include "elementary.h"
#include <stdio.h>
#include "dynamictuple/dynamictuple.h"
#include "RLtuple/rltuple.h"
#include "io/io.h"
#include "partitionsort/classification-main-ps.h"

#include <assert.h>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>
#include <unordered_map>
#include <set>
#include <queue>

#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib")
 
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

string float2str(float Num)
{
	ostringstream oss;
	oss << Num;
	string str(oss.str());
	return str;
}


double distribution2throughput(unordered_map<double, int>& distribution){
	int sum_weight = 0;
	double sum_cost = 0;
	for(auto key : distribution){
		sum_cost += key.first * key.second;
		sum_weight += key.second;
	}
	return sum_cost / sum_weight;
}

void dumpPolicy(RlTuple* classifier){
	for (int i = 0; i < classifier->tuples_num; ++i) {
        Tuple *tuple = classifier->tuples_arr[i];
		printf("field : ");
		for(auto field : tuple->used_field){
			printf("%d ", field);
		}
		printf("|| length : ");
		for(auto length : tuple->prefix_len){
			printf("%d ", length);
		}
        printf("|| rules : %d || priority : %d\n", tuple->rules_num, tuple->max_priority);
    }
}

map<string, string> runUpdate(Classifier* classifier, vector<Rule*>& rules, vector<Trace*> traces, map<double,int>& trace_interval, 
						int trials, ProgramState* program_state, int Flag){
	map<string, string> summary;
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	std::chrono::duration<double> elapsed_seconds;
	std::chrono::duration<double,std::milli> elapsed_milliseconds;
	bool _rltuple = true;

	// Flag 0x7F

	if(classifier->TupleNum() == 0){ //not rltuple & 0x01
		_rltuple = false;
		start = std::chrono::steady_clock::now();
		classifier->Create(rules, true);
		end = std::chrono::steady_clock::now();
		elapsed_milliseconds = end - start;
		if(Flag & 0x80)
			printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());
		summary["ConstructionTime(ms)"] = std::to_string(elapsed_milliseconds.count());
	}

	if(Flag & 0x2){
		int memSize = classifier->MemorySize();
		if(Flag & 0x80)
			printf("\tSize(bytes): %d \n", memSize);
		summary["Size(bytes)"] = std::to_string(memSize);
	}
	
	if(Flag & 0x4){
		int numTables = classifier->TupleNum();
		if(Flag & 0x80)
			printf("\tTuples: %d \n", numTables);
		summary["Tuples"] = std::to_string(numTables);
	}
	if(Flag & 0x8){
		std::chrono::duration<double> sum_time(0);
		std::vector<int> results;
		for (int t = 0; t < trials; t++) {
			start = std::chrono::steady_clock::now();
			for (auto const &t : traces) {
				if(classifier->Lookup(t, 0) == 0){
					dumpPolicy((RlTuple *)classifier);
					// exit(0);
				}
			}
			end = std::chrono::steady_clock::now();
			elapsed_seconds = end - start;
			sum_time += elapsed_seconds; 
		}
		auto throughput = 1 / (sum_time.count() / trials) * traces.size() / (1000 * 1000);
		if(Flag & 0x80)
			printf("\tLookup Throughput: %f Mpps\n", throughput);
		summary["Lookup Throughput(Mpps)"] = std::to_string(throughput);
	}

	if(Flag & 0x10){
		int lookup_tsc = (trials >= 1000)? trials : 1000;
		vector<double> lookup_times;
		for (auto const &t : traces) {
			#ifdef _WIN32
				auto s_tsc = __rdtsc();
			#else
    			unsigned int lo,hi;
    			__asm__ __volatile__ ("rdtsc" :"=a" (lo),"=d" (hi));
    			auto s_tsc = __rdtsc();((uint64_t)hi << 32) | lo;
			#endif
			for (int ep = 0; ep < lookup_tsc; ep++){
				classifier->Lookup(t, 0);
			}
			#ifdef _WIN32
				auto e_tsc = __rdtsc();
			#else
    			unsigned int lo,hi;
    			__asm__ __volatile__ ("rdtsc" :"=a" (lo),"=d" (hi));
    			auto e_tsc = __rdtsc();((uint64_t)hi << 32) | lo;
			#endif
			lookup_times.push_back(double(e_tsc - s_tsc) / lookup_tsc);
		}
		cdfLookup(lookup_times);
		auto tail_delay = lookuptimes2delay(lookup_times,trace_interval, 3000);

		if(Flag & 0x80)
			printf("\tTail delay: %f cycles\n", tail_delay);
		summary["Tail delay(cycles)"] = std::to_string(tail_delay);
	}

	if(Flag & 0x20){
		std::chrono::duration<double> sum_time(0);
		vector<int> insert_delete;
		for(int i=0; i<100000; i++){
			insert_delete.push_back((rand() * rand()) % rules.size());
		}
		for (int t = 0; t < trials; t++) {
			start = std::chrono::steady_clock::now();
			for (auto r_index : insert_delete) {
				classifier->DeleteRule(rules[r_index]);
				classifier->InsertRule(rules[r_index]);
			}
			end = std::chrono::steady_clock::now();
			elapsed_seconds = end - start;
			sum_time += elapsed_seconds; 
		}
		auto throughput = 1 / (sum_time.count() / trials) * 2 / (1000 * 1000 / 100000);
		if(Flag & 0x80)
			printf("\tUpdate Throughput: %f Mups\n", throughput);
		summary["Update Throughput(Mups)"] = std::to_string(throughput);
	}

	if(Flag & 0x40){
		for (auto const &t : traces) {
			classifier->LookupAccess(t, 0, NULL, program_state);
		}
		auto avg_tuple = (double)program_state->access_tuples.sum / traces.size();
		auto avg_rule = (double)program_state->access_rules.sum / traces.size();
		auto avg_node = (double)program_state->access_nodes.sum / traces.size();
		auto avg_table = (double)program_state->access_tables.sum / traces.size();
		auto max_tuple = program_state->access_tuples.maxn;
		auto max_rule = program_state->access_rules.maxn;
		auto max_node = program_state->access_nodes.maxn;
		auto max_table = program_state->access_tables.maxn;
		auto max_all = program_state->max_access_all;
		if(Flag & 0x80){
			printf("\tAccess avg_tuple: %f, avg_table: %f, avg_node: %f, avg_rule: %f\n", avg_tuple, avg_table, avg_node, avg_rule);
			printf("\tAccess max_tuple: %d, max_table: %d, max_node: %d, max_rule: %d, max_all: %d\n", max_tuple, max_table, max_node, max_rule, max_all);
		}
	}
	
	if (!_rltuple){
		classifier->Free(true);
	}
	return summary;
}


void RunRL(unordered_map<string, string> rlArgs, vector<Rule*>& rules, vector<Trace*> traces, int trials, 
			unordered_map<int,int> weights, map<double,int> trace_interval, vector<int> importantField) {
	
	SOCKET server_ = INVALID_SOCKET;
 
	WSADATA data_;
 
	WSAStartup(MAKEWORD(2, 2), &data_);
	server_ = socket(AF_INET, SOCK_STREAM, 0);
 
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(8888);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bind(server_, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	listen(server_, 10);
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);


	SOCKET socketConn = accept(server_, (SOCKADDR*)&addrClient, &len);

	auto classifier = new RlTuple();
	vector<Rule*> empty = vector<Rule*>{};
	set<pair<vector<uint32_t>, vector<uint32_t>>> tuples;

	vector<double> evaluate_results;
	int evaluate_results_size = 20;

	pair<vector<uint32_t>, vector<uint32_t>> current;
	classifier->Create(empty, true);

	double last_evaluate;
	double best_evaluate = 9999999;
	double best_actual = rlArgs["aim"] == "throughput"? 0:9999999;
	int actual_evaluate_interval = 0;
	int step = 0;
	int ep_step = 0;

	while(true){
		int performance_drop_cnt = 0;

		char recvBuff[1024];
		memset(recvBuff, 0, sizeof(recvBuff));
		
		recv(socketConn, recvBuff, sizeof(recvBuff), 0);
		// scanf("%s", recvBuff);
		auto f_l = StrSplit(recvBuff, "_");
		uint32_t _flag;
		uint32_t _field;
		uint32_t _length;
		if(f_l.size() != 3){
			printf("end training!");
			break;
		}
		else{
			_flag= atoi(f_l[0].c_str());
			_field = atoi(f_l[1].c_str());
			_length= atoi(f_l[2].c_str());
			// printf("actions: %d %d %d\n", _flag, _field, _length);
		}
		if(_flag == 3){
			dumpPolicy(classifier);
			ProgramState *program_state = new ProgramState();
			auto summary = runUpdate(classifier, rules, traces, trace_interval, trials, program_state, 0xEF); //计算实际的吞吐,尾时延
			free(program_state);
		}
		else if(_flag == 2){ // reset
			ep_step = 0;
			performance_drop_cnt = 0;
			tuples.clear();
			current.first.clear();
			current.second.clear();

			classifier->Free(true);
			classifier = new RlTuple();
			classifier->Create(empty, true);

			if(rlArgs["distribution"] == "no" && step == 0){
				weights.clear();
				auto weight = int(stoi(rlArgs["trace_num"]) / rules.size());
				for(auto r : rules){
					weights[r->priority] = weight;
				}
			}


			auto distribution = classifier->evaluateWithWeightEmpty(weights, rules);
			if(rlArgs["aim"] == "throughput"){ // 估计的平均lookup时间和尾时延
				last_evaluate = distribution2throughput(distribution);
			}
			else if(rlArgs["aim"] == "delay"){
				// last_evaluate = distribution2delay(distribution, trace_interval);
				last_evaluate = distribution2delay1(distribution) + distribution2throughput(distribution);
			}

			string buff;
			char temp_buf[30];

			vector<int> obs(90);
			obs[4] = rules.size();
			obs[5] = rules.size();
			// printf("reset/end epoch: obs(");
			for(auto info : obs){
				memset(temp_buf, 0, sizeof(temp_buf));
				// printf("%d ", info);
				itoa(info, temp_buf, 10);
				buff.append(temp_buf);
				buff += "_";
			}
			for(auto field : importantField){
				memset(temp_buf, 0, sizeof(temp_buf));
				// printf("%d ", info);
				itoa(field, temp_buf, 10);
				buff += temp_buf;
				buff += "_";
			}
			send(socketConn, buff.c_str(), buff.length(), 0);
		}
		else{
			step ++;
			ep_step ++;
			if(_flag == 1){ // modify current tuple
				current.first.push_back(_field);
				current.second.push_back(_length);
			}
			else if(_flag == 0){ // create new tuple
				tuples.insert(current);
				current.first.clear();
				current.second.clear();
				current.first.push_back(_field);
				current.second.push_back(_length);
			}

			classifier->Free(true);
			classifier = new RlTuple();
			classifier->Create(empty, true);
			
			for (auto p : tuples){
				classifier->addTuple(p.first, p.second);
			}
			classifier->addTuple(current.first, current.second);

			classifier->step(rules);

			double now_evaluate;
			auto distribution = classifier->evaluateWithWeight(weights, stod(rlArgs["trade_off"]));
			if(rlArgs["aim"] == "throughput"){ // 估计的平均lookup时间和尾时延
				now_evaluate = distribution2throughput(distribution);
			}
			else if(rlArgs["aim"] == "delay"){
				// now_evaluate = distribution2delay(distribution, trace_interval);
				now_evaluate = distribution2delay1(distribution) + distribution2throughput(distribution);
			}
	
			// if(step % 100 == 0){
			// 	best_evaluate *= 2;
			// }
			// if(now_evaluate < best_evaluate and step >= 1000){
			// 	if(evaluate_results.size() <= evaluate_results_size){
			// 		evaluate_results.push_back(now_evaluate);
			// 		sort(evaluate_results.begin(), evaluate_results.end());
			// 	}
			// 	else if(evaluate_results[0] >= now_evaluate / pow(1.01, actual_evaluate_interval)){
			// 		evaluate_results[evaluate_results_size - 1] = now_evaluate;
			// 		actual_evaluate_interval = 0;		
			// 		sort(evaluate_results.begin(), evaluate_results.end());
			// 		if(rlArgs["aim"] == "throughput"){
			// 			ProgramState *program_state = new ProgramState();
			// 			auto summary = runUpdate(classifier, rules, traces, trace_interval, trials, program_state, 0xEF); //计算实际的吞吐,尾时延
			// 			free(program_state);
			// 			auto now_actual_lookup = stod(summary["Lookup Throughput(Mpps)"]);
			// 			auto now_actual_update = stod(summary["Update Throughput(Mups)"]);
			// 			auto now_actual = (1- stod(rlArgs["trade_off"])) * now_actual_lookup + stod(rlArgs["trade_off"]) * now_actual_update;
			// 			if(now_actual > best_actual){
			// 				dumpPolicy(classifier);
			// 				best_actual = now_actual;
			// 				best_evaluate = now_evaluate;
			// 			}
			// 		}
			// 		else if(rlArgs["aim"] == "delay"){
			// 			ProgramState *program_state = new ProgramState();
			// 			auto summary = runUpdate(classifier, rules, traces, trace_interval, trials, program_state, 0xFF);
			// 			free(program_state);
			// 			auto now_actual = stoi(summary["Tail delay(cycles)"]);
			// 			if(now_actual < best_actual){
			// 				dumpPolicy(classifier);
			// 				best_actual = now_actual;
			// 				best_evaluate = now_evaluate;
			// 			}
			// 		}
			// 	}
			// 	else{
			// 		actual_evaluate_interval ++;
			// 	}
			// }

			string buff;
			char temp_buf[30];

			auto obs = classifier->getObs(15, current);
			// printf("obs(");
			for(auto info : obs){
				memset(temp_buf, 0, sizeof(temp_buf));
				// printf("%d ", info);
				itoa(info, temp_buf, 10);
				buff.append(temp_buf);
				buff += "_";
			}

			buff.append(float2str(last_evaluate));
			buff += "_";
			buff.append(float2str(now_evaluate));

			if(last_evaluate <= now_evaluate * 1.05){
				performance_drop_cnt ++;
			}
			else{
				performance_drop_cnt = 0;
			}
			// printf("last_evaluate:%lf, now_evaluate:%lf, best_evaluate:%lf\n", last_evaluate, now_evaluate, best_evaluate);
			last_evaluate = now_evaluate;

			if(classifier->tuples_num >= 20 || performance_drop_cnt >= 5 || ep_step >= 40){
				buff += "_1";
			}
			else{
				buff += "_0";
			}
			send(socketConn, buff.c_str(), buff.length(), 0);
		}
	}
}

int main(int argc, char* argv[]) {
	unordered_map<string, string> args = ParseArgs(argc, argv);
	string filterFile = GetOrElse(args, "f", "acl1_seed_1.rules");
	string traceFile = GetOrElse(args, "p", "");
	string algo = GetOrElse(args, "c", "RLTuple");
	int trials = stoi(GetOrElse(args, "n", "10"));
	int trace_num = stoi(GetOrElse(args, "t", "100000"));
	vector<string> algos;
	for(auto s : StrSplit(algo, ",")){
		algos.push_back(s);
	} 
	bool doShuffle = false;

	// if (doShuffle) {
	// 	rules = Random::shuffle_vector(rules);
	// }

	vector<Rule*> rules = ReadRules(filterFile, false);
	auto importanField = getImportantFiled(rules, 10);
	trace_num = trace_num > rules.size() ? trace_num :  rules.size();

	vector<Trace*> traces;
	unordered_map<int, int> rule_distribution;
	if(traceFile != ""){
		traces = ReadTraces(traceFile);
		for(auto t : traces){
			if(rule_distribution.find(t->rule_id) != rule_distribution.end()){
				rule_distribution[t->rule_id] ++;
			}
			else{
				rule_distribution[t->rule_id] = 1;
			}
		}
		trace_num = traces.size();
	}
	else{
		pair<pair<vector<Trace*>,vector<Trace*>>, unordered_map<int, int>> traces_all;
		if(GetOrElse(args, "d", "no") == "no"){
			traces_all = GenerateTraces(rules, trace_num);
		}
		else{
			traces_all = GenerateTraces(rules, trace_num, true);
		}
		
		// auto tracePerRule = traces_all.first.second;
		traces = traces_all.first.first;
		rule_distribution = traces_all.second;
		auto traceFileName = filterFile;
		filterFile.append("_traces");
		SaveTraces(traces, filterFile.c_str());

		trace_num = traces.size();
	}

	auto trace_interval = getWeibullInterval(trace_num);

	for(auto algo : algos){
		CommandStruct cc;
		ProgramState *program_state = new ProgramState();
		vector<int> ans;
		printf("\nalgo:%s\n", algo.c_str());
		if(algo == "PSTSS" || algo == "TupleMerge" || algo == "PartitionSort"){
			cc.method_name = algo;
			ClassificationMainPS(cc, program_state, rules, traces, trace_interval, ans, trials);
		}
		else if(algo == "DynamicTuple"){
			auto cc = new DynamicTuple();
			if(GetOrElse(args, "a", "throughput") == "delay"){
				runUpdate(cc, rules, traces, trace_interval, trials, program_state, 0xFF);
			}
			else{
				runUpdate(cc, rules, traces, trace_interval, trials, program_state, 0xEF);
			}
		}
		else if(algo == "RLTuple"){
			unordered_map<string, string> rlArgs;
			rlArgs["aim"] = GetOrElse(args, "a", "throughput");
			rlArgs["distribution"] = GetOrElse(args, "d", "no");
			rlArgs["trace_num"] = to_string(trace_num);
			rlArgs["trade_off"] = GetOrElse(args, "to", "0.0");
			RunRL(rlArgs, rules, traces, trials, rule_distribution, trace_interval, importanField);
		}
		free(program_state);
	}
	
	return 0;
}



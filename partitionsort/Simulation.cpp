#include "Simulation.h"

using namespace std;

void cdfLookup(vector<double> lookup_distribution){
	sort(lookup_distribution.begin(), lookup_distribution.end());
	for(int i = 1; i<= 100; i++){
		printf("%lf ", lookup_distribution[(lookup_distribution.size() - 1) * i / 100]);
	}
	printf("\n");
}


double lookuptimes2delay(vector<double>& lookup_times, map<double, int>& trace_interval, double cyclePerPacket){
	vector<double> interval_times;
	double interval_sum;
	double lookup_time_sum;
	for(auto key : trace_interval){
		for(int i=0; i<key.second; i++){
			interval_times.push_back(key.first);
			interval_sum += key.first;
		}
	}
	for(auto key : lookup_times){
		lookup_time_sum += key;
	}
	if(cyclePerPacket == 0){
		double _ratio = (lookup_time_sum / lookup_times.size()) / (interval_sum / interval_times.size()) * (1 / 0.5);
		for(int i = 0; i < interval_times.size(); i++){
			interval_times[i] *= _ratio;
		}
	}
	else{
		double _ratio = cyclePerPacket / (interval_sum / interval_times.size());
		for(int i = 0; i < interval_times.size(); i++){
			interval_times[i] *= _ratio;
		}
	}

	double now_time = 0;
	queue<pair<double, double>> packet_process; //in_queue, process
	for(auto lookup_time : lookup_times){
		packet_process.push(make_pair(now_time, lookup_time));
		now_time += interval_times[((uint32_t)rand() << 16 | rand()) % interval_times.size()];
	}
	now_time = 0;
	double process_time = 0;
	double waiting_time = 0;
	double packet_delay = 0;
	priority_queue<double, vector<double>, greater<double>> max_delays;
	while(packet_process.size() != 0){
		auto packet = packet_process.front();
		if(now_time < packet.first){
			now_time = packet.first;
		}
		waiting_time = now_time - packet.first;
		now_time += packet.second;
		process_time = packet.second;
		packet_process.pop();
		packet_delay = waiting_time + process_time;
		max_delays.push(packet_delay);
		if(max_delays.size() > (1 - 0.95) * lookup_times.size()){
			max_delays.pop();
		}
	}
	// return (process_time + waiting_time) / lookup_times.size();
	return max_delays.top();
}

double distribution2delay(unordered_map<double, int> &distribution, map<double, int>& trace_interval){
	vector<double> lookup_times;
	for(auto key : distribution){
		for(int i=0; i<key.second; i++)
			lookup_times.push_back(key.first);
	}
	lookup_times = Random::shuffle_vector(lookup_times);
	return lookuptimes2delay(lookup_times, trace_interval);
}

double distribution2delay1(unordered_map<double, int> &distribution){
	vector<double> lookup_times;
	for(auto key : distribution){
		for(int i=0; i<key.second; i++)
			lookup_times.push_back(key.first);
	}
	priority_queue<double, vector<double>, greater<double>> max_delays;
	for(auto look_uptime: lookup_times){
		if(max_delays.size() > (1 - 0.95) * lookup_times.size()){
			if(look_uptime > max_delays.top()){
				max_delays.pop();
				max_delays.push(look_uptime);
			}
		}
		else{
			max_delays.push(look_uptime);
		}
	}
	return max_delays.top();
}


vector<int> PerformOnlyPacketClassification(PacketClassifier& classifier, vector<PSRule> &rules, vector<PSPacket> &packets, 
										map<double,int> &trace_interval, int trials, ProgramState *program_state) {

	std::map<std::string, std::string> summary;
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	std::chrono::duration<double> elapsed_seconds;
	std::chrono::duration<double,std::milli> elapsed_milliseconds;

	start = std::chrono::steady_clock::now();
	classifier.ConstructClassifier(rules);
	end = std::chrono::steady_clock::now();
	elapsed_milliseconds = end - start;
	printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());
	summary["ConstructionTime(ms)"] = std::to_string(elapsed_milliseconds.count());

	int memSize = classifier.MemSizeBytes();
	printf("\tSize(bytes): %d \n", memSize);
	summary["Size(bytes)"] = std::to_string(memSize);
	
	int numTables = classifier.NumTables();
	printf("\tTuples: %d \n", numTables);
	summary["Tuples"] = std::to_string(numTables);


	std::chrono::duration<double> sum_time(0);
	std::vector<int> results;
	for (int t = 0; t < trials; t++) {
		results.clear();
		start = std::chrono::steady_clock::now();
		for (auto const &p : packets) {
			results.push_back(classifier.ClassifyAPacket(p));
		}
		end = std::chrono::steady_clock::now();
		elapsed_seconds = end - start;
		sum_time += elapsed_seconds; 
	}
	for(auto rr : results){
		if(rr == 0){
			printf("err match!\n");
		}
	}

	auto throughput = 1 / (sum_time.count() / trials) * packets.size() / (1000 * 1000);
	printf("\tLookup Throughput: %f Mpps\n", throughput);
	summary["Lookup Throughput(Mpps)"] = std::to_string(throughput);

	// vector<double> lookup_times;
	// int lookup_tsc = (trials >= 1000)? trials : 1000;
	// for (auto const &p : packets) {
	// 	#ifdef _WIN32
	// 		auto s_tsc = __rdtsc();
	// 	#else
    // 		unsigned int lo,hi;
    // 		__asm__ __volatile__ ("rdtsc" :"=a" (lo),"=d" (hi));
    // 		auto s_tsc = __rdtsc();((uint64_t)hi << 32) | lo;
	// 	#endif
	// 	for (int ep = 0; ep < lookup_tsc; ep++){
	// 		classifier.ClassifyAPacket(p);
	// 	}
	// 	#ifdef _WIN32
	// 		auto e_tsc = __rdtsc();
	// 	#else
    // 		unsigned int lo,hi;
    // 		__asm__ __volatile__ ("rdtsc" :"=a" (lo),"=d" (hi));
    // 		auto e_tsc = __rdtsc();((uint64_t)hi << 32) | lo;
	// 	#endif
	// 	lookup_times.push_back(double(e_tsc - s_tsc) / lookup_tsc);
	// }
	// cdfLookup(lookup_times);
	// auto tail_delay = lookuptimes2delay(lookup_times, trace_interval, 3000);

	// printf("\tTail delay: %f cycles\n", tail_delay);
	// summary["Tail delay(cycles)"] = std::to_string(tail_delay);

	std::chrono::duration<double> sum_time1(0);
	for (int t = 0; t < trials; t++) {
		start = std::chrono::steady_clock::now();
		for (auto const &r : rules) {
			classifier.DeleteRule(r);
			classifier.InsertRule(r);
		}
		end = std::chrono::steady_clock::now();
		elapsed_seconds = end - start;
		sum_time1 += elapsed_seconds; 
	}
	throughput = 1 / (sum_time1.count() / trials) * rules.size() * 2 / (1000 * 1000);
	printf("\tUpdate Throughput: %f Mups\n", throughput);
	summary["Update Throughput(Mups)"] = std::to_string(throughput);

	for (auto const &p : packets) {
		classifier.ClassifyAPacketAccess(p, program_state);
	}
	auto avg_tuple = (double)program_state->access_tuples.sum / packets.size();
	auto avg_rule = (double)program_state->access_rules.sum / packets.size();
	auto avg_node = (double)program_state->access_nodes.sum / packets.size();
	auto avg_table = (double)program_state->access_tables.sum / packets.size();
	auto max_tuple = program_state->access_tuples.maxn;
	auto max_rule = program_state->access_rules.maxn;
	auto max_node = program_state->access_nodes.maxn;
	auto max_table = program_state->access_tables.maxn;
	auto max_all = program_state->max_access_all;
	printf("\tAccess avg_tuple: %f, avg_table: %f, avg_node: %f, avg_rule: %f\n", avg_tuple, avg_table, avg_node, avg_rule);
	printf("\tAccess max_tuple: %d, max_table: %d, max_node: %d, max_rule: %d, max_all: %d\n", max_tuple, max_table, max_node, max_rule, max_all);

	return results;
}
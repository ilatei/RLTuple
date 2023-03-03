#ifndef  SIMULATION_H
#define  SIMULATION_H

// #include "../elementary.h"
#include <intrin.h>
#include "ElementaryClasses.h"
// #include "Utilities/MapExtensions.h"

#include <map>
#include <unordered_map>

using namespace std;


vector<int> PerformOnlyPacketClassification(PacketClassifier& classifier, vector<PSRule> &rules, vector<PSPacket> &packets, 
                                        map<double,int> &trace_interval, int trials, ProgramState *program_state);

double lookuptimes2delay(vector<double>& lookup_times, map<double, int>& trace_interval, double bandwith=0);
double distribution2delay(unordered_map<double, int> &distribution, map<double, int>& trace_interval);
double distribution2delay1(unordered_map<double, int> &distribution);
void cdfLookup(vector<double> lookup_distribution);

#endif
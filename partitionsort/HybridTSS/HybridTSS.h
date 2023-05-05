//
// Created by GIGABYTE on 2022/3/2.
//

#ifndef HYBRIDTSSV1_2_HYBRIDTSS_H
#define HYBRIDTSSV1_2_HYBRIDTSS_H
#include "SubHybridTSS.h"
using namespace std;
struct Node {
    int a, b;
    Node(int _a, int _b) {
        a = _a;
        b = _b;
    }
};
class HybridTSS : public PacketClassifier {
public:
    HybridTSS();
    void ConstructClassifier(const std::vector<PSRule> &rules) override;

    int ClassifyAPacket(const PSPacket& packet) override;
    void DeleteRule(const PSRule& rule) override;
    void InsertRule(const PSRule& rule) override;
    Memory MemSizeBytes() const override;
    int MemoryAccess() const override;
    size_t NumTables() const override;
    size_t RulesInTable(size_t tableIndex) const override;

    virtual int ClassifyAPacketAccess(const PSPacket& one_packet, ProgramState *program_state);
    virtual int CalculateState(ProgramState *program_state);

    string prints() {
        return "";
    }
    vector<Node *> test() {
        vec.push_back(new Node(1, 2));
        vec.push_back(new Node(3, 4));
        return vec;
    }
    void printVec() {
        for (auto iter : vec) {
            cout << iter->a << "\t" << iter->b << endl;
        }
    }
    void printInfo();
    vector<int> getAction(SubHybridTSS *state, int epsilion);
    void ConstructBaseline(const vector<PSRule> &rules);


private:
    vector<Node*> vec;
    int binth = 8;
    SubHybridTSS *root;
//    int rtssleaf = 2;
    double rtssleaf = 1.5;
    vector<vector<double> > QTable;

    void train(const vector<PSRule> &rules);



};
#endif //HYBRIDTSSV1_2_HYBRIDTSS_H

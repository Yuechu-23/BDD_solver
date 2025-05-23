#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "cuddObj.hh"

int main(int argc, char* argv[]) {

    std::string filename = argv[1];
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr<<"Cannot open "<<argv[1]<<"\n";
        return 1;
    }

    std::string header;
    fin >> header;
    int M, I, L, O, A;
    fin >> M >> I >> L >> O >> A;

    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    std::vector<DdNode*> bdd_vars(M+1,nullptr);
    for(int i = 0; i < I; ++i) {
        int input;
        fin >> input;
        int idx = input / 2;
        bdd_vars[idx] = Cudd_bddIthVar(mgr, idx);
        Cudd_Ref(bdd_vars[idx]);
    }
    //L = 0
    //O = 1
    int output;
    fin >> output;

    Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);

    for(int i = 0;i < A; ++i){
        int lhs, rhs0, rhs1;
        fin >> lhs >> rhs0 >> rhs1;
        int id_lhs = lhs / 2;
        
        DdNode* f0 = bdd_vars[rhs0/2];
        if (rhs0 & 1) f0 = Cudd_Not(f0);
        DdNode* f1 = bdd_vars[rhs1/2];
        if (rhs1 & 1) f1 = Cudd_Not(f1);
        
        DdNode* tmp = Cudd_bddAnd(mgr, f0, f1);
        Cudd_Ref(tmp);
        
        if (bdd_vars[id_lhs] != nullptr) {
            Cudd_RecursiveDeref(mgr, bdd_vars[id_lhs]);
        }
        bdd_vars[id_lhs] = tmp;
    }

    DdNode* output_bdd = bdd_vars[output/2];
    if (output & 1) output_bdd = Cudd_Not(output_bdd);
    Cudd_Ref(output_bdd);

    Cudd_AutodynDisable(mgr);
    Cudd_ReduceHeap(mgr, CUDD_REORDER_SIFT, 0);

    // 打印输出BDD信息
    std::cout << "Output BDD for variable " << output << ":" << Cudd_DagSize(output_bdd) << std::endl;

    // 减少引用计数
    for(auto& bdd_var : bdd_vars) {
        if (bdd_var != nullptr) {
            Cudd_RecursiveDeref(mgr, bdd_var);
        }
    }
    Cudd_Quit(mgr);
    return 0;
}

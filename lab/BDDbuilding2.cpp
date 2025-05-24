#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "cuddObj.hh"
#include "cuddInt.h"

std::unordered_map<DdNode*, __float128> node_odd_cnt;
std::unordered_map<DdNode*, __float128> node_even_cnt;

std::pair<__float128,__float128> countPaths(DdManager* mgr, DdNode* n){
    auto it = node_even_cnt.find(n);
    if(it != node_even_cnt.end()){
        return std::make_pair(node_odd_cnt[n], node_even_cnt[n]);
    }
    if(Cudd_IsConstant(n)){
        return Cudd_IsComplement(n) ?
            std::pair<__float128,__float128>{0, 0} :
            std::pair<__float128,__float128>{0, 1};
    }

    DdNode* real = Cudd_Regular(n);
    bool is_complement = Cudd_IsComplement(n);
    DdNode* t = Cudd_T(real);
    DdNode* e = Cudd_E(real);
    if(is_complement){
        t = Cudd_Not(t);
        e = Cudd_Not(e);
    }
    auto [odd_t, even_t] = countPaths(mgr, t);
    auto [odd_e, even_e] = countPaths(mgr, e);
    __float128 odd = 0, even = 0;

    if(Cudd_IsComplement(t)){
        odd += even_t;
        even += odd_t;
    } else {
        odd += odd_t;
        even += even_t;
    }
    if(Cudd_IsComplement(e)){
        odd += even_e;
        even += odd_e;
    } else {
        odd += odd_e;
        even += even_e;
    }

    node_odd_cnt[n] = odd;
    node_even_cnt[n] = even;
    return std::make_pair(odd, even);
}

void DFS(DdNode* x,bool odd,std::vector<int>& path){
    
    if(Cudd_IsConstant(x))
        return;
    
    DdNode* real = Cudd_Regular(x);
    int var_idx = Cudd_NodeReadIndex(real);
    bool is_complement = Cudd_IsComplement(x);
    DdNode* t = Cudd_T(real);
    DdNode* e = Cudd_E(real);
    if(is_complement){
        t = Cudd_Not(t);
        e = Cudd_Not(e);
    }

    bool odd_left = odd ^ Cudd_IsComplement(t);
    bool odd_right = odd ^ Cudd_IsComplement(e);
    __float128 cnt_left = odd_left ? node_odd_cnt[t] : node_even_cnt[t];
    __float128 cnt_right = odd_right ? node_odd_cnt[e] : node_even_cnt[e];
    double prob_left = (double)(cnt_left / (cnt_left + cnt_right));
    if((double)rand() / RAND_MAX < prob_left){
        path.push_back(var_idx);
        DFS(t, odd_left, path);
    } else {
        path.push_back(-var_idx);
        DFS(e, odd_right, path);
    }
}

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

    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT);

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

    countPaths(mgr, output_bdd);
    std::vector<int> path;
    DFS(output_bdd, Cudd_IsComplement(output_bdd), path);

    sort(path.begin(), path.end(), [](int a, int b) {
        return abs(a) < abs(b);
    });
    for(int var: path){
        bool val = var > 0;
        std::cout << "x" << var << " = " << (val ? "1" : "0") << "\n";
    }
    
    for(auto& bdd_var : bdd_vars) {
        if (bdd_var != nullptr) {
            Cudd_RecursiveDeref(mgr, bdd_var);
        }
    }
    Cudd_Quit(mgr);
    return 0;
}

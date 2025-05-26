#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cassert>
#include "cuddObj.hh"
#include "cuddInt.h"

std::unordered_map<DdNode*, __float128> node_odd_cnt;
std::unordered_map<DdNode*, __float128> node_even_cnt;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<double> dis(0.0, 1.0);

std::pair<__float128,__float128> countPaths(DdManager* mgr, DdNode* n){
    
    auto it = node_even_cnt.find(n);
    if(it != node_even_cnt.end()){
        return std::make_pair(node_odd_cnt[n], node_even_cnt[n]);
    }

    if(Cudd_IsConstant(n)){
        __float128 odd = 0, even = 0;
        if(Cudd_IsComplement(n)) odd = 1;
        else even = 1;
        node_odd_cnt[n] = odd;
        node_even_cnt[n] = even;
        return std::make_pair(odd, even);
    }

    DdNode* real = Cudd_Regular(n);
    bool is_complement = Cudd_IsComplement(n);
    DdNode* t = Cudd_T(real);
    DdNode* e = Cudd_E(real);

    auto [odd_t, even_t] = countPaths(mgr, t);
    auto [odd_e, even_e] = countPaths(mgr, e);
    __float128 odd = odd_t + odd_e;
    __float128 even = even_t + even_e;
    if(is_complement)
        std::swap(odd, even);
    node_odd_cnt[n] = odd;
    node_even_cnt[n] = even;
    return std::make_pair(odd, even);
}

void DFS(DdNode* x,int odd,std::vector<int>& path){
    
    if(Cudd_IsConstant(x)){
        assert(!odd);
        return;
    }
    DdNode* real = Cudd_Regular(x);
    int var_idx = Cudd_NodeReadIndex(real);
    DdNode* t = Cudd_T(real);
    DdNode* e = Cudd_E(real);
   
    int odd_left = odd ^ Cudd_IsComplement(t);
    int odd_right = odd ^ Cudd_IsComplement(e);
    __float128 cnt_left = odd ? node_odd_cnt[t] : node_even_cnt[t];
    __float128 cnt_right = odd ? node_odd_cnt[e] : node_even_cnt[e];
    double prob_left = 0.5;
    if(cnt_left + cnt_right > 0)
        prob_left = static_cast<double>(cnt_left) / (cnt_left + cnt_right);
    
    if(dis(gen) < prob_left){
        path.push_back(var_idx);
        DFS(t, odd_left, path);
    } else {
        path.push_back(-var_idx);
        DFS(e, odd_right, path);
    }
}

int main(int argc, char* argv[]) {

    std::string filename = argv[1];
    std::string num_samples_str = argv[2];
    int num_samples = std::stoi(num_samples_str);
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
        bdd_vars[idx] = Cudd_bddIthVar(mgr,idx);
        Cudd_Ref(bdd_vars[idx]);
    }
    //L = 0
    //O = 1
    int output_idx;
    fin >> output_idx;

    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT);

    for(int i = 0;i < A; ++i){
        int lhs, rhs0, rhs1;
        fin >> lhs >> rhs0 >> rhs1;
        int id_lhs = lhs / 2;
        
        DdNode* f0 = bdd_vars[rhs0 / 2];
        if (rhs0 & 1) f0 = Cudd_Not(f0);
        DdNode* f1 = bdd_vars[rhs1 / 2];
        if (rhs1 & 1) f1 = Cudd_Not(f1);
        
        bdd_vars[id_lhs] = Cudd_bddAnd(mgr, f0, f1);
        Cudd_Ref(bdd_vars[id_lhs]);
    }
    
    DdNode* output_bdd = bdd_vars[output_idx / 2];
    if (output_idx & 1) output_bdd = Cudd_Not(output_bdd);
    Cudd_Ref(output_bdd);
    
    Cudd_AutodynDisable(mgr);
    Cudd_ReduceHeap(mgr, CUDD_REORDER_SIFT, 0);

    countPaths(mgr, output_bdd);
    for(int i=0;i<num_samples;i++){
        gen.seed(rd() + i);
        std::vector<int> path;
        DFS(output_bdd, Cudd_IsComplement(output_bdd), path);
        for(int var: path) std::cout << var <<" ";
        std::cout << std::endl; 
    }
    
    for(auto& bdd_var : bdd_vars) 
        if (bdd_var != nullptr) 
            Cudd_RecursiveDeref(mgr, bdd_var);
        
    Cudd_Quit(mgr);
    return 0;
}

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cassert>
#include <queue>
#include "cuddObj.hh"
#include "cuddInt.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::unordered_map<DdNode*, __float128> node_odd_cnt;
std::unordered_map<DdNode*, __float128> node_even_cnt;

static std::mt19937 gen;
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
        path[var_idx] = 1;
        DFS(t, odd_left, path);
    } else {
        path[var_idx] = 0;
        DFS(e, odd_right, path);
    }
}

struct AND{
    int lhs;
    int rhs0;
    int rhs1;
    AND(int lhs, int rhs0, int rhs1) : lhs(lhs), rhs0(rhs0), rhs1(rhs1) {}
};

void topologicalSort(int node, const std::vector<std::vector<int>>& graph, std::vector<int>& visited, std::vector<int>& order) {
    visited[node] = 1;
    for(int to : graph[node]) 
        if(!visited[to]) 
            topologicalSort(to, graph, visited, order);
    order.push_back(node);
}


int main(int argc, char* argv[]) {

    //input
    if(argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <aig_file> <num_samples> <seed> <bitwidth_file> <output_file>\n";
        return 1;
    }
    std::string aig_filename = argv[1];
    int num_samples = std::stoi(argv[2]);
    unsigned seed = std::stoul(argv[3]);
    std::string bitwidth_filename = argv[4];
    std::string output_filename = argv[5];
    
    //aig input
    std::ifstream aig_fin(aig_filename);
    if (!aig_fin) {
        std::cerr<<"Cannot open "<<aig_filename<<"\n";
        return 1;
    }
    
    std::string header;
    aig_fin >> header;
    int M, I, L, O, A;
    aig_fin >> M >> I >> L >> O >> A;

    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS * 2, CUDD_CACHE_SLOTS * 2, 0);

    
    //aig input 
    std::vector<DdNode*> bdd_vars(M+1,nullptr);
    for(int i = 0; i < I; ++i) {
        int input;
        aig_fin >> input;
    }
    //L = 0
    //O = 1
    int output_idx;
    aig_fin >> output_idx;

    
    std::vector<AND> and_gates;
    std::vector<std::vector<int>> graph(M+1);
    std::vector<int> visited(M+1,0);
    std::vector<int> order;
    for(int i = 0;i < A;i++){
        int lhs, rhs0, rhs1;
        aig_fin >> lhs >> rhs0 >> rhs1;
        and_gates.emplace_back(lhs, rhs0, rhs1);
        graph[lhs / 2].push_back(rhs0 / 2);
        graph[lhs / 2].push_back(rhs1 / 2);
    }
    topologicalSort(output_idx/2, graph, visited, order);
    std::vector<int> perm;
    for(auto num: order) 
        if(num <= I) 
            perm.push_back(num);

    for(auto i: perm){
        bdd_vars[i] = Cudd_bddIthVar(mgr, i);
        Cudd_Ref(bdd_vars[i]);
    }
    
    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT);
    //aig AND gates
    for(const auto& gate : and_gates){
        int lhs = gate.lhs;
        int rhs0 = gate.rhs0;
        int rhs1 = gate.rhs1;
        int id_lhs = lhs / 2;
        bdd_vars[id_lhs] = Cudd_bddAnd(mgr,
            (rhs0 & 1) ? Cudd_Not(bdd_vars[rhs0/2]): bdd_vars[rhs0/2],
            (rhs1 & 1) ? Cudd_Not(bdd_vars[rhs1/2]): bdd_vars[rhs1/2]);
        Cudd_Ref(bdd_vars[id_lhs]);
    }

    aig_fin.close();

    DdNode* output_bdd = bdd_vars[output_idx / 2];
    if (output_idx & 1) output_bdd = Cudd_Not(output_bdd);
    Cudd_Ref(output_bdd);

    Cudd_AutodynDisable(mgr);
    //Cudd_ReduceHeap(mgr, CUDD_REORDER_SIFT, 0);
    
    //get bit widths from txt
    std::vector<int> bitwidths;
    std::ifstream bitwidth_fin(bitwidth_filename);
    int width;
    if (!bitwidth_fin) {
        std::cerr<<"Cannot open "<<bitwidth_filename<<"\n";
        return 1;
    }
    while(bitwidth_fin >> width) 
        bitwidths.push_back(width);
    bitwidth_fin.close();

    //Generate random paths
    std::vector<std::vector<std::vector<int>>> results_binary(num_samples, std::vector<std::vector<int>>(bitwidths.size()));
    
    countPaths(mgr, output_bdd);
    for(int i = 0; i < num_samples; i++){
        gen.seed(seed + i);
        for(int j = 0; j < bitwidths.size(); j++) 
            results_binary[i][j].resize(bitwidths[j], 0);
        std::vector<int> path(I + 1, 0);
        DFS(output_bdd, Cudd_IsComplement(output_bdd), path);
        int cnt = 1;
        for(int j = 0;j < bitwidths.size(); j++)
            for(int k = 0;k < bitwidths[j];k++)
                results_binary[i][j][k] = path[cnt++];
    }

    for(auto& bdd_var : bdd_vars) 
        if (bdd_var != nullptr) 
            Cudd_RecursiveDeref(mgr, bdd_var);
        
    Cudd_Quit(mgr);

    //convert results to hex and write to json
    json output;
    json assignment_list = json::array();
    for(int i = 0; i < num_samples; i++){
        json sample = json::array();
        for(int j = 0; j < bitwidths.size(); j++){
            std::string hex_value;
            int bits_left = bitwidths[j];
            int hex_digit = 0;
            int bit_pos = 0;
            for(int k = 0; k < bitwidths[j]; k++){
                hex_digit |= (results_binary[i][j][k] << bit_pos);
                bit_pos++;
                bits_left--;
                if(bit_pos == 4 || bits_left == 0) {
                    char hex_char;
                    if(hex_digit < 10)
                        hex_char = '0' + hex_digit;
                    else
                        hex_char = 'a' + (hex_digit - 10);
                    hex_value = hex_char + hex_value;
                    hex_digit = 0;
                    bit_pos = 0;
                }
            }
            json value_obj;
            value_obj["value"] = hex_value;
            sample.push_back(value_obj);
        }
        assignment_list.push_back(sample);
    }
    output["assignment_list"] = assignment_list;
    std::ofstream output_fout(output_filename);
    if (!output_fout) {
        std::cerr<<"Cannot open "<<output_filename<<"\n";
        return 1;
    }
    output_fout << output.dump(4);
    output_fout.close();
    return 0;
}

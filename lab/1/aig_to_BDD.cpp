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
#include "json.hpp"

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
        path.push_back(var_idx);
        DFS(t, odd_left, path);
    } else {
        path.push_back(-var_idx);
        DFS(e, odd_right, path);
    }
}

int main(int argc, char* argv[]) {

    std::string aig_filename = argv[1];
    int num_samples = std::stoi(argv[2]);
    unsigned seed = std::stoul(argv[3]);
    std::string bitwidth_filename = argv[4];
    std::string output_filename = argv[5];

    gen.seed(seed);
    
    std::ifstream aig_fin(aig_filename);
    if (!aig_fin) {
        std::cerr<<"Cannot open "<<aig_filename<<"\n";
        return 1;
    }
    
    std::string header;
    aig_fin >> header;
    int M, I, L, O, A;
    aig_fin >> M >> I >> L >> O >> A;

    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    std::vector<DdNode*> bdd_vars(M+1,nullptr);
    for(int i = 0; i < I; ++i) {
        int input;
        aig_fin >> input;
        int idx = input / 2;
        bdd_vars[idx] = Cudd_bddIthVar(mgr,idx);
        Cudd_Ref(bdd_vars[idx]);
    }
    //L = 0
    //O = 1
    int output_idx;
    aig_fin >> output_idx;

    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT);

    for(int i = 0;i < A; ++i){
        int lhs, rhs0, rhs1;
        aig_fin >> lhs >> rhs0 >> rhs1;
        int id_lhs = lhs / 2;
        
        DdNode* f0 = bdd_vars[rhs0 / 2];
        if (rhs0 & 1) f0 = Cudd_Not(f0);
        DdNode* f1 = bdd_vars[rhs1 / 2];
        if (rhs1 & 1) f1 = Cudd_Not(f1);
        
        bdd_vars[id_lhs] = Cudd_bddAnd(mgr, f0, f1);
        Cudd_Ref(bdd_vars[id_lhs]);
    }

    aig_fin.close();

    DdNode* output_bdd = bdd_vars[output_idx / 2];
    if (output_idx & 1) output_bdd = Cudd_Not(output_bdd);
    Cudd_Ref(output_bdd);
    
    Cudd_AutodynDisable(mgr);
    Cudd_ReduceHeap(mgr, CUDD_REORDER_SIFT, 0);

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

    std::vector<std::vector<std::vector<int>>> results_binary(num_samples, std::vector<std::vector<int>>(bitwidths.size()));
    
    countPaths(mgr, output_bdd);
    for(int i = 0; i < num_samples; i++){
        for(int j = 0; j < bitwidths.size(); j++) 
            results_binary[i][j].resize(bitwidths[j], 0);
        std::vector<int> path(I + 1, 0);
        DFS(output_bdd, Cudd_IsComplement(output_bdd), path);
        for(int var: path)
            if(var > 0) path[var] = 1;
        int cnt = 1;
        for(int j = 0;j < bitwidths.size(); j++)
            for(int k = 0;k < bitwidths[j];k++)
                results_binary[i][j][k] = path[cnt++];
    }

    for(auto& bdd_var : bdd_vars) 
        if (bdd_var != nullptr) 
            Cudd_RecursiveDeref(mgr, bdd_var);
        
    Cudd_Quit(mgr);

    json output;
    json assignment_list = json::array();
    for(int i = 0; i <num_samples; i++){
        json sample = json::array();
        for(int j = 0; j < bitwidths.size(); j++){
            std::string hex_value;
            int bits_left = bitwidths[j];
            int hex_digit = 0;
            int bit_pos = 0;
            for(int k = 0; k < bitwidths[j]; k++){
                hex_digit = (hex_digit << 1) | results_binary[i][j][k];
                bit_pos++;
                bits_left--;
            
                // 每4位或到达最后一组位时，转换为十六进制
                if(bit_pos == 4 || bits_left == 0) {
                // 如果不足4位，左移到最高位
                    if(bit_pos < 4) {
                        hex_digit <<= (4 - bit_pos);
                    }
                // 转换为十六进制字符
                    char hex_char;
                    if(hex_digit < 10)
                        hex_char = '0' + hex_digit;
                    else
                        hex_char = 'a' + (hex_digit - 10);
                
                    hex_value += hex_char;
                    hex_digit = 0;
                    bit_pos = 0;
                }
            }
            if(hex_value.length() > 1) {
                size_t first_non_zero = hex_value.find_first_not_of('0');
                if(first_non_zero != std::string::npos) {
                    hex_value = hex_value.substr(first_non_zero);
                } else {
                    hex_value = "0";  // 全是0的情况
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

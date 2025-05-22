#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include "./third_parties/cudd.h"

using namespace std;

// 解析AIGER文件
bool parseAigerFile(const string& filename, 
                   int& max_var_index,
                   int& num_inputs,
                   int& num_latches,
                   int& num_outputs,
                   int& num_ands,
                   vector<int>& inputs,
                   vector<int>& outputs,
                   vector<vector<int>>& and_gates) {
    
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return false;
    }

    string header;
    file >> header;
    if (header != "aag") {
        cerr << "Error: Not a valid AIGER file" << endl;
        return false;
    }

    file >> max_var_index >> num_inputs >> num_latches >> num_outputs >> num_ands;

    // 读取输入变量
    inputs.resize(num_inputs);
    for (int i = 0; i < num_inputs; ++i) {
        file >> inputs[i];
    }

    // 读取锁存器(本示例中忽略)
    for (int i = 0; i < num_latches; ++i) {
        int latch;
        file >> latch; // 忽略锁存器
    }

    // 读取输出
    outputs.resize(num_outputs);
    for (int i = 0; i < num_outputs; ++i) {
        file >> outputs[i];
    }

    // 读取与门
    and_gates.resize(num_ands);
    for (int i = 0; i < num_ands; ++i) {
        int lhs, rhs0, rhs1;
        file >> lhs >> rhs0 >> rhs1;
        and_gates[i] = {lhs, rhs0, rhs1};
    }

    file.close();
    return true;
}

// 将AIGER转换为BDD
DdNode* aigerToBdd(DdManager* ddMgr, 
                  const vector<int>& inputs,
                  const vector<vector<int>>& and_gates,
                  int output_var,
                  unordered_map<int, DdNode*>& var_to_bdd) {
    
    // 检查是否已缓存
    if (var_to_bdd.find(output_var) != var_to_bdd.end()) {
        return var_to_bdd[output_var];
    }

    // 处理常量
    if (output_var == 0 || output_var == 1) {
        DdNode* cnst = (output_var == 1) ? Cudd_ReadOne(ddMgr) : Cudd_ReadLogicZero(ddMgr);
        Cudd_Ref(cnst);
        var_to_bdd[output_var] = cnst;
        return cnst;
    }

    // 处理取反
    bool is_negated = (output_var & 1);
    int base_var = is_negated ? output_var - 1 : output_var;

    // 检查输入变量
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i] == base_var) {
            DdNode* bddVar = Cudd_bddIthVar(ddMgr, i);
            DdNode* result = is_negated ? Cudd_Not(bddVar) : bddVar;
            Cudd_Ref(result);
            var_to_bdd[output_var] = result;
            return result;
        }
    }

    // 检查与门
    for (const auto& gate : and_gates) {
        if (gate[0] == base_var) {
            DdNode* left = aigerToBdd(ddMgr, inputs, and_gates, gate[1], var_to_bdd);
            DdNode* right = aigerToBdd(ddMgr, inputs, and_gates, gate[2], var_to_bdd);
            
            DdNode* and_bdd = Cudd_bddAnd(ddMgr, left, right);
            Cudd_Ref(and_bdd);
            
            DdNode* result = is_negated ? Cudd_Not(and_bdd) : and_bdd;
            Cudd_Ref(result);
            
            var_to_bdd[output_var] = result;
            Cudd_RecursiveDeref(ddMgr, and_bdd);
            return result;
        }
    }

    // 错误处理
    cerr << "Missing variable definition for: " << output_var << endl;
    cerr << "Base var: " << base_var << " Negated: " << is_negated << endl;
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <aiger_file.aag>" << endl;
        return 1;
    }

    string filename = argv[1];
    
    // 解析AIGER文件
    int max_var_index, num_inputs, num_latches, num_outputs, num_ands;
    vector<int> inputs, outputs;
    vector<vector<int>> and_gates;
    
    if (!parseAigerFile(filename, max_var_index, num_inputs, num_latches, 
                       num_outputs, num_ands, inputs, outputs, and_gates)) {
        return 1;
    }

    // 初始化CUDD管理器
    DdManager* ddMgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    if (!ddMgr) {
        cerr << "Error: Could not initialize CUDD manager" << endl;
        return 1;
    }

    // 为每个输入变量创建BDD变量
    for (int i = 0; i < num_inputs; ++i) {
        Cudd_bddIthVar(ddMgr, i);
    }

    // 存储变量到BDD的映射
    unordered_map<int, DdNode*> var_to_bdd;

    // 为每个输出构建BDD
    for (int output : outputs) {
        DdNode* output_bdd = aigerToBdd(ddMgr, inputs, and_gates, output, var_to_bdd);
        if (!output_bdd) {
            cerr << "Error: Failed to convert output " << output << " to BDD" << endl;
            continue;
        }

        // 打印输出BDD信息
        cout << "Output BDD for variable " << output << ":" << endl;
        Cudd_PrintDebug(ddMgr, output_bdd, num_inputs, 3);
        
        // 减少引用计数
        Cudd_RecursiveDeref(ddMgr, output_bdd);
    }

    // 清理CUDD管理器
    Cudd_Quit(ddMgr);

    return 0;
}

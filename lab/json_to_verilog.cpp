#include <iostream>
#include <fstream>
#include <string>
#include "./third_parties/nlohmann/json.hpp"

using json = nlohmann::json;

// 生成Verilog变量声明
std::string generate_variable_declarations(const json& variable_list) {
    std::string verilog_code;
    for (const auto& var : variable_list) {
        std::string name = var["name"];
        int bit_width = var["bit_width"];
        verilog_code += "    input wire [" + std::to_string(bit_width - 1) + ":0] " + name + ",\n";
    }
    verilog_code += "    output wire result\n";
    return verilog_code;
}

// 递归生成Verilog表达式
std::string generate_expression(const json& expr, const json& variable_list) {
    std::string op = expr["op"];
    
    // 一元操作符
    if (op == "LOG_NEG") {
        return "!(" + generate_expression(expr["lhs_expression"], variable_list) + ")";
    } else if (op == "BIT_NEG") {
        return "~(" + generate_expression(expr["lhs_expression"], variable_list) + ")";
    } else if (op == "MINUS") {
        return "-(" + generate_expression(expr["lhs_expression"], variable_list) + ")";
    }
    // 二元操作符
    else if (op == "ADD") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " + " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "SUB") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " - " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "MUL") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " * " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "DIV") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " / " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LOG_AND") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " && " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LOG_OR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " || " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "EQ") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " == " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "NEQ") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " != " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " < " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LTE") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " <= " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "GT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " > " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "GTE") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " >= " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_AND") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " & " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_OR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " | " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " ^ " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " >> " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list) + " << " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "IMPLY") {
        // Verilog中没有直接的 "->" 操作符，用逻辑等价形式 (!a || b) 表示
        return "(!(" + generate_expression(expr["lhs_expression"], variable_list) + ") || " +
               generate_expression(expr["rhs_expression"], variable_list) + ")";
    }
    // 变量或常量
    else if (op == "VAR") {
        int id = expr["id"];
        return variable_list[id]["name"];
    } else if (op == "CONST") {
        return expr["value"];
    }
    // 未知操作符
    else {
        std::cerr << "Warning: Unknown operator '" << op << "' in expression.\n";
        return "";
    }
}

// 生成Verilog约束
std::string generate_constraints(const json& constraint_list, const json& variable_list) {
    std::string verilog_code;
    std::string result;
    int constraint_count = 0;
    for (const auto& constraint : constraint_list) {
        verilog_code += "    assign cnstr" + std::to_string(constraint_count) + " = " + generate_expression(constraint, variable_list) + ";\n";
        verilog_code += "    assign cnstr" + std::to_string(constraint_count) + "_redOR = " + "|cnstr" + std::to_string(constraint_count) + ";\n";
        result += "cnstr" + std::to_string(constraint_count) + "_redOR & ";
        constraint_count++;
    }
    result.erase(result.length()-3);
    verilog_code += "    assign result = " + result + ";\n";
    return verilog_code;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_json_file> <output_verilog_file>\n";
        return 1;
    }

    std::string input_json_file = argv[1];
    std::string output_verilog_file = argv[2];

    // 读取JSON文件
    std::ifstream json_file(input_json_file);
    if (!json_file.is_open()) {
        std::cerr << "Error: Could not open JSON file " << input_json_file << "\n";
        return 1;
    }

    json data;
    try {
        json_file >> data;
    } catch (const json::parse_error& e) {
        std::cerr << "Error: Failed to parse JSON file: " << e.what() << "\n";
        return 1;
    }

    // 生成Verilog代码
    std::string verilog_code = "module test(\n";
    verilog_code += generate_variable_declarations(data["variable_list"]);
    verilog_code += ");\n\n";
    verilog_code += generate_constraints(data["constraint_list"], data["variable_list"]);
    verilog_code += "endmodule\n";

    // 写入Verilog文件
    std::ofstream verilog_file(output_verilog_file);
    if (!verilog_file.is_open()) {
        std::cerr << "Error: Could not open Verilog file " << output_verilog_file << "\n";
        return 1;
    }
    verilog_file << verilog_code;
    verilog_file.close();

    std::cout << "Verilog code successfully written to " << output_verilog_file << "\n";
    return 0;
}
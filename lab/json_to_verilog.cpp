#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

std::string generate_variable_declarations(const json& expr, const json& variable_list) {
    std::string op = expr["op"];
    
    // 基本变量常量
    if (op == "VAR") {
        int id = expr["id"];
        return variable_list[id]["name"];
    } else if (op == "CONST") {
        return expr["value"];
    }
    
    // 单目运算符
    else if (op == "LOG_NEG") {
        return "(!(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + "))";
    } else if (op == "BIT_NEG") {
        return "(~(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + "))";
    } else if (op == "MINUS") {
        return "(-" + generate_variable_declarations(expr["lhs_expression"], variable_list) + ")";
    }
    
    // 双目运算符
    else if (op == "ADD") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " + " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "SUB") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " - " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "MUL") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " * " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "DIV") {
        std::string divisor = generate_variable_declarations(expr["rhs_expression"], variable_list);
        return "(" + divisor + " == 0 ? 0 : " + generate_variable_declarations(expr["lhs_expression"], variable_list) + " / " + divisor + ")";
    } else if (op == "LOG_AND") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " && " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LOG_OR") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " || " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "EQ") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " == " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "NEQ") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " != " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LT") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " < " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LTE") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " <= " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "GT") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " > " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "GTE") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " >= " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_AND") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " & " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_OR") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " | " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " ^ " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " >> " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generate_variable_declarations(expr["lhs_expression"], variable_list) + " << " + generate_variable_declarations(expr["rhs_expression"], variable_list) + ")";
    } else if (op == "IMPLY") {
        std::string lhs = generate_variable_declarations(expr["lhs_expression"], variable_list);
        std::string rhs = generate_variable_declarations(expr["rhs_expression"], variable_list);
        return "(!(" + lhs + " != 0) || (" + rhs + " != 0))";
    }

    // 未定义运算符
    std::cerr << "Warning: Unsupported operator '" << op << "'" << std::endl;
    return "";  
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_json_file> <output_verilog_file>\n";
        return 1;
    }

    std::string input_json_file = argv[1];
    std::string output_verilog_file = argv[2];
    
    // 打开并解析 JSON 输入文件
    std::ifstream inputFile(input_json_file);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open JSON file " << input_json_file << std::endl;
        return 1;
    }

    json inputJson;
    inputFile >> inputJson;

    auto& variable_list = inputJson["variable_list"];
    const auto& constraintList = inputJson["constraint_list"];

    // 生成输出文件
    std::ofstream outputFile(output_verilog_file);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Failed to parse JSON file: " << output_verilog_file << std::endl;
        return 1;
    }

    // 生成 Verilog module 名称
    outputFile << "module test(";

    // 从变量名称中删除引号
    for (size_t i = 0; i < variable_list.size(); ++i) {
        std::string name = variable_list[i]["name"].get<std::string>();
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        variable_list[i]["name"] = name;
    }

    // 生成端口列表
    for (size_t i = 0; i < variable_list.size(); ++i) {
        outputFile << variable_list[i]["name"].get<std::string>();
        if (i != variable_list.size() - 1) {
            outputFile << ", ";
        }
    }
    outputFile << ", result";
    outputFile << ");" << std::endl;

    // 生成输入声明
    for (size_t i = 0; i < variable_list.size(); ++i) {
        outputFile << "    input "
                   << (variable_list[i]["signed"] ? "signed " : "")
                   << "[" << (static_cast<int>(variable_list[i]["bit_width"]) - 1) << ":0] "
                   << variable_list[i]["name"].get<std::string>() << ";" << std::endl;
    }

    // 生成内部约束wires
    outputFile << "    wire cnstr_0";
    for (size_t i = 1; i < constraintList.size(); ++i) {
        outputFile << ", cnstr_" << i;
    }
    outputFile << ";" << std::endl;
    
    // 生成输出声明
    outputFile << "    output wire result;" << std::endl;
    outputFile << std::endl;

    // 生成约束
    for (size_t i = 0; i < constraintList.size(); ++i) {
        std::string expr = generate_variable_declarations(constraintList[i], variable_list);
        outputFile << "    assign cnstr_" << i << " = |(" << expr << ");" << std::endl;
    }
    
    // 生成总体约束（所有单个约束的 AND）
    outputFile << "    assign result = cnstr_0";
    for (size_t i = 1; i < constraintList.size(); ++i) {
        outputFile << " & cnstr_" << i;
    }
    outputFile << ";" << std::endl;
    
    // End module
    outputFile << "endmodule" << std::endl;
    
    std::cout << "Verilog code successfully written to " << output_verilog_file << std::endl;
    return 0;
}

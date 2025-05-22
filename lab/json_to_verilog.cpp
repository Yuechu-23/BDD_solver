#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

// 变量声明
std::string generate_variable_declarations(const json& variable_list) {
    std::string verilog_code;
    for (const auto& var : variable_list) {
        std::string name = var["name"];
        int bit_width = var["bit_width"];
        verilog_code += "    input [" + std::to_string(bit_width - 1) + ":0] " + name + ",\n";
    }
    verilog_code += "    output result\n";
    return verilog_code;
}

// 约束表达式
std::string generate_expression(const json& expr, const json& variable_list, std::vector<std::string>& divide_exprs) {
    std::string op = expr["op"];
    // 一元操作符
    if (op == "LOG_NEG") {
        return "!(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "BIT_NEG") {
        return "~(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "MINUS") {
        return "-(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + ")";
    }
    // 二元操作符
    else if (op == "ADD") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " + " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "SUB") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " - " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "MUL") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " * " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "DIV") {
        std::string divisor = generate_expression(expr["rhs_expression"], variable_list, divide_exprs);
        divide_exprs.push_back(divisor);
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " / " + divisor + ")";
    } else if (op == "LOG_AND") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " && " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "LOG_OR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " || " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "EQ") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " == " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "NEQ") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " != " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "LT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " < " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "LTE") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " <= " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "GT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " > " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "GTE") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " >= " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "BIT_AND") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " & " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "BIT_OR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " | " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "BIT_XOR") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " ^ " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "RSHIFT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " >> " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "LSHIFT") {
        return "(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + " << " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    } else if (op == "IMPLY") {
        return "(!(" + generate_expression(expr["lhs_expression"], variable_list, divide_exprs) + ") || " +
               generate_expression(expr["rhs_expression"], variable_list, divide_exprs) + ")";
    }
    // 变量或常量
    else if (op == "VAR") {
        int id = expr["id"];
        return variable_list[id]["name"];
    } else if (op == "CONST") {
        return expr["value"];
    }
}

// 生成Verilog约束
std::string generate_constraints(const json& constraint_list, const json& variable_list) {
    std::string verilog_code;
    std::string result;
    int constraint_count = 0;
    std::vector<std::string> divide_exprs;
    for (const auto& constraint : constraint_list) {
        verilog_code += "    wire cnstr" + std::to_string(constraint_count) + ";\n";
        verilog_code += "    assign cnstr" + std::to_string(constraint_count) + " = |(" + generate_expression(constraint, variable_list, divide_exprs) + ");\n";
        result += "cnstr" + std::to_string(constraint_count) + " & ";
        constraint_count++;
    }
    for(const auto& expr: divide_exprs) {
        verilog_code += "   wire cnstrDIV" + std::to_string(constraint_count) + ";\n";
        verilog_code += "   assign cnstrDIV" + std::to_string(constraint_count) + " = |(" + expr + ");\n";
        result += "cnstrDIV" + std::to_string(constraint_count) + " & ";
        constraint_count++;
    }
    result.erase(result.length()-3);
    verilog_code += "    assign result = " + result + ";\n";
    return verilog_code;
}

int main(int argc, char* argv[]) {
    std::ifstream json_file(argv[1]);
    json data;
    json_file >> data;

    // 生成Verilog代码
    std::string verilog_code = "module test(\n";
    verilog_code += generate_variable_declarations(data["variable_list"]);
    verilog_code += ");\n\n";
    verilog_code += generate_constraints(data["constraint_list"], data["variable_list"]);
    verilog_code += "endmodule\n";

    // 写入Verilog文件
    std::ofstream verilog_file(argv[2]);
    verilog_file << verilog_code;
    verilog_file.close();
    return 0;
}
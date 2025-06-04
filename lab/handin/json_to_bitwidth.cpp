#include <iostream>
#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
#include <vector>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.json>\n";
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << '\n';
        return 1;
    }
    json j;
    input_file >> j;
    std::vector<int> var_bit_widths;
    for (const auto& var : j["variable_list"]) {
        int bit_width = var["bit_width"];
        var_bit_widths.push_back(bit_width);
    }
    input_file.close();
    std::ofstream output_file(argv[2]);
    for (const auto& width : var_bit_widths) {
        output_file << width << '\n';
    }
    output_file.close();
    return 0;
}

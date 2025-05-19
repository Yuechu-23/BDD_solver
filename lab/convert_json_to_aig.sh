#!/bin/bash

# 赋予执行权限： chmod +x convert_json_to_aig.sh
# 运行脚本： ./convert_json_to_aig.sh

# 参数定义（可修改为实际路径，此处以opt1为例）
addr1="/root/workspace/sv-sampler-lab/opt1"      # 输入JSON文件目录
addr2="/root/workspace/sv-sampler-lab/opt1"  # 输出Verilog文件目录
addr3="/root/workspace/sv-sampler-lab/opt1"      # 输出AIG文件目录
json_to_verilog_cpp="json_to_verilog.cpp"  # C++转换程序路径

# 编译 json_to_verilog.cpp
echo "Step 1: Compiling json_to_verilog.cpp..."
g++ -o json_to_verilog "$json_to_verilog_cpp" -lstdc++fs
if [ $? -ne 0 ]; then
    echo "Error: Failed to compile json_to_verilog.cpp"
    exit 1
fi

# 运行 JSON → Verilog 转换 + 使用 Yosys 综合为 AIG (可同时批量处理多个文件)
for json_file in "$addr1"/*.json; do
    # JSON → Verilog
    base=$(basename "$json_file" .json)
    ./json_to_verilog "$json_file" "$addr2/$base.v"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to convert JSON to Verilog"
        exit 1
    fi
    # Verilog → AIG
    yosys -q <<EOF
    read_verilog $addr2/$base.v
    synth
    aigmap
    write_aiger $addr3/$base.aig
EOF
done

echo "Success! AIG files saved to $addr3"

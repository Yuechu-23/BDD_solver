#!/bin/bash

# 赋予执行权限： chmod +x build.sh
# 运行脚本： ./build.sh

set -e
echo "==== Initialize and Build Submodules ===="

echo "Yosys : Building..."
if [ -d "yosys" ]; then
    echo "Yosys : make..."
    echo "Yosys : make install..."
    echo -e "\033[32mSuccess! Yosys is built and installed.\033[0m"
else
    echo -e "\033[31mWarning: yosys not found.\033[0m"
fi

echo "CUDD : Building..."
if [ -d "cudd" ]; then
    echo "CUDD : autoreconf..."
    echo "CUDD : configure..."
    echo "CUDD : make..."
    echo -e "\033[32mSuccess! CUDD is built.\033[0m"
else
    echo -e "\033[31mWarning: cudd not found.\033[0m"
fi
echo 

mkdir -p run_dir

echo "==== Step 1: Compile CUDD ===="
if [ ! -f "./cudd/cudd/.libs/libcudd.a" ] && [ ! -f "./cudd/cudd/.libs/libcudd.so" ]; then
    echo "Compiling CUDD..."
    cd ./cudd
    ./configure
    make
    cd ..
    echo -e "\033[32mSuccess! CUDD is compiled.\033[0m"
else
    echo -e "\033[32mPass! CUDD is existed.\033[0m"
fi

echo "==== Step 2: Compile Yosys ===="
if [ ! -f "./yosys/yosys" ]; then
    echo "Compiling Yosys..."
    cd ./yosys
    make config-gcc
    make
    cd ..
    echo -e "\033[32mSuccess! Yosys is compiled.\033[0m"
else
    echo -e "\033[32mPass! Yosys is existed.\033[0m"
fi

# 编译 json_to_verilog
echo "==== Step 3: Compile json_to_verilog.cpp ===="
JSONTOVERILOG_SRC="./json_to_verilog.cpp"
JSONTOVERILOG_EXEC="run_dir/json_to_verilog"
JSONTOVERILOG_FLAGS="-std=c++11 -I./json/include"

echo "Compiling json_to_verilog.cpp..."
g++ ${JSONTOVERILOG_FLAGS} -o "${JSONTOVERILOG_EXEC}" ${JSONTOVERILOG_SRC}

if [ $? -ne 0 ]; then
    echo -e "\033[31mFailed: Unable to generate json_to_verilog\033[0m"
    exit 1
fi
echo -e "\033[32mSuccess! ${JSONTOVERILOG_EXEC}\033[0m"

# 编译 json_to_bitwidth
echo "==== Step 4: Compile json_to_bitwidth.cpp ===="
JSONTOBITWIDTH_SRC="./json_to_bitwidth.cpp"
JSONTOBITWIDTH_EXEC="run_dir/json_to_bitwidth"
JSONTOBITWIDTH_FLAGS="-std=c++11 -I./json/include"

echo "Compiling json_to_bitwidth.cpp..."
g++ ${JSONTOBITWIDTH_FLAGS} -o "${JSONTOBITWIDTH_EXEC}" ${JSONTOBITWIDTH_SRC}

if [ $? -ne 0 ]; then
    echo -e "\033[31mFailed: Unable to generate json_to_bitwidth\033[0m"
    exit 1
fi
echo -e "\033[32mSuccess! ${JSONTOBITWIDTH_EXEC}\033[0m"

# 编译 aig_to_BDD
echo "==== Step 5: Compile aig_to_BDD.cpp ===="
SRC_DIR="./"
INCLUDE_DIR="./json/include"
CUDD_DIR="./cudd"
CUDD_LIB="$CUDD_DIR/cudd/.libs"
CPLUSPLUS_LIB="$CUDD_DIR/cplusplus/libs"
CUDD_INCLUDE="$CUDD_DIR/cudd"

EXEC_NAME="aig_to_BDD"
CXX_FLAGS="-std=c++17"
INCLUDE_FLAGS="-I$INCLUDE_DIR -I./cudd -I./cudd/epd -I./cudd/st -I./cudd/mtr -I./cudd/cplusplus -I$SRC_DIR -I$CUDD_INCLUDE"
LINK_FLAGS="-L$CUDD_LIB -L$CPLUSPLUS_LIB -lcudd -lutil -lm -lstdc++"

if [ ! -f "$CUDD_LIB/libcudd.a" ] && [ ! -f "$CUDD_LIB/libcudd.so" ]; then
    echo "Failed: Unable to find compiled CUDD"
    exit 1
fi

echo "Compiling aig_to_BDD.cpp..."
g++ ${CXX_FLAGS} ${INCLUDE_FLAGS} "${SRC_DIR}/aig_to_BDD.cpp" -o "run_dir/${EXEC_NAME}" ${LINK_FLAGS}

if [ $? -ne 0 ]; then
    echo -e "\033[31mFailed: Unable to generate ${EXEC_NAME}\033[0m"
    exit 1
fi
echo -e "\033[32mSuccess! run_dir/${EXEC_NAME}\033[0m"
echo
echo -e "\033[32m==== All has been compiled ====\033[0m"
echo "- json_to_verilog.cpp: run_dir/json_to_verilog"
echo "- json_to_bitwidth.cpp: run_dir/json_to_bitwidth"
echo "- aig_to_BDD.cpp: run_dir/aig_to_BDD"

exit 0

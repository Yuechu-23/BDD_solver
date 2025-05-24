#!/usr/bin/env bash
set -euo pipefail

# 赋予执行权限： chmod +x build_cudd.sh
# 运行脚本： ./build_cudd.sh

CUDD_DIR="${HOME}/workspace/sv-sampler-lab/cudd"
PROJECT_DIR="${HOME}/workspace/sv-sampler-lab"
TARGET="aig_to_BDD"
CXX="g++"
STD="c++17"

cd "${PROJECT_DIR}"

"${CXX}" -std=${STD} \
  -I "${CUDD_DIR}" \
  -I "${CUDD_DIR}/cplusplus" \
  -I "${CUDD_DIR}/cudd" \
  -I "${CUDD_DIR}/st" \
  -I "${CUDD_DIR}/mtr" \
  -I "${CUDD_DIR}/epd" \
  -o "${TARGET}" aig_to_BDD.cpp \
  -L "${CUDD_DIR}/cplusplus/.libs" \
  -L "${CUDD_DIR}/cudd/.libs" \
  -lcudd -lutil -lm -lstdc++

# ./${TARGET} ${PROJECT_DIR}/basic/temp/0.aig

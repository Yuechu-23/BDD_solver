# !/bin/bash

# 赋予执行权限： chmod +x run.sh
# 运行脚本： ./run.sh

if [ $# -lt 4 ]; then
  echo "用法: $0 [constraint.json] [num_samples] [run_dir] [random_seed]"
  echo "  constraint.json: "
  echo "  num_samples: "
  echo "  run_dir: "
  echo "  random_seed: "
  exit 1
fi

CONSTRAINT_JSON="$1"
NUM_SAMPLES="$2"
RUN_DIR="$3"
RANDOM_SEED="$4"

if [ ! -f "$CONSTRAINT_JSON" ]; then
  echo "错误: 找不到约束条件文件 $CONSTRAINT_JSON"
  exit 1
fi

mkdir -p "$RUN_DIR"

BASE_DIR="/root/workspace/sv-sampler-lab/run_dir"
JSON_DIR="${BASE_DIR}/json"
CUDD_DIR="${BASE_DIR}/cudd"
JSON_TO_VERILOG="${BASE_DIR}/json_to_verilog"
JSON_TO_BITWIDTH="${BASE_DIR}/json_to_bitwidth"
AIG_TO_BDD="${BASE_DIR}/aig_to_BDD"
BASE_FILENAME=$(basename "$CONSTRAINT_JSON" .json)
BITWIGTH_FILE="${RUN_DIR}/${BASE_FILENAME}.bitwidth.txt"
VERILOG_FILE="${RUN_DIR}/${BASE_FILENAME}.v"
AIG_FILE="${RUN_DIR}/${BASE_FILENAME}.aig"
SAMPLES_FILE="${RUN_DIR}/result.json"

"$JSON_TO_VERILOG" "$CONSTRAINT_JSON" "$VERILOG_FILE"
"$JSON_TO_BITWIDTH" "$CONSTRAINT_JSON" "$BITWIGTH_FILE"

yosys -q <<EOF
read_verilog "$VERILOG_FILE"
synth
aigmap
write_aiger -ascii "$AIG_FILE"
EOF

"$AIG_TO_BDD" "$AIG_FILE" "$NUM_SAMPLES" "$RANDOM_SEED" "$BITWIGTH_FILE" "$SAMPLES_FILE"

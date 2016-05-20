#!/bin/bash

model=worstCase
model=GE2014

for i in `seq 1 7`; do
  for j in `seq 1 5`; do
    ./testTool.x DrawProfileTest_${j}p${i}_24hr67 $model
    echo ""
  done
done


./testTool.x slowFlowTest_0.25 $model
./testTool.x slowFlowTest_0.3 $model
./testTool.x slowFlowTest_0.4 $model
./testTool.x slowFlowTest_0.5 $model
./testTool.x slowFlowTest_0.6 $model
./testTool.x slowFlowTest_0.75 $model
./testTool.x slowFlowTest_1.0 $model

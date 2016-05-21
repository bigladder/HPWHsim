#!/bin/bash

model=custom
model=GE2014

for i in `seq 1 7`; do
  for j in `seq 1 5`; do
    ./testTool.x DrawProfileTest_${j}p${i}_24hr67 $model
    echo ""
  done
done


./testTool.x slowFlowTest_0.25 $model
echo ""
./testTool.x slowFlowTest_0.3 $model
echo ""
./testTool.x slowFlowTest_0.4 $model
echo ""
./testTool.x slowFlowTest_0.5 $model
echo ""
./testTool.x slowFlowTest_0.6 $model
echo ""
./testTool.x slowFlowTest_0.75 $model
echo ""
./testTool.x slowFlowTest_1.0 $model

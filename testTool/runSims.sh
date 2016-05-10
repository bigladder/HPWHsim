#!/bin/bash


for i in `seq 1 7`; do
  for j in `seq 1 5`; do
    ./testTool.x DrawProfileTest_${j}p${i}_24hr67 worstCase
  done
done


./testTool.x slowFlowTest_0.25 worstCase
./testTool.x slowFlowTest_0.5 worstCase
./testTool.x slowFlowTest_0.75 worstCase
./testTool.x slowFlowTest_1.0 worstCase

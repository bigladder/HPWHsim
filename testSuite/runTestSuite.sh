#!/bin/bash

# Script to run standard test suite given a build

if [ -z $1 ]; then 
  echo "Must enter a directory name"
  exit
fi


rm -f $1
mkdir $1

testNames="test30 test50 test70 test95"
modelNames="AOSmithPHPT60 AOSmithHPTU80 SandenGAU RheemHB50 Stiebel220e GE502014"

for testName in $testNames; do
  for modelName in $modelNames; do
    ./testTool.x Preset $modelName $testName
    ./testTool.x File $modelName $testName
    mv $testName/File_$modelName.csv ./$1/${testName}_File_${modelName}.csv
    mv $testName/Preset_$modelName.csv ./$1/${testName}_Preset_${modelName}.csv
  done
done


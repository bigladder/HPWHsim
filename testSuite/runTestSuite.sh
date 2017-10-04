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


# Take care of the lockout tests
for modelName in $modelNames; do
  if [ "$modelName" = "AOSmithPHPT60" ]; then
    airTemp="48"
  elif [ "$modelName" = "AOSmithHPTU80" ]; then
    airTemp="45"
  elif [ "$modelName" = "RheemHB50" ]; then
    airTemp="43"
  elif [ "$modelName" = "Stiebel220e" ]; then
    airTemp="35"
  elif [ "$modelName" = "GE502014" ]; then
    airTemp="40"
  else
    continue
  fi
  echo "./testTool.x Preset $modelName testLockout $airTemp"
  ./testTool.x Preset $modelName testLockout $airTemp
  ./testTool.x File $modelName testLockout  $airTemp
  mv testLockout/File_$modelName.csv ./$1/testLockout_File_${modelName}.csv
  mv testLockout/Preset_$modelName.csv ./$1/testLockout_Preset_${modelName}.csv

done


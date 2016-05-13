#!/bin/bash

#for i in 0.74 0.76 0.78 0.8 0.82 0.84 0.86 0.88 0.90 0.92 0.94
for i in 19.66 19.16 18.66 18.16 17.66 17.16 16.66 16.16 15.66
do
  #temp=`echo "$i*5.50" | bc`
  #temp2=`echo "$i*7.207" | bc`

  temp=`echo "0.78*5.50" | bc`
  temp2=`echo "0.78*7.207" | bc`

  cat ./models/worstCase/parameterFile2.txt  > ./models/worstCase/parameterFile.txt
  echo "heatsource 2 copT1const $temp" >> ./models/worstCase/parameterFile.txt
  echo "heatsource 2 copT2const $temp2" >> ./models/worstCase/parameterFile.txt

  echo "heatsource 0 onlogic topThird $i F" >> ./models/worstCase/parameterFile.txt



  #./testTool.x DOE2014_24hr67 worstCase > /dev/null
  ./testTool.x DrawProfileTest_4p5_24hr67 worstCase > /dev/null
  echo $i
  #cat ./models/worstCase/DOE2014_24hr67/TestToolOutput.csv | awk -F, 'BEGIN{sum9 = 0; sum10 = 0;num = 0}(NR > 1){sum9 += $9; sum10 += $10; num++}END{print sum9 " " sum10 " " sum10/sum9}'
  cat ./models/worstCase/DrawProfileTest_4p5_24hr67/TestToolOutput.csv | awk -F, 'BEGIN{sum6 = 0; sum8 = 0; sum9 = 0; sum10 = 0;num = 0}(NR > 1){sum6 += $6; sum8 += $8; sum9 += $9; sum10 += $10; num++}END{print sum6 " " sum8 " " sum9 " " (sum6 + sum8)/sum9}'
  echo ""
done


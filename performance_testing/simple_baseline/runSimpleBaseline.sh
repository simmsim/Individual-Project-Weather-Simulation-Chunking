#!/bin/bash

### Produce average and append it to the log file.
function produce_average() {
    sumMili=0
    sumMicro=0
    lineNo=0
    while read -r line
    do
        value="$(echo "$line" | awk '{print $2;}')"
        if [[ $((${lineNo}%2)) -eq 0 ]]; then
            sumMili=$(( sumMili+${value} ))
        else
            sumMicro=$(( sumMicro+${value} ))
        fi
        lineNo=$(( lineNo+1 ))
    done < <(tail -n +5 $1)

    avgMili=$(( sumMili/$2 ))
    avgMicro=$(( sumMicro/$2 ))

    echo -e "\nAverage[ms] ${avgMili}" >> $1
    echo "Average[µs] ${avgMicro}" >> $1
}

### Construct fileName and echo info to console.
timestamp=$(date +%d-%m-%Y_%H-%M-%S)
fileNameStem="simpleBaselineLog_"
location="../performance_testing/simple_baseline/"
fileName="${fileNameStem}${timestamp}.txt"

numberOfRuns=1
numberOfIterations=1
ip=1000
jp=1000
kp=90

numberOfRunsDescription="Number of runs ${numberOfRuns}"
numberOfIterationsDescription="Number of iterations ${numberOfIterations}"
dimensionsDescription="Dimensions ip=${ip}, jp=${jp}, kp=${kp}"
### Append property values used in this run.
echo "${numberOfRunsDescription}" >> ${fileName}
echo "${numberOfIterationsDescription}" >> ${fileName}
echo -e "${dimensionsDescription}\n" >> ${fileName}

### Output information to console
echo "The execution time results will be printed to ${fileName}, where:"
echo "${numberOfRunsDescription}"
echo "${numberOfIterationsDescription}"
echo "${dimensionsDescription}"

### Save dimension properties to param file
propertiesFile="../../sor-c-reference/sor_params.h"
echo "const int ip = ${ip};" > ${propertiesFile}
echo "const int jp = ${jp};" >> ${propertiesFile}
echo "const int kp = ${kp};" >> ${propertiesFile}

### Compile
g++ -std=c++11 -O3 ../../sor-c-reference/SOR_Simple_Baseline.cpp -o ../../sor-c-reference/SOR_Simple_Baseline

### Run
for ((i=0; i<$numberOfRuns; i++)); do 
    ../../sor-c-reference/./SOR_Simple_Baseline "$fileName" "$numberOfIterations"
done

### Print out whether the file creation succeeded - doesn't check for content
echo -ne "Runs have finished, and the file ${fileName} "
if test -f "${fileName}"; then
    echo "was SUCCESSFULLY created."
    produce_average ${fileName} ${numberOfRuns}
else
    echo "FAILED to be created."
fi

### Cleanup
rm ../../sor-c-reference/SOR_Simple_Baseline
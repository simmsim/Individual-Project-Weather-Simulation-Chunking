#!/bin/bash

measurements=(0,0,0,0,0,0,0,0,0,0)

declare -A nameIndexDictionary
nameIndexDictionary["ElapsedOCL[ms]"]=0
nameIndexDictionary["ElapsedOCL[µs]"]=1
nameIndexDictionary["ElapsedSimulationOnly[ms]"]=2
nameIndexDictionary["ElapsedSimulationOnly[µs]"]=3
nameIndexDictionary["Kernel[ms]"]=4
nameIndexDictionary["Kernel[µs]"]=5
nameIndexDictionary["Write[ms]"]=6
nameIndexDictionary["Write[µs]"]=7
nameIndexDictionary["Read[ms]"]=8
nameIndexDictionary["Read[µs]"]=9

declare -a orders
orders+=( "ElapsedOCL[ms]" )
orders+=( "ElapsedOCL[µs]" )
orders+=( "ElapsedSimulationOnly[ms]" )
orders+=( "ElapsedSimulationOnly[µs]" )
orders+=( "Kernel[ms]" )
orders+=( "Kernel[µs]" )
orders+=( "Write[ms]" )
orders+=( "Write[µs]" )
orders+=( "Read[ms]" )
orders+=( "Read[µs]" )

function addMeasurement() {
    index="$1"
    if [[ $((${index}%2)) -eq 0 ]]; then
        currentSumMili=${measurements[${index}]}
        measurements[${index}]=$(( currentSumMili+"$2" ))
    else
        currentSumMicro=${measurements[${index}]}
        measurements[${index}]=$(( currentSumMicro+"$2" ))
    fi
}

function produce_average() {
    while read -r line
    do
        measurementName="$(echo "$line" | awk '{print $1;}')"
        if [[ -z "$measurementName" ]]; then
            continue
        fi
        value="$(echo "$line" | awk '{print $2;}')"
        startIndex=${nameIndexDictionary[${measurementName}]}
        if [[ ! -z "$startIndex" ]]; then
            addMeasurement ${startIndex} ${value}
        fi
    done < <(tail -n +6 $1)

    for i in "${orders[@]}"; do
        keyName=$i
        indexValue=${nameIndexDictionary[${keyName}]}
        measurementValue=${measurements[${indexValue}]}
        average=$(( measurementValue/$2 ))
        echo "Average ${keyName} ${average}" >> $1
    done
}

### Construct fileName and echo info to console.
timestamp=$(date +%d-%m-%Y_%H-%M-%S)
deviceName="CPU"
fileNameStem="apiVersionLog_"
location="../performance_testing/api_version/"
fileName="${fileNameStem}${deviceName}_${timestamp}.txt"

numberOfRuns=5
numberOfIterations=5
# 0 for CPU and 1 for GPU
device=0
ip=152
jp=152
kp=92
cip=150
cjp=150
ckp=90

numberOfRunsDescription="Number of runs ${numberOfRuns}"
numberOfIterationsDescription="Number of iterations ${numberOfIterations}"
coreDimensionsDescription="Core dimensions ip=${ip}, jp=${jp}, kp=${kp}"
chunkDimensionsDescription="Chunk dimensions cip=${cip}, cjp=${cjp}, ckp=${ckp}"

### Append property values used in this run.
echo "${numberOfRunsDescription}" >> ${fileName}
echo "${numberOfIterationsDescription}" >> ${fileName}
echo "${coreDimensionsDescription}" >> ${fileName}
echo -e "${chunkDimensionsDescription}\n" >> ${fileName}

### Output information to console
echo "The execution time results will be printed to ${fileName}, where:"
echo "${numberOfRunsDescription}"
echo "${numberOfIterationsDescription}"
echo "${dimensionsDescription}"
echo "${chunkDimensionsDescription}"

### Save dimension properties to param file
propertiesFile="../../sor-c-reference/sor_api_params.h"
echo "const int ip = ${ip};" > ${propertiesFile}
echo "const int jp = ${jp};" >> ${propertiesFile}
echo "const int kp = ${kp};" >> ${propertiesFile}
echo "const int cip = ${cip};" >> ${propertiesFile}
echo "const int cjp = ${cjp};" >> ${propertiesFile}
echo "const int ckp = ${ckp};" >> ${propertiesFile}

### Compile
### Add -DMEASURE_EVENT_PERFORMANCE if want to measure execution time of OpenCL events
g++ -std=c++11 -O3 -DMEASURE_EVENT_PERFORMANCE ../../sor-c-reference/SOR_API_Version.cpp -o ../../sor-c-reference/SOR_API_Version ../../Simulation.cpp ../../OCLSetup.cpp ../../ErrorHelper.cpp -lOpenCL

### Run
for ((i=0; i<$numberOfRuns; i++)); do 
    ../../sor-c-reference/./SOR_API_Version "$fileName" "$numberOfIterations" "$device"
done

### Print out whether the file creation succeeded - doesn't check for content
echo -ne "Runs have finished, and the file ${fileName} "
# fix this to check length of the file to determine success
linesInFile="$(cat "$fileName" | wc -l)"
if test -f "${fileName}"; then
    echo "was SUCCESSFULLY populated."
    produce_average ${fileName} ${numberOfRuns}
else
    echo "FAILED to be populated."
fi

### Cleanup
rm ../../sor-c-reference/SOR_API_Version
#!/bin/bash

### Construct fileName and echo info to console.
timestamp=$(date +%d-%m-%Y_%H-%M-%S)
deviceName="CPU"
fileNameStem="apiVersionLog_"
location="../performance_testing/api_version/"
fileName="${fileNameStem}${deviceName}_${timestamp}.txt"

numberOfRuns=10
numberOfIterations=50
# 0 for CPU and 1 for GPU
device=0
ip=150
jp=150
kp=90
cip=30
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
g++ -std=c++11 -O3 ../../sor-c-reference/SOR_API_Version.cpp -o ../../sor-c-reference/SOR_API_Version ../../Simulation.cpp ../../OCLSetup.cpp ../../ErrorHelper.cpp -lOpenCL

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
    #produce_average ${fileName} ${numberOfRuns}
else
    echo "FAILED to be populated."
fi

### Cleanup
rm ../../sor-c-reference/SOR_API_Version
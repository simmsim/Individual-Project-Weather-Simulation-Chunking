#include "../SimulationRange.h"
#include "fillAndChunk1D.h"

#include <iostream>
#include <string>

bool assertEquals(float * expected, float * actual, int coreSize) {
    bool equals = true;
    for (int i = 0; i < coreSize; i++) {
        if (expected[i] != actual[i]) {
            equals = false;
            std::cout << "Values were different at index " << i << ": expected {"
            << expected[i] << "} but actual was {" << actual[i] << "}\n";
            break;
        }
    }

    return equals;
}

void chunk1D(SimulationRange coreRange, SimulationRange chunkRange,
             SimulationRange haloRange, float * expected,
             std::string testName) {
    int coreSize = coreRange.getSimulationSize();

    float *outputArray = (float*)malloc(sizeof(float)*coreSize);
    std::fill(outputArray, outputArray+(coreSize), 0.0);

    float *inputArray = (float*)malloc(sizeof(float)*coreSize);
    for (int idx = 0; idx < coreSize; idx++) {
        inputArray[idx] = idx + 1;
    }

    fillAndChunk(coreRange, chunkRange, haloRange, inputArray, outputArray);

    bool equals = assertEquals(expected, outputArray, coreSize);
    if (equals) {
        std::cout << "Test succeeded for " << testName << std::endl;
    } else {
        std::cout << "Test failed for " << testName << std::endl;
    }

    free(outputArray);
    free(inputArray);
}

void chunk1DUsingChunkSize2() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(4);
    SimulationRange haloRange = SimulationRange(6);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk1D(coreRange, chunkRange, haloRange, expected, "chunk1DUsingChunkSize2");
}

void process1DInFullWithoutChunking() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(10);
    SimulationRange haloRange = SimulationRange(12);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk1D(coreRange, chunkRange, haloRange, expected, "process1DInFullWithoutChunking");
}

int main(void) {
    chunk1DUsingChunkSize2();
   // process1DInFullWithoutChunking();
}
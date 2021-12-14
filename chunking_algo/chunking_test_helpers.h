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

void chunk(SimulationRange coreRange, SimulationRange chunkRange,
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
        std::cout << "Test SUCCEEDED for " << testName << std::endl;
    } else {
        std::cout << "Test FAILED for " << testName << std::endl;
    }

    std::cout << "hello\n";
    free(inputArray);
    free(outputArray);
}
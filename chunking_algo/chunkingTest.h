#include "../SimulationRange.h"
#include "../test_helpers/assertions.h"
#include "fillAndChunk.h"

#include <string>

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

    assertEquals(expected, outputArray, coreSize, testName);

    free(inputArray);
    free(outputArray);
}
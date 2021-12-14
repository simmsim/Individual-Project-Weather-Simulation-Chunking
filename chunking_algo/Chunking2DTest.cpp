#include "chunking_test_helpers.h"

#include <iostream>
#include <string>

void process2DWithLeftoverChunk() {
    SimulationRange coreRange = SimulationRange(6, 4);
    SimulationRange chunkRange = SimulationRange(4, 4);
    SimulationRange haloRange = SimulationRange(6, 6);

    float expected[24] = {10, 14, 18, 22, 26, 23, 29, 40, 45, 50, 55, 47, 53, 70, 75, 80, 85, 71, 52, 74, 78, 82, 86, 65};

    chunk(coreRange, chunkRange, haloRange, expected, "process2DWithLeftoverChunk");
}

void process2DWithEqualChunksNoLeftover() {
    SimulationRange coreRange = SimulationRange(6, 4);
    SimulationRange chunkRange = SimulationRange(2, 4);
    SimulationRange haloRange = SimulationRange(4, 6);

    float expected[24] = {10, 14, 18, 22, 26, 23, 29, 40, 45, 50, 55, 47, 53, 70, 75, 80, 85, 71, 52, 74, 78, 82, 86, 65};

    chunk(coreRange, chunkRange, haloRange, expected, "process2DWithEqualChunksNoLeftover");
}

void process2DProblemThatFitsInSingleChunk() {
    SimulationRange coreRange = SimulationRange(6, 4);
    SimulationRange chunkRange = SimulationRange(6, 4);
    SimulationRange haloRange = SimulationRange(8, 6);

    float expected[24] = {10, 14, 18, 22, 26, 23, 29, 40, 45, 50, 55, 47, 53, 70, 75, 80, 85, 71, 52, 74, 78, 82, 86, 65};

    chunk(coreRange, chunkRange, haloRange, expected, "process2DProblemThatFitsInSingleChunk");
}

int main(void) {
    process2DWithLeftoverChunk();
    process2DWithEqualChunksNoLeftover();
    process2DProblemThatFitsInSingleChunk();
}
#include "chunkingTest.h"

#include <iostream>
#include <string>

void process1DWithEqualChunksNoLeftover() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(2);
    SimulationRange haloRange = SimulationRange(4);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "process1DWithEqualChunksNoLeftover");
}

void process1DWithLeftoverChunk() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(4);
    SimulationRange haloRange = SimulationRange(6);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "process1DWithLeftoverChunk");
}

void process1DProblemThatFitsInSingleChunk() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(10);
    SimulationRange haloRange = SimulationRange(12);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "process1DProblemThatFitsInSingleChunk");
}

int main(void) {
    process1DWithEqualChunksNoLeftover();
    process1DProblemThatFitsInSingleChunk();
    process1DWithLeftoverChunk();
}
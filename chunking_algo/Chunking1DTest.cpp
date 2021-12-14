#include "chunking_test_helpers.h"

#include <iostream>
#include <string>

void chunk1DUsingChunkSize2() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(2);
    SimulationRange haloRange = SimulationRange(4);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "chunk1DUsingChunkSize2");
}

void chunk1DWithLeftoverChunkSize() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(4);
    SimulationRange haloRange = SimulationRange(6);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "chunk1DWithLeftoverChunkSize");
}

void process1DInFullWithoutChunking() {
    SimulationRange coreRange = SimulationRange(10);
    SimulationRange chunkRange = SimulationRange(10);
    SimulationRange haloRange = SimulationRange(12);

    float expected[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 19};

    chunk(coreRange, chunkRange, haloRange, expected, "process1DInFullWithoutChunking");
}

int main(void) {
    chunk1DUsingChunkSize2();
    chunk1DWithLeftoverChunkSize();
    process1DInFullWithoutChunking();
}
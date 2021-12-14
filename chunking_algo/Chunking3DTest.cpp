#include "chunking_test_helpers.h"

#include <iostream>
#include <string>

void process3DWith2ChunksInIDirection() {
    SimulationRange coreRange = SimulationRange(4, 4, 3);
    SimulationRange chunkRange = SimulationRange(2, 4, 3);
    SimulationRange haloRange = SimulationRange(4, 6, 5);

    float expected[48] = {24, 28, 32, 31, 37, 46, 51, 47, 53, 66, 71, 63, 52, 68, 72, 59, 73, 
                          94, 99, 83, 106, 132, 138, 119, 126, 156, 162, 139, 113, 146, 151, 
                          123, 88, 124, 128, 95, 133, 174, 179, 143, 149, 194, 199, 159, 116, 
                          164, 168, 123};

    chunk(coreRange, chunkRange, haloRange, expected, "process3DWith2ChunksInIDirection");
}

void process3DWith3ChunksInIDirection() {
    SimulationRange coreRange = SimulationRange(6, 4, 3);
    SimulationRange chunkRange = SimulationRange(2, 4, 3);
    SimulationRange haloRange = SimulationRange(4, 6, 5);

    float expected[72] = {34, 38, 42, 46, 50, 47, 53, 64, 69, 74, 79, 71, 77, 94, 99, 104, 
                          109, 95, 76, 98, 102, 106, 110, 89, 107, 136, 141, 146, 151, 125, 
                          156, 192, 198, 204, 210, 179, 186, 228, 234, 240, 246, 209, 167, 
                          214, 219, 224, 229, 185, 130, 182, 186, 190, 194, 143, 197, 256, 
                          261, 266, 271, 215, 221, 286, 291, 296, 301, 239, 172, 242, 246, 
                          250, 254, 185};

    chunk(coreRange, chunkRange, haloRange, expected, "process3DWith3ChunksInIDirection");
}

void process3DWithLeftoverChunkInIDirection() {
    SimulationRange coreRange = SimulationRange(6, 4, 3);
    SimulationRange chunkRange = SimulationRange(4, 4, 3);
    SimulationRange haloRange = SimulationRange(6, 6, 5);

    float expected[72] = {34, 38, 42, 46, 50, 47, 53, 64, 69, 74, 79, 71, 77, 94, 99, 104, 
                          109, 95, 76, 98, 102, 106, 110, 89, 107, 136, 141, 146, 151, 125, 
                          156, 192, 198, 204, 210, 179, 186, 228, 234, 240, 246, 209, 167, 
                          214, 219, 224, 229, 185, 130, 182, 186, 190, 194, 143, 197, 256, 
                          261, 266, 271, 215, 221, 286, 291, 296, 301, 239, 172, 242, 246, 
                          250, 254, 185};

    chunk(coreRange, chunkRange, haloRange, expected, "process3DWithLeftoverChunkInIDirection");
}

void process3DProblemThatFitsInSingleChunk() {
    SimulationRange coreRange = SimulationRange(4, 4, 3);
    SimulationRange chunkRange = SimulationRange(4, 4, 3);
    SimulationRange haloRange = SimulationRange(6, 6, 5);

    float expected[48] = {24, 28, 32, 31, 37, 46, 51, 47, 53, 66, 71, 63, 52, 68, 72, 59, 73, 
                          94, 99, 83, 106, 132, 138, 119, 126, 156, 162, 139, 113, 146, 151, 
                          123, 88, 124, 128, 95, 133, 174, 179, 143, 149, 194, 199, 159, 116, 
                          164, 168, 123};

    chunk(coreRange, chunkRange, haloRange, expected, "process3DProblemThatFitsInSingleChunk");
}

int main(void) {
    process3DWith2ChunksInIDirection();
    process3DWith3ChunksInIDirection();
    process3DWithLeftoverChunkInIDirection();
    process3DProblemThatFitsInSingleChunk();
}
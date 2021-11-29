#include "Simulation.h"

#include <iostream>

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    OCLSetup(deviceType, programFileName, kernelName);
}

void Simulation::ChunkAndCompute(cl_float2 * p, 
                                 int iDim, int jDim, int kDim, 
                                 int iChunk, int jChunk, int kChunk) {
    InitializeSimulationArea(p, iDim, jDim, kDim, iChunk, jChunk, kChunk);
    CheckSpecifiedChunkSize();
    RunSimulation();
}

void Simulation::InitializeSimulationArea(cl_float2 * p, int iDim, int jDim, int kDim, 
                                        int iChunk, int jChunk, int kChunk) {
    simulationArea.p = p;
    simulationArea.iDim = iDim;
    simulationArea.jDim = jDim;
    simulationArea.kDim = kDim;
    simulationArea.iChunk = iChunk;
    simulationArea.jChunk = jChunk;
    simulationArea.kChunk = kChunk;
    simulationArea.iHalChunk = iChunk + 2; // hardcoded halo for now
    simulationArea.jHalChunk = jChunk + 2;
    simulationArea.kHalChunk = kChunk + 2;
    simulationArea.halloedSize = simulationArea.iHalChunk * simulationArea.jHalChunk * simulationArea.kHalChunk;
}

void Simulation::CheckSpecifiedChunkSize() {
    long maxMem = oclSetup.deviceProperties.maxMemAllocSize * 0.95;
    // size of the 4D twinned buffer
    int chunkSize = simulationArea.halloedSize * sizeof(long) * 2;
    if (chunkSize > maxMem) {
        std::cout << "Provided chunk size exceeds device's memory allocation size. Chunk size: " 
                  << chunkSize << " available memory allocation size: " 
                  << maxMem << ".\n Chunk size will be determined automatically." << std::endl; 
        // TODO: determine chunk size;
    }
}

void Simulation::RunSimulation() {

}



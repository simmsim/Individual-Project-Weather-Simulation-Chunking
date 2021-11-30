#include "Simulation.h"

#include <iostream>

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    OCLSetup(deviceType, programFileName, kernelName);
}

void Simulation::ChunkAndCompute(cl_float2 * p, int halo, SimulationRange coreDimensions, SimulationRange chunkDimensions) {
    InitializeSimulationArea(p, halo, coreDimensions, chunkDimensions);
    // TODO: check that coreDimensions and chunkDimensions makes sense.
    // For example, chunk dim is not larger than core dim; 
    CheckSpecifiedChunkSize();
    RunSimulation();
}

void Simulation::InitializeSimulationArea(cl_float2 * p, int halo, SimulationRange coreDimensions, SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.halo = halo;
    simulationArea.coreDimensions = coreDimensions;
    simulationArea.chunkDimensions = chunkDimensions;

    // TODO: will need to come back to this... 2D and 1D problems don't need halos in all directions
    int iHalChunk = chunkDimensions.getDimSizes()[0] + halo;
    int jHalChunk = chunkDimensions.getDimSizes()[1] + halo;
    int kHalChunk = chunkDimensions.getDimSizes()[2] + halo;
    simulationArea.halChunkDimensions = SimulationRange(iHalChunk, jHalChunk, kHalChunk);
}

void Simulation::CheckSpecifiedChunkSize() {
    long maxMem = oclSetup.deviceProperties.maxMemAllocSize * 0.95;
    // size of the 4D twinned buffer
    int chunkSize = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(long) * 2;
    if (chunkSize > maxMem) {
        std::cout << "Provided chunk size exceeds device's memory allocation size. Chunk size: " 
                  << chunkSize << " available memory allocation size: " 
                  << maxMem << ".\n Chunk size will be determined automatically." << std::endl; 
        // TODO: determine chunk size;
    }
}

void Simulation::RunSimulation() {
    
}



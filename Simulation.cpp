#include "Simulation.h"

#include <iostream>

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    OCLSetup(deviceType, programFileName, kernelName);
}

void Simulation::RunSimulation(cl_float * p, int halo, SimulationRange coreDimensions, 
                                 SimulationRange chunkDimensions = SimulationRange()) {
    InitializeSimulationArea(p, halo, coreDimensions, chunkDimensions);
    CheckSpecifiedChunkSize();
    ChunkAndCompute();
}

void Simulation::InitializeSimulationArea(cl_float * p, int halo, SimulationRange coreDimensions, 
                                          SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.halo = halo;
    simulationArea.coreDimensions = coreDimensions;
    simulationArea.chunkDimensions = (chunkDimensions.getDimensions() == 0) ? SimulationRange(coreDimensions) : chunkDimensions;
    simulationArea.halChunkDimensions = SimulationRange(chunkDimensions);
    simulationArea.halChunkDimensions.incrementDimensionsBy(halo);
}

void Simulation::CheckSpecifiedChunkSize() {
    long maxMem = oclSetup.deviceProperties.maxMemAllocSize * 0.95;
    int chunkSize = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(long);
    if (chunkSize > maxMem) {
        std::cout << "Provided chunk size exceeds device's memory allocation size. Chunk size: " 
                  << chunkSize << " available memory allocation size: " << maxMem; 
    } else if (ChunkExceedsCoreDimensions()) {
        std::cout << "Provided chunk dimensions exceed core dimensions. ";
    } else {
        // Provided chunk size was appropriate; continue
        return;
    }

    std::cout << "\nNew chunk size will be determined automatically.\n";
    ReconfigureChunkSize();
}

bool Simulation::ChunkExceedsCoreDimensions() {
    int coreDimensions = simulationArea.coreDimensions.getDimensions();
    int chunkDimensions = simulationArea.chunkDimensions.getDimensions();

    if (coreDimensions != chunkDimensions) {
        std::cout << "Core and chunk must have the same dimensions, but provided core dimensions {"
                  << coreDimensions << "} and chunk dimensions {" << chunkDimensions << "} are different. "
                  << "Terminating." << std::endl;
        exit(EXIT_FAILURE);
    }

    int * coreDimSizes = simulationArea.coreDimensions.getDimSizes();
    int * chunkDimSizes = simulationArea.chunkDimensions.getDimSizes();
    for (int i = 0; i < coreDimensions; i++) {
        if (chunkDimSizes[i] > coreDimSizes[i]) {
            return true;
        }
    }

    return false;
}

void Simulation::ReconfigureChunkSize() {

}

void Simulation::ChunkAndCompute() {
    
}
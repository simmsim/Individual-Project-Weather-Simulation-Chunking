#include "Simulation.h"

#include <iostream>

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    OCLSetup(deviceType, programFileName, kernelName);
}

// TODO: rename this method to RunSimulation and that other method inside of it to ChunkAndCompute!
void Simulation::ChunkAndCompute(cl_float * p, int halo, SimulationRange coreDimensions, 
                                 SimulationRange chunkDimensions = SimulationRange()) {
    InitializeSimulationArea(p, halo, coreDimensions, chunkDimensions);
    CheckSpecifiedChunkSize();
    RunSimulation();
}

void Simulation::InitializeSimulationArea(cl_float * p, int halo, SimulationRange coreDimensions, 
                                          SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.halo = halo;
    simulationArea.coreDimensions = coreDimensions;
    simulationArea.chunkDimensions = (chunkDimensions.getDimSizes() == 0) ? SimulationRange(coreDimensions) : chunkDimensions;
    simulationArea.halChunkDimensions = SimulationRange(chunkDimensions);
    simulationArea.halChunkDimensions.incrementDimensionsBy(halo);
}

void Simulation::CheckSpecifiedChunkSize() {
    long maxMem = oclSetup.deviceProperties.maxMemAllocSize * 0.95;
    int chunkSize = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(long);
    if (chunkSize > maxMem) {
        std::cout << "Provided chunk size exceeds device's memory allocation size. Chunk size: " 
                  << chunkSize << " available memory allocation size: " 
                  << maxMem << ".\n New chunk size will be determined automatically." << std::endl; 
    } else if (ChunkExceedsCoreDimensions()) {
        std::cout << "Provided chunk dimensions exceed core dimensions. "
                  << "New chunk size will be determined automatically." << std::endl;
    } else {
        // Provided chunk size was appropriate; continue
        return;
    }

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

void Simulation::RunSimulation() {
    
}
#include "../Simulation.h"

#include <iostream>
#include <string>

int deviceType = CPU;
int halo = 1; // 1-point halo
int iterations = 1;
char * programFileName = (char*) "../sor_kernel.cl";
char * kernelName = (char*) "sor_superkernel";

void processingHaltsIfChunkDimsExceedCoreDims() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4);
    SimulationRange chunkRange = SimulationRange(2, 4);
    Simulation simulation = Simulation(deviceType, programFileName, kernelName);
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    // initialize array with simple values
    for (int idx = 0; idx < pSize; idx++) {
        p[idx] = idx + 1;
    }

    // ACT
    simulation.RunSimulation(p, halo, iterations, coreRange, chunkRange);

    // ASSERT

    // TEARDOWN
}


int main(void) {
    processingHaltsIfChunkDimsExceedCoreDims();
    
}
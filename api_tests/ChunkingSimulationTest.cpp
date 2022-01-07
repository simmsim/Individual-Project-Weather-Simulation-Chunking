#include "../Simulation.h"
#include "../test_helpers/assertions.h"

#include <iostream>
#include <string>

int deviceType = CPU;
int halo = 1; // 1-point halo
int iterations = 1;
char * programFileName = (char*) "../sor_kernel.cl";
char * kernelName = (char*) "sor_superkernel";

void processingHaltsIfChunkDimsExceedCoreDims() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(2, 4, 2);
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

// EXPECTED VALUES WILL HAVE TO BE UPDATED ONCE CORE CALC KERNEL IS DONE
void testProcessingSimpleCase() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(2, 4, 2);
    Simulation simulation = Simulation(deviceType, programFileName, kernelName);
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    // initialize array with simple values
    for (int idx = 0; idx < pSize; idx++) {
        p[idx] = idx + 1;
    }

    float expected[32] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
                            18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

    // ACT
    simulation.RunSimulation(p, halo, iterations, coreRange, chunkRange);

    // ASSERT
    assertEquals(expected, simulation.simulationArea.p, coreRange.getSimulationSize(), "testProcessingSimpleCase");

    // TEARDOWN
    free(p);
    simulation.simulationArea.p = NULL;
}


int main(void) {
    testProcessingSimpleCase();
    
}
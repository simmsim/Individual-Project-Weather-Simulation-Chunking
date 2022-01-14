#include "../Simulation.h"
#include "../utility/assertions.h"

#include <iostream>
#include <string>

int deviceType = CPU;
int halo = 1; // 1-point halo
int iterations = 1;
char * programFileName = (char*) "../sor_kernel.cl";
char * kernelName = (char*) "sor_superkernel";

Simulation initializeAndRun(SimulationRange coreRange, SimulationRange chunkRange,
                         float maxSimulationAreaMemUsage) {
    Simulation simulation = Simulation(deviceType, programFileName, kernelName);
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    for (int idx = 0; idx < pSize; idx++) {
        p[idx] = idx + 1;
    }

    simulation.RunSimulation(p, halo, iterations, maxSimulationAreaMemUsage, coreRange, chunkRange);    
    return simulation;
}

void testProcessingHaltsIfChunkDimsExceedCoreDims() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(3, 4, 2);
    float maxSimulationAreaMemUsage = 0.00004;

    // EXPECTATIONS
    float expectedSimulationArea[32] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
                            18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
    int expectedDimensionValue = 2;

    // ACT
    Simulation simulation = initializeAndRun(coreRange, chunkRange, maxSimulationAreaMemUsage);

    // ASSERT
    SimulationRange newChunkRange = simulation.getSimulationArea().chunkDimensions;
    int actualDimensionValue = newChunkRange.getDimSizes()[0];
    float * actualSimulationArea = simulation.getSimulationArea().p;

    assertEquals(expectedSimulationArea, actualSimulationArea, coreRange.getSimulationSize(), 
                "processingHaltsIfChunkDimsExceedCoreDims: simulation area has been computed correctly");
    assertEquals(expectedDimensionValue, actualDimensionValue, 
                "processingHaltsIfChunkDimsExceedCoreDims: new chunk dimensions was re-computed correctly.");

    // TEARDOWN
    free(simulation.getSimulationArea().p);
}

// EXPECTED VALUES WILL HAVE TO BE UPDATED ONCE CORE CALC KERNEL IS DONE
void testProcessingSimpleCase() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(2, 4, 2);
    float maxSimulationAreaMemUsage = 95;

    float expectedSimulationArea[32] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
                            18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

    // ACT
    Simulation simulation = initializeAndRun(coreRange, chunkRange, maxSimulationAreaMemUsage);

    // ASSERT
    float * actualSimulationArea = simulation.getSimulationArea().p;
    assertEquals(expectedSimulationArea, actualSimulationArea, coreRange.getSimulationSize(), "testProcessingSimpleCase");

    // TEARDOWN
    free(simulation.getSimulationArea().p);
}

int main(void) {
    testProcessingHaltsIfChunkDimsExceedCoreDims();
    testProcessingSimpleCase();

    return 0;
}
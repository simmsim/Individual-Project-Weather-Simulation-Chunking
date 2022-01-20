#include "../Simulation.h"
#include "../utility/assertions.h"
#include "../utility/domain_generator.h"

#include <iostream>
#include <string>

int deviceType = CPU;
int halo = 1; // 1-point halo
char * programFileName = (char*) "../sor_kernel.cl";
char * kernelName = (char*) "sor_superkernel";

Simulation initializeAndRun(float * p, float * rhs,
                            SimulationRange coreRange, SimulationRange chunkRange,
                            float maxSimulationAreaMemUsage, int iterations) {
    int err;
    Simulation simulation = Simulation(deviceType, programFileName, kernelName, &err);
    int pSize = coreRange.getSimulationSize();

    generateDomain(p, rhs, coreRange);
    simulation.RunSimulation(p, rhs, halo, iterations, maxSimulationAreaMemUsage, coreRange, chunkRange);   
    return simulation;
}

void testRecomputeChunksFailure() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(3, 4, 2);
    float maxSimulationAreaMemUsage = 0.00004;
    int iterations = 1;
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // EXPECTED
    int expectedDimensionValue = 3;

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, coreRange, chunkRange, maxSimulationAreaMemUsage, iterations);

    // ASSERT
    SimulationRange newChunkRange = simulation.getSimulationArea().chunkDimensions;
    int actualDimensionValue = newChunkRange.getDimSizes()[0];

    assertEquals(expectedDimensionValue, actualDimensionValue, 
                "testRecomputeChunksFailure: chunk has not been recalculated due to failure.");

    // TEARDOWN
    free(p);
    free(rhs);
}

void testChunkDimensionsRecalculated() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(3, 4, 2);
    float maxSimulationAreaMemUsage = 0.00006;
    int iterations = 3;
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // EXPECTED
    int expectedDimensionValue = 2;
    float expectedValue = -0.0768519;

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, coreRange, chunkRange, maxSimulationAreaMemUsage, iterations);

    // ASSERT
    SimulationRange newChunkRange = simulation.getSimulationArea().chunkDimensions;
    int actualDimensionValue = newChunkRange.getDimSizes()[0];
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualValue =  actualSimulationArea[F3D2C(4, 4, 0,0,0, 4/2,4/2,2/2)];

    assertEquals(expectedDimensionValue, actualDimensionValue, 
                "testChunkDimensionsRecalculated: new chunk dimensions was re-computed correctly.");

    assertEquals(expectedValue, actualValue, 
                "testChunkDimensionsRecalculated: simulation area has been computed correctly.");

    // TEARDOWN
    free(p);
    free(rhs);
}

void testProcessingSimpleCase() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(4, 4, 2);
    SimulationRange chunkRange = SimulationRange(2, 4, 2);
    int iterations = 3;
    float maxSimulationAreaMemUsage = 95;
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // EXPECTED
    float expectedValue = -0.0768519;

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, coreRange, chunkRange, maxSimulationAreaMemUsage, iterations);

    // ASSERT
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualValue =  actualSimulationArea[F3D2C(4, 4, 0,0,0,4/2,4/2,2/2)];
    assertEquals(expectedValue, actualValue, "testProcessingSimpleCase");

    // TEARDOWN
    free(p);
    free(rhs);
}

int main(void) {
    testChunkDimensionsRecalculated();
    testProcessingSimpleCase();
    testRecomputeChunksFailure();
    
    return 0;
}
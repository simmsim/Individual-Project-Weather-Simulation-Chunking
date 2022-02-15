#include "../Simulation.h"
#include "../utility/assertions.h"
#include "../utility/domain_generator.h"

#include <iostream>
#include <string>
#include <chrono>

int deviceType = CPU;
int halo = 1; // 1-point halo
char * programFileName = (char*) "../sor_kernel.cl";
char * kernelName = (char*) "sor_superkernel";

Simulation initializeAndRun(float * p, float * rhs,
                            SimulationRange simulationRange, SimulationRange chunkRange,
                            float maxSimulationAreaMemUsage, int iterations) {
    generateDomain(p, rhs, simulationRange);

    int err;
    Simulation simulation = Simulation(deviceType, programFileName, kernelName, &err);
    simulation.RunSimulation(p, rhs, halo, iterations, maxSimulationAreaMemUsage, simulationRange, chunkRange);

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
    SimulationRange coreRange = SimulationRange(6, 6, 4);
    SimulationRange chunkRange = SimulationRange(3, 4, 2);
    float maxSimulationAreaMemUsage = 0.00006;
    int iterations = 3;
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // EXPECTED
    int expectedDimensionValue = 2;
    float expectedValue = -0.057626;

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, coreRange, chunkRange, maxSimulationAreaMemUsage, iterations);

    // ASSERT
    SimulationRange newChunkRange = simulation.getSimulationArea().chunkDimensions;
    int actualDimensionValue = newChunkRange.getDimSizes()[0];
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualValue =  actualSimulationArea[F3D2C(6, 6, 0,0,0, 1, 1, 1)];

    assertEquals(expectedDimensionValue, actualDimensionValue, 
                "testChunkDimensionsRecalculated: new chunk dimensions was re-computed correctly.");

    assertEquals(expectedValue, actualValue, 
                "testChunkDimensionsRecalculated: simulation area has been computed correctly.");

    // TEARDOWN
    free(p);
    free(rhs);
}

void testChunkingSetup(SimulationRange chunkRange, std::string testName) {
    // ARRANGE
    SimulationRange simulationRange = SimulationRange(152, 152, 92);
    int iterations = 5;
    float maxSimulationAreaMemUsage = 99;
    int pSize = simulationRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, simulationRange, chunkRange, maxSimulationAreaMemUsage, iterations);

    // EXPECTED
    float expectedTopLeftValue = -0.166636;
    float expectedBottomRightValue = -0.180353;
    
    // ASSERT
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualTopLeftValue =  actualSimulationArea[F3D2C(152, 152, 0,0,0, 1,1,1)];
    float actualBottomRightValue =  actualSimulationArea[F3D2C(152, 152, 0,0,0, 150,150,1)];

    assertEquals(expectedTopLeftValue, actualTopLeftValue, testName + ": check for top left value");
    assertEquals(expectedBottomRightValue, actualBottomRightValue, testName + ": check for bottom right value");

    // TEARDOWN
    free(p);
    free(rhs);
}

void testProcessingFullSimulationWithoutChunking() {
    SimulationRange chunkRange = SimulationRange(150, 150, 90);
    testChunkingSetup(chunkRange, "testProcessingFullSimulationWithoutChunking");
}

void testProcessingSimulationInEqualChunks() {
    SimulationRange chunkRange = SimulationRange(75, 150, 90);
    testChunkingSetup(chunkRange, "testProcessingSimulationInEqualChunks");
}

void testProcessingSimulationWithLeftoverChunk() {
    SimulationRange chunkRange = SimulationRange(100, 150, 90);
    testChunkingSetup(chunkRange, "testProcessingSimulationWithLeftoverChunk");
}

int main(void) {
    testProcessingFullSimulationWithoutChunking();
    testProcessingSimulationInEqualChunks();
    testProcessingSimulationWithLeftoverChunk();
    testChunkDimensionsRecalculated();
    testRecomputeChunksFailure();
    
    return 0;
}
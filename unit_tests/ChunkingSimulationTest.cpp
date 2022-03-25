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
                            int iterations, float maxSimulationAreaMemUsage = 100) {
    generateDomain(p, rhs, simulationRange);

    int err;
    Simulation simulation = Simulation(deviceType, programFileName, kernelName, &err);
    simulation.RunSimulation(p, rhs, halo, iterations, simulationRange, chunkRange, maxSimulationAreaMemUsage);

    return simulation;
}

void testChunkDimensionsRecalculated() {
    // ARRANGE
    SimulationRange coreRange = SimulationRange(152, 152, 92);
    SimulationRange chunkRange = SimulationRange(150, 150, 90);
    int iterations = 5;
    float maxSimulationAreaMemUsage = 0.09;
    int pSize = coreRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // EXPECTED
    // the dimension vlaue calculated will be machine specific, since calculation is based on the amount
    // of available data
    int expectedDimensionValue = 30;
    float expectedValue = -0.165622;

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, coreRange, chunkRange, iterations, maxSimulationAreaMemUsage);

    // ASSERT
    SimulationRange newChunkRange = simulation.getSimulationArea().chunkDimensions;
    int actualDimensionValue = newChunkRange.getDimSizes()[0];
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualValue =  actualSimulationArea[F3D2C(152, 152, 0,0,0, 1, 1, 1)];

    assertEquals(expectedDimensionValue, actualDimensionValue, 
                "testChunkDimensionsRecalculated: new chunk dimensions was re-computed correctly.");

    assertEquals(expectedValue, actualValue, 
                "testChunkDimensionsRecalculated: simulation area has been computed correctly.");

    // TEARDOWN
    free(actualSimulationArea);
    free(rhs);
}

void testChunkingSetup(SimulationRange chunkRange, std::string testName) {
    // ARRANGE
    SimulationRange simulationRange = SimulationRange(152, 152, 92);
    int iterations = 5;
    int pSize = simulationRange.getSimulationSize();
    float *p = (float*)malloc(sizeof(float)*pSize);
    float *rhs = (float*)malloc(sizeof(float)*pSize);

    // ACT
    Simulation simulation = initializeAndRun(p, rhs, simulationRange, chunkRange, iterations);
    // EXPECTED
    float expectedTopLeftValue = -0.165622;
    float expectedBottomRightValue = -0.180549;
    
    // ASSERT
    float * actualSimulationArea = simulation.getSimulationArea().p;
    float actualTopLeftValue =  actualSimulationArea[F3D2C(152, 152, 0,0,0, 1,1,1)];
    float actualBottomRightValue =  actualSimulationArea[F3D2C(152, 152, 0,0,0, 150,150,1)];

    assertEquals(expectedTopLeftValue, actualTopLeftValue, testName + ": check for top left value");
    assertEquals(expectedBottomRightValue, actualBottomRightValue, testName + ": check for bottom right value");

    // TEARDOWN
    free(actualSimulationArea);
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
    
    return 0;
}
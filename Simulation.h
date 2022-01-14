#include "OCLSetup.h"
#include "SimulationRange.h"

#include <CL/opencl.hpp>

typedef struct simulationAreaStruct {
    cl_float *p;
    cl_float *rhs;
    int halo;
    int iterations;
    SimulationRange coreDimensions;
    SimulationRange chunkDimensions;
    SimulationRange halChunkDimensions;
} simulationAreaStruct;

class Simulation {
    private:
        OCLSetup oclSetup;
        simulationAreaStruct simulationArea;

        void InitializeSimulationArea(cl_float * p, cl_float * rhs, int halo, int iterations, 
                                      SimulationRange coreDimensions, SimulationRange chunkDimensions);
        void CheckChunkDimensions();
        void CheckSpecifiedChunkSize(float maxSimulationAreaMemUsage);
        void ReconfigureChunkSize(long maxMem);
        void ChunkAndCompute();
        void EnqueueKernel(int type, cl::NDRange range);

    public:        
        Simulation(int deviceType, char * programFileName,
                    char * kernelName);

        void RunSimulation(cl_float * p, cl_float * rhs, int halo, int iterations,
                           float maxSimulationAreaMemUsage,
                           SimulationRange coreDimensions,
                           SimulationRange chunkDimensions);

        const simulationAreaStruct & getSimulationArea() const {
            return simulationArea;
        }
};
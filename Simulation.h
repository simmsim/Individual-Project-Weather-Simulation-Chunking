#include "OCLSetup.h"
#include "SimulationRange.h"
#include "ErrorCodes.h"

#include <CL/opencl.hpp>

typedef struct simulationAreaStruct {
    cl_float *p;
    cl_float *rhs;
    int halo;
    int iterations;
    SimulationRange simulationDimensions;
    SimulationRange chunkDimensions;
    SimulationRange halChunkDimensions;
} simulationAreaStruct;

class Simulation {
    private:
        OCLSetup oclSetup;
        simulationAreaStruct simulationArea;

        // Simulation dimensions are core + halo; chunk dimensions specify how to chunk core area (without halo)
        void InitializeSimulationArea(cl_float * p, cl_float * rhs, int halo, int iterations, 
                                      SimulationRange simulationDimensions, SimulationRange chunkDimensions);
        int CheckChunkDimensions();
        int CheckSpecifiedChunkSize(float maxSimulationAreaMemUsage);
        int ReconfigureChunkSize(long maxMem);
        void ProcessSimulation();
        void ComputeFullSimulation();
        void ChunkAndCompute();
        void CallKernel(cl::Buffer in_p, cl::Buffer out_p);
        void EnqueueKernel(int type, cl::NDRange range);

    public:        
        Simulation(int deviceType, char * programFileName,
                    char * kernelName, int * err);

        int RunSimulation(cl_float * p, cl_float * rhs, int halo, int iterations,
                           float maxSimulationAreaMemUsage,
                           SimulationRange simulationDimensions,
                           SimulationRange chunkDimensions);

        const simulationAreaStruct & getSimulationArea() const {
            return simulationArea;
        }
};
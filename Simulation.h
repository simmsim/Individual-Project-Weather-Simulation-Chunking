#include "OCLSetup.h"
#include "SimulationRange.h"
#include "ErrorCodes.h"

#include <CL/opencl.hpp>
#include <vector>

typedef struct SimulationAreaStruct {
    cl_float *p;
    cl_float *rhs;
    int halo;
    int iterations;
    SimulationRange simulationDimensions;
    SimulationRange chunkDimensions;
    SimulationRange halChunkDimensions;
} SimulationAreaStruct;

typedef struct PerformanceMeasurementsStruct {
    std::vector<double> clWriteToDevice;
    std::vector<double> clKernelExecution;
    std::vector<double> clReadFromDevice;

    std::vector<long> constructChunk;
    std::vector<long> reintegrateChunk;
} PerformanceMeasurementsStruct;

class Simulation {
    private:
        OCLSetup oclSetup;
        SimulationAreaStruct simulationArea;
        PerformanceMeasurementsStruct performanceMeasurements;

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

        void MeasureEventPerformance(cl::Event event, std::vector<double>& measurementVec);

    public:        
        Simulation(int deviceType, char * programFileName,
                    char * kernelName, int * err);

        int RunSimulation(cl_float * p, cl_float * rhs, int halo, int iterations,
                           float maxSimulationAreaMemUsage,
                           SimulationRange simulationDimensions,
                           SimulationRange chunkDimensions);

        const SimulationAreaStruct & getSimulationArea() const {
            return simulationArea;
        }

        const PerformanceMeasurementsStruct & getPerformanceMeasurements() const {
            return performanceMeasurements;
        }
};
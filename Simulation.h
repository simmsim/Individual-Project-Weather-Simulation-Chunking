#include "OCLSetup.h"
#include "SimulationRange.h"

#include <CL/opencl.hpp>

typedef struct simulationAreaStruct {
    cl_float *p;
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

        void InitializeSimulationArea(cl_float * p, int halo, int iterations, 
                                      SimulationRange coreDimensions, SimulationRange chunkDimensions);
        void CheckSpecifiedChunkSize();
        bool ChunkExceedsCoreDimensions();
        void ReconfigureChunkSize(long maxMem, long requiredMem);
        void ChunkAndCompute();

    public:        
        Simulation(int deviceType, char * programFileName,
                    char * kernelName);

        // might split this up
        // TODO: deal with loop ordering? does user specify 
        // loop ordering expliclty?
        // TODO: is it always 1 point halo or do we generalize and allow
        // the user to specify the n-point halo?
        void RunSimulation(cl_float * p, int halo, int iterations,
                             SimulationRange coreDimensions,
                             SimulationRange chunkDimensions);

        const simulationAreaStruct & getSimulationArea() const {
            return simulationArea;
        }
};
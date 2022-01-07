#include "OCLSetup.h"
#include "SimulationRange.h"
//#include "errorHelper.h"

#define CL_HPP_TARGET_OPENCL_VERSION 200

#include <CL/opencl.hpp>

class Simulation {
    private:
        OCLSetup oclSetup;

        void InitializeSimulationArea(cl_float * p, int halo, int iterations, 
                                      SimulationRange coreDimensions, SimulationRange chunkDimensions);
        void CheckSpecifiedChunkSize();
        bool ChunkExceedsCoreDimensions();
        void ReconfigureChunkSize(long maxMem);
        void ChunkAndCompute();

    public:
        // does this need to be public??
        struct simulationArea {
            cl_float *p;
            int halo;
            int iterations;
            SimulationRange coreDimensions;
            SimulationRange chunkDimensions;
            SimulationRange halChunkDimensions;
        } simulationArea;

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
};
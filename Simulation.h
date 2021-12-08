#include "OCLSetup.h"
#include "SimulationRange.h"

#include <CL/opencl.hpp>

class Simulation {
    private:
        OCLSetup oclSetup;

    public:
        struct simulationArea {
            cl_float *p;
            int halo;
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
        void ChunkAndCompute(cl_float * p, int halo,
                             SimulationRange coreDimensions,
                             SimulationRange chunkDimensions);

        void InitializeSimulationArea(cl_float * p, int halo, SimulationRange coreDimensions,
                            SimulationRange chunkDimensions);

        void CheckSpecifiedChunkSize();
        bool ChunkExceedsCoreDimensions();
        void ReconfigureChunkSize();
        void RunSimulation();

};
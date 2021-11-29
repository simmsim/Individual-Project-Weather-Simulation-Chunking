#include "OCLSetup.h"
#include "SimulationRange.h"

#include <CL/opencl.hpp>

class Simulation {
    private:
        OCLSetup oclSetup;

    public:
        struct simulationArea {
            cl_float2 *p;
            int iDim, jDim, kDim;
            int iChunk, jChunk, kChunk;
            int iHalChunk, jHalChunk, kHalChunk;
            int halo;
            int halloedSize;
        } simulationArea;

        Simulation(int deviceType, char * programFileName,
                    char * kernelName);

        // might split this up
        // TODO: deal with loop ordering? does user specify 
        // loop ordering expliclty?
        // TODO :for dimensions (simulation's and chunk's), could do something similar do NDRange?
        // TODO: is it always 1 point halo or do we generalize and allow
        // the user to specify the n-point halo?
        void ChunkAndCompute(cl_float2 * p, 
                            int iDim, int jDim, int kDim, 
                            int iChunk, int jChunk, int kChunk);

        void InitializeSimulationArea(cl_float2 * p, int iDim, int jDim, int kDim, 
                                    int iChunk, int jChunk, int kChunk);

        void CheckSpecifiedChunkSize();

        void RunSimulation();

};
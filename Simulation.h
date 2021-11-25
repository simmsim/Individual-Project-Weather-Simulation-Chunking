#include "OCLSetup.h"

class Simulation {
    private:
        OCLSetup oclSetup;

    public:
        Simulation(int deviceType, char * programFileName,
                    char * kernelName);


        // might split this up
        // TODO: deal with loop ordering? does user specify 
        // loop ordering expliclty?
        // TODO :for dimensions (simulation's and chunk's), could do something similar do NDRange?
        // TODO: is it always 1 point halo or do we generalize and allow
        // the user to specify the n-point halo?
        void ChunkAndCompute(float * p1, float * p2,
        int iDim, int jDim, int kDim, int iChunk, int jChunk, 
        int kChunk);
        
};
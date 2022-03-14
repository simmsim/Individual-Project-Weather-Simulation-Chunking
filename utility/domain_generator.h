#include "array_index_f2c.h"

// Populating arrays with values in the same manner as C reference; 
// needed to compare results;

void generateDomain(float * p0, float * rhs, SimulationRange simulationRange) {
    int iDim = simulationRange.getDimSizes()[0];
    int jDim = simulationRange.getDimSizes()[1];
    int kDim = simulationRange.getDimSizes()[2];

    //Simple numbers that are easy to reason about when testing.
    //  for (int z = 0; z < simulationRange.getSimulationSize(); z++) {
    //      p0[z] = z;
    //     // std::cout << " " << p0[z];
    //  }
    //std::cout << "\n\n";

    for (int i = 0;i < iDim;i += 1) {
        for (int j = 0;j < jDim;j += 1) {
            for (int k = 0;k < kDim;k += 1) {
                rhs[F3D2C(iDim,jDim,0,0,0,i,j,k)] = 0.1+((float)(i+1)*(j+1)*(k+1))/((float)(iDim)*(jDim)*(kDim));
                p0[F3D2C(iDim,jDim,0,0,0,i,j,k)] = ((float)(i+1)*(j+1)*(k+1))/((float)(iDim)*(jDim)*(kDim));
                //std::cout << " " << p0[F3D2C(iDim,jDim,0,0,0,i,j,k)];
            }
        }
    }
    //std::cout << "\n\n";
}

void generateSimpleDomain(float * p0, float * rhs, SimulationRange simulationRange) {
    int simulationSize = simulationRange.getSimulationSize();
    
    for (int idx = 0; idx < simulationSize; idx++) {
        p0[idx] = idx + 1;
        rhs[idx] = 1;
    }
}
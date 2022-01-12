#include "../SimulationRange.h"
#include "array_index_f2c.h"

// Populating arrays with values in the same manner as C reference; 
// needed to compare results;

void generateDomain(float * p0, float * rhs, SimulationRange simulationRange) {
    int iDim = simulationRange.getDimSizes()[0];
    int jDim = simulationRange.getDimSizes()[1];
    int kDim = simulationRange.getDimSizes()[2];

    for (int i = 0;i <= iDim+1;i += 1) {
        for (int j = 0;j <= jDim+1;j += 1) {
            for (int k = 0;k <= kDim+1;k += 1) {
                rhs[F3D2C(iDim+2,jDim+2,0,0,0,i,j,k)] = 0.1+((i+1)*(j+1)*(k+1))/((iDim+2)*(jDim+2)*(kDim+2));
                p0[F3D2C(iDim+2,jDim+2,0,0,0,i,j,k)] = ((i+1)*(j+1)*(k+1))/((iDim+2)*(jDim+2)*(kDim+2));
                // p0[F3D2C(im+2,jDim+2,0,0,0,i,j,k)] = 1.0;
            }
        }
    }
}
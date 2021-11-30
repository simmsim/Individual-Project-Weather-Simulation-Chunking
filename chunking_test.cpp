
#define COLUMN_MAJOR

#include <algorithm>
#include <stdlib.h>
#include <iostream>

void computePeriodic(float * ch1);
void computeCore(float * ch1, float * ch2);

void fillAndChunk(float *inputArray, float *outputAray) {
    int iDim = 4;
    int jDim = 4;
    int kDim = 2;

    int iChunk = 2;
    int jChunk = 4;
    int kChunk = 2;

    int chSize = iChunk*jChunk*kChunk;

    int iHalChunk = 4;
    int jHalChunk = 6;
    int kHalChunk = 4;

    int chHSize = iHalChunk*jHalChunk*kHalChunk;

    for (int k_start = 0; k_start < kDim; k_start += kChunk) {  //k_start = 0
        for (int j_start = 0; j_start < jDim; j_start += jChunk) { //j_start = 0
            for (int i_start = 0; i_start < iDim; i_start += iChunk) {  //i_start = 2
                // input chunk
                float *ch1 = (float*)malloc(sizeof(float)*chHSize);
                std::fill(ch1, ch1+(chHSize), 0.0);
                // output chunk
                float *ch2 = (float*)malloc(sizeof(float)*chHSize);
                std::fill(ch2, ch2+(chHSize), 0.0);

                for (int k = 0; k < kHalChunk; k++) { // k = 0
                    if ((k_start == 0 && k == 0) ||
                        (k_start + kChunk == kDim && k + 1 == kHalChunk)) {
                            // this is halo for bottom plane or top plane; leave as zeroes
                            continue;
                        }

                    for (int j = 0; j < jHalChunk; j++) {
                        if ((j_start == 0 && j == 0) ||
                            (j_start + jChunk == jDim && j + 1 == jHalChunk)) {
                                continue;
                            }

                        for (int i = 0; i < iHalChunk; i++) {
                            if (((i_start == 0) && i == 0) ||
                                (i_start + iChunk == iDim && i + 1 == iHalChunk)) {
                                    continue;
                                }
                            
                            #if defined(COLUMN_MAJOR)
                                int inputArrayIdx = (k-1+k_start)*iDim*jDim + (j-1+j_start)*iDim + (i-1+i_start);
                                //int chunkIdx = (k+k_start)*iHalChunk*jHalChunk + (j+j_start)*iHalChunk + (i + i_start);
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + i;
                            #else // ROW MAJOR
                                int inputArrayIdx = (k-1+k_start)*iDim*jDim + (i-1+i_start)*jDim + (j-1+j_start);
                                int chunkIdx = k*iHalChunk*jHalChunk + i*jHalChunk + j;
                            #endif

                            ch1[chunkIdx] = inputArray[inputArrayIdx];
                            // Once ch1 is filled with values and correctly padded with halo, send it to kernel...
                            // there, haloes will be updated accordingly based on appropriate boundary conditions
                            // calculations on core values will be places to ch2, and ch2 will be coppied back
                            // to the host
                        }
                    }
                }

                computePeriodic(ch1);

                // print out chunk
                for (int idx = 0; idx < chHSize; idx++) {
                    std::cout << ch1[idx] << " ";
                }
                std::cout << "\n\n" << "";
                
                // Then compute something (increment by 1 or smt...)
               
                computeCore(ch1, ch2);
                std::cout << "\nCore\n" << "";
                for (int idx = 0; idx < chHSize; idx++) {
                    std::cout << ch2[idx] << " ";
                }
                std::cout << "\n\n" << "";
                

                // integrate chunk into output_array
                // this is done assuming that the
                for (int k = 1; k < kHalChunk - 1; k++) { // k = 0
                    for (int j = 1; j < jHalChunk - 1; j++) {
                       // for (int i = 1; i < iHalChunk - 1; i++) {
                            int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                            // j =1; chunkIdx = 24 + 4 + 1 = 29
                            // j =2; chunkIx = 24 + 8 + 1= 33
                            //int outputArrayIdx = (k-1+k_start)*iDim*jDim + (j-1+j_start)*iDim + (i-1+i_start);
                            int first = ch2[chunkIdx];
                            int second = ch2[chunkIdx+iChunk];
                            int arrayEnd = i_start + (j-1)*iChunk*2 + (k-1)*iChunk*jChunk*2; // 2 should be no of chunks
                            std::copy(ch2 + chunkIdx, ch2 + chunkIdx + iChunk, outputAray+arrayEnd);
                            int outputArraySize = iDim*jDim*kDim;

                            std::cout << "\n Ouput array at " << j << "\n";
                            for (int x = 0; x < outputArraySize; x++) {
                                float number = outputAray[x];
                                std::cout << number << " ";
                            }
                            std::cout << "end of output array\n" << "";
                           // outputAray[outputArrayIdx] = ch1[chunkIdx];
                        //}
                    }
                }

                //for (int x = (iHalChunk*jHalChunk)+iHalChunk+1; x < 96-iHalChunk*jHalChunk; x+)

                /*
                int chunkSize = iChunk*jChunk*kChunk;
                for (int idx = 0; idx < chunkSize; idx++) {
                    float number = outputAray[idx];
                }
                */

                free(ch1);
                free(ch2);
            }
        }
    }
}

unsigned int FTNREF3D0(int ix, int jx, int kx, unsigned int iz,unsigned int jz) {
        return iz*jz*kx+iz*jx+ix ;
}

void computePeriodic(float * ch1) {
    int iHalChunk = 4;
    int jHalChunk = 6;
    int kHalChunk = 4;
    unsigned int ip = iHalChunk - 2;
    unsigned int jp = jHalChunk - 2;

    // on real kernel there will be no loop like here
    // In the real kernel, int i = get_globa_id(0), int j = get_global_id(1)...
    for (int k = 0; k < kHalChunk; k++) {
        for (int j = 0; j < jHalChunk; j++) {
            for (int i = 0; i < iHalChunk; i++) {
                ch1[FTNREF3D0(i,0,k, ip+2, jp+2)] = ch1[FTNREF3D0(i,jp,k, ip+2, jp+2)];
                ch1[FTNREF3D0(i,jp+1,k, ip+2, jp+2)] = ch1[FTNREF3D0( i,1,k, ip+2, jp+2)];
            }
        }
    }
}

unsigned int F3D2C(unsigned int i_rng,unsigned int j_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, int k_lb, // lower bounds
        int ix, int jx, int kx
        ) {
    return (i_rng*j_rng*(kx-k_lb)+i_rng*(jx-j_lb)+ix-i_lb);
}

void computeCore(float * ch1, float * ch2) {
    int iHalChunk = 4;
    int jHalChunk = 6;
    int kHalChunk = 4;
    unsigned int ip = iHalChunk - 2;
    unsigned int jp = jHalChunk - 2;

    // on real kernel there will be no loop like here
    // In the real kernel, int i = get_globa_id(0), int j = get_global_id(1)...
    for (int k = 1; k < kHalChunk-1; k++) {
        for (int j = 1; j < jHalChunk-1; j++) {
            for (int i = 1; i < iHalChunk-1; i++) {
                ch2[F3D2C(ip+2, jp+2, 0,0,0, i,j,k)] = ch1[F3D2C(ip+2, jp+2, 0,0,0, i-1,j,k)]
                                                    + ch1[F3D2C(ip+2, jp+2, 0,0,0, i+1,j,k)]
                                                    + ch1[F3D2C(ip+2, jp+2, 0,0,0, i,j-1,k)]
                                                    + ch1[F3D2C(ip+2, jp+2, 0,0,0, i,j+1,k)]
                                                    + ch1[F3D2C(ip+2, jp+2, 0,0,0, i,j,k-1)]
                                                    + ch1[F3D2C(ip+2, jp+2, 0,0,0, i,j,k+1)];
            }
        }
    }
}


int main(void) {
    int iDim = 4;
    int jDim = 4;
    int kDim = 2;

    int inputArraySize = iDim*jDim*kDim;

    int iChunk = 2;
    int jChunk = 4;
    int kChunk = 2;

    int arraySize = iChunk*jChunk*kChunk;

    int iHalChunk = 4;
    int jHalChunk = 6;
    int kHalChunk = 4;

    float *outputArray = (float*)malloc(sizeof(float)*inputArraySize);
    std::fill(outputArray, outputArray+(inputArraySize), 0.0);

    float *inputArray = (float*)malloc(sizeof(float)*inputArraySize);
    // initialize array with simple values
    for (int idx = 0; idx < inputArraySize; idx++) {
        inputArray[idx] = idx;
    }

    fillAndChunk(inputArray, outputArray);

    for (int idx = 0; idx < inputArraySize; idx++) {
        float number = outputArray[idx];
        std::cout << number << " ";
    }

    free(outputArray);
    free(inputArray);

}
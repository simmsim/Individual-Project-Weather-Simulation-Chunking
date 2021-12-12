
#define COLUMN_MAJOR

#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <chrono>

void computePeriodic(float * ch1);
void computeCore(float * ch1, float * ch2);
void simpleCompute(float * ch1, float * ch2);

void fillAndChunk(float *p, float *outputAray) {
    int iDim = 40;
    int jDim = 4;
    int kDim = 3;

    int coreSize = iDim*jDim*kDim;

    int iChunk = 20;
    int jChunk = 4;
    int kChunk = 1;

    int chSize = iChunk*jChunk*kChunk;
    int noOfChunks = coreSize/chSize;

    int iHalChunk = 22;
    int jHalChunk = 6;
    int kHalChunk = 3;

    int chHSize = iHalChunk*jHalChunk*kHalChunk;
    int noOfChunksInJ = (iDim*jDim) / (iChunk*jChunk);

    float * inputArray = p;
    int timeIterations = 1;

    for (int n = 0; n < timeIterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {  
                    // input chunk
                    float *ch1 = (float*)malloc(sizeof(float)*chHSize);
                    std::fill(ch1, ch1+(chHSize), 0.0);
                    // output chunk
                    float *ch2 = (float*)malloc(sizeof(float)*chHSize);
                    std::fill(ch2, ch2+(chHSize), 0.0);

                    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
                    for (int k = 0; k < kHalChunk; k++) {
                        if (((k_start == 0 && k == 0) ||
                            (k_start + kChunk == kDim && k + 1 == kHalChunk)) && kDim > 1) {
                            // this is halo for bottom plane or top plane; leave as zeroes
                            continue;
                        }

                        for (int j = 0; j < jHalChunk; j++) {
                            if (((j_start == 0 && j == 0) ||
                                (j_start + jChunk == jDim && j + 1 == jHalChunk)) && jDim > 1) {
                                continue;
                            }

                            // TODO: done for 1-point halo: should be more general
                            // Calculation for offsets are done based on the assumption that we're chunking jChunk = jDim
                            if (kDim == 1 && jDim == 1) {
                                int inputStartOffset = i_start - 1;
                                int inputEndOffset = i_start + 1 + iChunk;
                                int chunkOffset = 0;
                                if (i_start == 0) {
                                    inputStartOffset = i_start;
                                    chunkOffset = 1;
                                }
                                if (i_start + iChunk >= iDim) {
                                    inputEndOffset = i_start + iChunk;
                                }
                                std::copy(inputArray + inputStartOffset, inputArray + inputEndOffset, ch1 + chunkOffset);
                            }  else if (kDim == 1) {
                                int chunkIdx = j*iHalChunk;
                                int offset = (j-1)*iChunk*noOfChunks + i_start - 1;
                                int inputEndOffset = offset + iChunk + 2;

                                if (i_start == 0) {
                                    chunkIdx += 1; // halo of zero
                                    offset = (j-1)*iChunk*noOfChunks;
                                    inputEndOffset = offset + iChunk + 1;
                                }
                                
                                if (i_start + iChunk >= iDim) {
                                    inputEndOffset -= 1;
                                }

                                std::copy(inputArray + offset, inputArray + inputEndOffset, ch1 + chunkIdx);   
                            } else {
                                int offset = i_start + (j-1)*iChunk*noOfChunksInJ + k*iDim*jDim + (k_start-1)*iDim*jDim - 1;
                                int chunkIdx = j*iHalChunk + k*iHalChunk*jHalChunk;

                                if (i_start == 0) {
                                   chunkIdx += 1; 
                                   offset = (j-1)*iChunk*noOfChunksInJ + k*iDim*jDim + (k_start-1)*iDim*jDim;
                                }

                                if (k_start == 0) {
                                    offset = i_start + (j-1)*iChunk*noOfChunksInJ + (k-1)*iDim*jDim - 1;
                                }

                                if (k_start == 0 && i_start == 0) {
                                    offset = (j-1)*iChunk*noOfChunksInJ + (k-1)*iDim*jDim;
                                }

                                int inputEndOffset = offset + iChunk + 1;
                                std::copy(inputArray + offset, inputArray + inputEndOffset, ch1 + chunkIdx);                          
                            }
                        }
                    }
                    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
                    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
                    
                    /*
                    std::cout << "\nCore\n" << "";
                    std::cout << "i_start " << i_start << "\n";
                    for (int idx = 0; idx < chHSize; idx++) {
                        std::cout << ch1[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    
                    std::cout << "\nchunk\n";
                    */
                    
                    //computePeriodic(ch1);
                    simpleCompute(ch1, ch2);
                    // Again, for testing purposes. Remove once integrated to the api.

                    if (kDim == 1 && jDim == 1) {
                        int arrayEnd = i_start;
                        int chunkIdx = 1;
                        std::copy(ch2 + chunkIdx, ch2 + chunkIdx + iChunk, outputAray+arrayEnd);
                    } else if (kDim == 1) {
                        for (int j = 1; j < jHalChunk - 1; j++) {
                            int chunkIdx = j*iHalChunk + 1;
                            int arrayEnd = (j-1)*iChunk*noOfChunks + i_start;
                            std::copy(ch2 + chunkIdx, ch2 + chunkIdx + iChunk, outputAray+arrayEnd);
                        }
                    } else {
                        for (int k = 1; k < kHalChunk - 1; k++) { 
                            for (int j = 1; j < jHalChunk - 1; j++) {
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int arrayEnd = i_start + (j-1)*iChunk*noOfChunks + (k-1)*iChunk*jChunk*noOfChunks;
                                std::copy(ch2 + chunkIdx, ch2 + chunkIdx + iChunk, outputAray+arrayEnd);
                            }
                        }
                    }

                    // Fly away and be free.
                    free(ch1);
                    free(ch2);
                }
            }
        }
        // the output array becomes our problem/input array for next iteration
        inputArray = outputAray;
        outputAray = p;
    }
}


inline unsigned int F2D2C(
        unsigned int i_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, // lower bounds
        int ix, int jx
        ) {
    return (i_rng*(jx-j_lb)+ix-i_lb);
}

void simpleCompute(float * ch1, float * ch2) {
    int iHalChunk = 22;
    int jHalChunk = 6;
    int kHalChunk = 3;

    for (int k = 0; k < kHalChunk; k++) {
        for (int j = 1; j < jHalChunk-1; j++) {
            for (int i = 1; i < iHalChunk-1; i++) {
                ch2[F2D2C(iHalChunk, 0, 0, i, j)] = ch1[F2D2C(iHalChunk, 0, 0, i, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i-1, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i+1, j)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i, j-1)]
                                                + ch1[F2D2C(iHalChunk, 0, 0, i, j+1)];
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
    int iDim = 40;
    int jDim = 4;
    int kDim = 3;

    int inputArraySize = iDim*jDim*kDim;

    int iChunk = 20;
    int jChunk = 4;
    int kChunk = 1;

    int iHalChunk = 22;
    int jHalChunk = 6;
    int kHalChunk = 3;

    float *outputArray = (float*)malloc(sizeof(float)*inputArraySize);
    std::fill(outputArray, outputArray+(inputArraySize), 0.0);

    float *inputArray = (float*)malloc(sizeof(float)*inputArraySize);
    // initialize array with simple values
    for (int idx = 0; idx < inputArraySize; idx++) {
        inputArray[idx] = idx + 1;
    }

    fillAndChunk(inputArray, outputArray);
    /*
    for (int idx = 0; idx < inputArraySize; idx++) {
        float number = outputArray[idx];
        std::cout << number << " ";
    }
    */

    // Fly away and be free
    free(outputArray);
    free(inputArray);
}
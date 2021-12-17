#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <chrono>

void computePeriodic(float * ch1);
void computeCore(SimulationRange haloRange, float * ch1, float * ch2);
void simpleCompute1D(SimulationRange haloRange, float * ch1, float * ch2);
void simpleCompute2D(SimulationRange haloRange, float * ch1, float * ch2);

void fillAndChunk(SimulationRange coreRange, SimulationRange chunkRange,
                  SimulationRange haloRange, float *p, float *outputAray) {
    int iDim = coreRange.getDimSizes()[0];
    int jDim = coreRange.getDimSizes()[1];
    int kDim = coreRange.getDimSizes()[2];

    int iChunk = chunkRange.getDimSizes()[0];
    int jChunk = chunkRange.getDimSizes()[1];
    int kChunk = chunkRange.getDimSizes()[2];

    int currentIChunk = iChunk;
    int chSize = iChunk*jChunk*kChunk;

    int coreSizeij = iDim*jDim;
    int chSizeij = iChunk*jChunk;
    int noOfChunks = coreSizeij/chSizeij;

    int noOfLeftoverChunks = coreSizeij % chSizeij == 0? 0 : 1;
    int leftoverIChunk = noOfLeftoverChunks == 1 ? (coreSizeij - noOfChunks*chSizeij)/jChunk : 0;

    int iHalChunk = haloRange.getDimSizes()[0];
    int jHalChunk = haloRange.getDimSizes()[1];
    int kHalChunk = haloRange.getDimSizes()[2];

    int chHSize = iHalChunk*jHalChunk*kHalChunk;
    int noOfChunksInJ = (iDim*jDim) / (iChunk*jChunk);

    float * inputArray = p;
    int timeIterations = 1;
    int chunkIdx = 0;

    for (int n = 0; n < timeIterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {
                    // Logic to deal with a leftover chunk
                    if (i_start + iChunk > iDim) {
                        currentIChunk = (coreSizeij % chSizeij) / jChunk;
                        if (jDim > 1 && kDim == 1) {
                            iHalChunk = currentIChunk + 2;
                            haloRange.getDimSizes()[0] = iHalChunk;
                            chHSize =  iHalChunk*jHalChunk*kHalChunk;
                        }
                    }

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
                                int inputEndOffset = i_start + 1 + currentIChunk;
                                int chunkOffset = 0;
                                if (i_start == 0) {
                                    inputStartOffset = i_start;
                                    chunkOffset = 1;
                                }
                                if (i_start + currentIChunk >= iDim) {
                                    inputEndOffset = i_start + currentIChunk;
                                }
                                std::copy(inputArray + inputStartOffset, inputArray + inputEndOffset, ch1 + chunkOffset);
                            }  else if (kDim == 1) {
                                int chunkIdx = j*iHalChunk;
                                int offset = (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk + i_start - 1; // the iChunk must
                                // the full chunks'
                                int inputEndOffset = offset + currentIChunk + 2; // HERE iCHUNK will vary

                                if (i_start == 0) {
                                    chunkIdx += 1; // halo of zero
                                    offset = (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk; 
                                    inputEndOffset = offset + iChunk + 1; // this iChunk value will always be of full chunks'
                                    // here since i_start == 0 is always full
                                }
                                
                                if (i_start + iChunk >= iDim) {
                                    inputEndOffset -= 1;
                                }

                                std::copy(inputArray + offset, inputArray + inputEndOffset, ch1 + chunkIdx);   
                            } else {
                                int offset = i_start + (j-1)*iChunk*noOfChunksInJ + (j-1)*leftoverIChunk + k*iDim*jDim + (k_start-1)*iDim*jDim - 1;
                                int chunkIdx = j*iHalChunk + k*iHalChunk*jHalChunk;
                                int inputEndOffset = offset + currentIChunk + 2;

                                if (i_start == 0) {
                                   chunkIdx += 1; 
                                   offset = (j-1)*iChunk*noOfChunksInJ + (j-1)*leftoverIChunk + k*iDim*jDim + (k_start-1)*iDim*jDim;
                                   inputEndOffset = offset + currentIChunk + 1;
                                }

                                if (k_start == 0) {
                                    offset = i_start + (j-1)*iChunk*noOfChunksInJ + (j-1)*leftoverIChunk + (k-1)*iDim*jDim - 1;
                                    inputEndOffset = offset + currentIChunk + 2;
                                }

                                if (k_start == 0 && i_start == 0) {
                                    offset = (j-1)*iChunk*noOfChunksInJ + (j-1)*leftoverIChunk + (k-1)*iDim*jDim;
                                    inputEndOffset = offset + currentIChunk + 1;
                                }

                                if (iChunk == iDim || i_start + iChunk >= iDim) { 
                                    inputEndOffset -= 1;
                                }

                                std::copy(inputArray + offset, inputArray + inputEndOffset, ch1 + chunkIdx);                          
                            }
                        }
                    }
                    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                   // std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
                    //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
                    
                    /*
                    std::cout << "\nCore\n" << "";
                    std::cout << "i_start " << i_start << "\n";
                    for (int idx = 0; idx < chHSize; idx++) {
                        std::cout << ch1[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    */
                    
                    // For testing purposes.
                    if (jDim == 1 && kDim == 1) {
                        simpleCompute1D(haloRange, ch1, ch2);
                    } else if (kDim == 1) {
                        simpleCompute2D(haloRange, ch1, ch2);
                    } else {
                        computeCore(haloRange, ch1, ch2);
                    }
                    
                    // Again, for testing purposes. Remove once integrated to the api.
                    /*
                    std::cout << "\n After SIMPLE compute\n" << "";
                    for (int idx = 0; idx < chHSize; idx++) {
                        std::cout << ch2[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    */

                    if (kDim == 1 && jDim == 1) {
                        int arrayEnd = i_start;
                        int chunkIdx = 1;
                        std::copy(ch2 + chunkIdx, ch2 + chunkIdx + iChunk, outputAray+arrayEnd);
                    } else if (kDim == 1) {
                        for (int j = 1; j < jHalChunk - 1; j++) {
                            int chunkIdx = j*iHalChunk + 1;
                            int arrayEnd = (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk + i_start;
                            std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, outputAray+arrayEnd);
                        }
                    } else {
                        for (int k = 1; k < kHalChunk - 1; k++) { 
                            for (int j = 1; j < jHalChunk - 1; j++) {
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int arrayEnd = i_start + (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk + (k-1)*iChunk*jChunk*noOfChunks + (k-1)*leftoverIChunk*jChunk;
                                std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, outputAray+arrayEnd);
                                /*
                                for (int x = 0; x < chHSize; x++) {
                                    std::cout << outputAray[x] << " ";
                                }
                                std::cout << "\n\n";
                                */
                            }
                        }
                    }

                    /*
                    std::cout << "\nOutputArray\n" << "";
                    for (int idx = 0; idx < coreSize; idx++) {
                        std::cout << outputAray[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    */

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

void simpleCompute1D(SimulationRange haloRange, float * ch1, float * ch2) {
    int iHalChunk = haloRange.getDimSizes()[0];

    for (int i = 1; i < iHalChunk-1; i++) {
        ch2[i] = ch1[i-1] + ch1[i] + ch1[i+1];
    }
}

void simpleCompute2D(SimulationRange haloRange, float * ch1, float * ch2) {
    int iHalChunk = haloRange.getDimSizes()[0];
    int jHalChunk = haloRange.getDimSizes()[1];
    int kHalChunk = haloRange.getDimSizes()[2];

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

unsigned int F3D2C(unsigned int i_rng,unsigned int j_rng, // ranges, i.e. (hb-lb)+1
        int i_lb, int j_lb, int k_lb, // lower bounds
        int ix, int jx, int kx
        ) {
    return (i_rng*j_rng*(kx-k_lb)+i_rng*(jx-j_lb)+ix-i_lb);
}

void computeCore(SimulationRange haloRange, float * ch1, float * ch2) {
    int iHalChunk = haloRange.getDimSizes()[0];
    int jHalChunk = haloRange.getDimSizes()[1];
    int kHalChunk = haloRange.getDimSizes()[2];
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
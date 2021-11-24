
#define COLUMN_MAJOR

#include <algorithm>
#include <stdlib.h>
#include <iostream>

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
                // output chunk
                float *ch2 = (float*)malloc(sizeof(float)*chSize);
                std::fill(ch1, ch1+(chHSize), 0.0);

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

                            ch1[chunkIdx] = inputArray[inputArrayIdx]*2;
                            // Once ch1 is filled with values and correctly padded with halo, send it to kernel...
                            // there, haloes will be updated accordingly based on appropriate boundary conditions
                            // calculations on core values will be places to ch2, and ch2 will be coppied back
                            // to the host
                        }
                    }
                }

                // print out chunk
                for (int idx = 0; idx < chHSize; idx++) {
                    std::cout << ch1[idx] << " ";
                }
                std::cout << "\n\n" << "";

                // integrate chunk into output_array
                // this is done assuming that the
                for (int k = 1; k < kHalChunk - 1; k++) { // k = 0
                    for (int j = 1; j < jHalChunk - 1; j++) {
                        for (int i = 1; i < iHalChunk - 1; i++) {
                            int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + i;
                            int outputArrayIdx = (k-1+k_start)*iDim*jDim + (j-1+j_start)*iDim + (i-1+i_start);
                            // this ch1 should actually be ch2 with only core values, without halo; with no halo
                            // we could just copy the hole thing, not point by point
                            outputAray[outputArrayIdx] = ch1[chunkIdx];
                        }
                    }
                }
                /*
                int chunkSize = iChunk*jChunk*kChunk;
                for (int idx = 0; idx < chunkSize; idx++) {
                    float number = outputAray[idx];
                }
                */

                free(ch1);
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
#include "Simulation.h"

#include <iostream>

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    oclSetup = OCLSetup(deviceType, programFileName, kernelName);
}

void Simulation::RunSimulation(cl_float * p, int halo, int iterations,
                               SimulationRange coreDimensions, 
                               SimulationRange chunkDimensions = SimulationRange()) {
    InitializeSimulationArea(p, halo, iterations, coreDimensions, chunkDimensions);
    CheckSpecifiedChunkSize();
    ChunkAndCompute();
}

void Simulation::InitializeSimulationArea(cl_float * p, int halo, int iterations, 
                                          SimulationRange coreDimensions, SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.halo = halo;
    simulationArea.iterations = iterations;
    simulationArea.coreDimensions = coreDimensions;
    simulationArea.chunkDimensions = (chunkDimensions.getDimensions() == 0) ? SimulationRange(coreDimensions) : chunkDimensions;
    simulationArea.halChunkDimensions = SimulationRange(chunkDimensions);
    simulationArea.halChunkDimensions.incrementDimensionsBy(halo*2);
    
}

void Simulation::CheckSpecifiedChunkSize() {
    long maxMem = oclSetup.deviceProperties.maxMemAllocSize * 0.95;
    long requiredMem = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(float) * 2;
    if (requiredMem > maxMem) {
        std::cout << "Required memory for processing {" << requiredMem 
            << "} exceeds device's memory max allocation size {" << maxMem << "}\n";
    } else if (!ChunkExceedsCoreDimensions()) {
        return;
    } 

    std::cout << "New chunk size will be determined automatically.\n";
    ReconfigureChunkSize(maxMem);
}

bool Simulation::ChunkExceedsCoreDimensions() {
    int coreDimensions = simulationArea.coreDimensions.getDimensions();
    int chunkDimensions = simulationArea.chunkDimensions.getDimensions();

    if (coreDimensions != chunkDimensions) {
        std::cout << "Core and chunk must have the same dimensions, but provided core dimensions {"
                  << coreDimensions << "} and chunk dimensions {" << chunkDimensions << "} are different.\n";
        return true; 
    }

    int * coreDimSizes = simulationArea.coreDimensions.getDimSizes();
    int * chunkDimSizes = simulationArea.chunkDimensions.getDimSizes();
    for (int i = 0; i < coreDimensions; i++) {
        if (chunkDimSizes[i] > coreDimSizes[i]) {
            std::cout << "Provided chunk dimension size {" << chunkDimSizes[i] << "} " <<
            "exceed core dimension size {" << coreDimSizes[i] << "} at index " << i << "\n"; 
            return true;
        }
    }

    return false;
}

void Simulation::ReconfigureChunkSize(long maxMem) {
    // TODO: automatic chunk configuration; low priority
}

void Simulation::ChunkAndCompute() {
    int iDim = simulationArea.coreDimensions.getDimSizes()[0];
    int jDim = simulationArea.coreDimensions.getDimSizes()[1];
    int kDim = simulationArea.coreDimensions.getDimSizes()[2];

    int coreSize = iDim*jDim*kDim;
    float *p2 = (float*)malloc(sizeof(float)*coreSize);
    std::fill(p2, p2+(coreSize), 0.0);

    int iChunk = simulationArea.chunkDimensions.getDimSizes()[0];
    int jChunk = simulationArea.chunkDimensions.getDimSizes()[1];
    int kChunk = simulationArea.chunkDimensions.getDimSizes()[2];

    int currentIChunk = iChunk;
    int chSize = iChunk*jChunk*kChunk;

    int coreSizeij = iDim*jDim;
    int chSizeij = iChunk*jChunk;
    int noOfChunks = coreSizeij/chSizeij;

    int noOfLeftoverChunks = coreSizeij % chSizeij == 0? 0 : 1;
    int leftoverIChunk = noOfLeftoverChunks == 1 ? (coreSizeij - noOfChunks*chSizeij)/jChunk : 0;

    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    int chHSize = iHalChunk*jHalChunk*kHalChunk;
    int noOfChunksInJ = (iDim*jDim) / (iChunk*jChunk);

    //float * p1 = simulationArea.p;
    float * p1 = (float*)malloc(sizeof(float)*coreSize);
    std::copy(simulationArea.p, simulationArea.p + coreSize, p1);
    int chunkIdx = 0;

    for (int n = 0; n < simulationArea.iterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {
                    // Logic to deal with a leftover chunk
                    if (i_start + iChunk > iDim) {
                        currentIChunk = (coreSizeij % chSizeij) / jChunk;
                        if (jDim > 1 && kDim == 1) {
                            iHalChunk = currentIChunk + 2;
                            simulationArea.halChunkDimensions.getDimSizes()[0] = iHalChunk;
                            chHSize =  iHalChunk*jHalChunk*kHalChunk;
                        }
                    }

                    // input chunk
                    float *ch1 = (float*)malloc(sizeof(float)*chHSize);
                    std::fill(ch1, ch1+(chHSize), 0.0);
                    // output chunk
                    float *ch2 = (float*)malloc(sizeof(float)*chHSize);
                    std::fill(ch2, ch2+(chHSize), 0.0);

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
                                std::copy(p1 + inputStartOffset, p1 + inputEndOffset, ch1 + chunkOffset);
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

                                std::copy(p1 + offset, p1 + inputEndOffset, ch1 + chunkIdx);   
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

                                std::copy(p1 + offset, p1 + inputEndOffset, ch1 + chunkIdx);                          
                            }
                        }
                    }

                    std::cout << "\nCore\n" << "";
                    std::cout << "i_start " << i_start << "\n";
                    for (int idx = 0; idx < chHSize; idx++) {
                        std::cout << ch1[idx] << " ";
                    }
                    std::cout << "\n\n" << "";

                    // ****** openCL bit
                    // TODO: maybe move this bit into a separate method?
                    size_t sizeOfHChunk = simulationArea.halChunkDimensions.getSimulationSize();
                    cl_int errorCode;

                    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeOfHChunk * sizeof(float), ch1, &errorCode);
                    ErrorHelper::testError(errorCode, "Failed to create a buffer");                    
                    cl::Buffer out_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, sizeOfHChunk * sizeof(float), nullptr);

                    errorCode = oclSetup.kernel.setArg(0, in_p);
                    ErrorHelper::testError(errorCode, "Failed to set a kernel argument");
                    errorCode = oclSetup.kernel.setArg(1, out_p);
                    ErrorHelper::testError(errorCode, "Failed to set a kernel argument");
                    oclSetup.kernel.setArg(2, currentIChunk);
                    oclSetup.kernel.setArg(3, jChunk);
                    oclSetup.kernel.setArg(4, kChunk);

                    cl::Event event;
                    if (i_start == 0) {
                        std::cout << "inflow\n";
                        oclSetup.kernel.setArg(5, 2);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(1, jHalChunk, kHalChunk),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                        // in-flow
                    } else if (i_start + iChunk >= iDim) {
                        // out-flow
                        std::cout << "outflow\n";
                        // TODO: this whole thing could be factored out into a separate method
                        oclSetup.kernel.setArg(5, 3);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(1, jHalChunk, kHalChunk),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    } 

                    // top-bottom, only applies for 3D
                    if (kDim > 1) {
                        std::cout << "top-bottom\n";
                        oclSetup.kernel.setArg(5, 4);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(iHalChunk, jHalChunk, 1),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    }

                    // PERIODIC
                    oclSetup.kernel.setArg(5, 1);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(iHalChunk, 1, kHalChunk),
                        cl::NullRange, nullptr, &event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    
                    event.wait();

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(in_p, CL_TRUE, 0, sizeOfHChunk * sizeof(float), ch2);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device");

                    std::cout << "\n After SIMPLE compute\n" << "";
                    for (int idx = 0; idx < chHSize; idx++) {
                        std::cout << ch2[idx] << " ";
                    }
                    std::cout << "\n\n" << "";

                    if (kDim == 1 && jDim == 1) {
                        int arrayEnd = i_start;
                        int chunkIdx = 1;
                        std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, p2+arrayEnd);
                    } else if (kDim == 1) {
                        for (int j = 1; j < jHalChunk - 1; j++) {
                            int chunkIdx = j*iHalChunk + 1;
                            int arrayEnd = (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk + i_start;
                            std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, p2+arrayEnd);
                        }
                    } else {
                        for (int k = 1; k < kHalChunk - 1; k++) { 
                            for (int j = 1; j < jHalChunk - 1; j++) {
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int arrayEnd = i_start + (j-1)*iChunk*noOfChunks + (j-1)*leftoverIChunk + (k-1)*iChunk*jChunk*noOfChunks + (k-1)*leftoverIChunk*jChunk;
                                std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, p2+arrayEnd);
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
        float * intermediate = p1;
        p1 = p2;
        p2 = intermediate;
    }
    std::copy(p1, p1 + coreSize, simulationArea.p);
    for (int i = 0; i < coreSize; i++) {
        std::cout << simulationArea.p[i] << " ";
    }
    std::cout << "\n";
    free(p2);
    free(p1);
}
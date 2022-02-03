#include "Simulation.h"

#include <iostream>
#include <math.h>

#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

#define ARR_BLOCKS 3

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName, int * err) {
    oclSetup = OCLSetup(deviceType, programFileName, kernelName, err);
}

int Simulation::RunSimulation(cl_float * p, cl_float * rhs,
                               int halo, int iterations,
                               float maxSimulationAreaMemUsage,
                               SimulationRange coreDimensions, 
                               SimulationRange chunkDimensions = SimulationRange()) {
    int errorCode;
    InitializeSimulationArea(p, rhs, halo, iterations, coreDimensions, chunkDimensions);
    errorCode = CheckChunkDimensions();
    if (errorCode != CHUNK_SUCCESS) {
        return errorCode;
    }
    errorCode = CheckSpecifiedChunkSize(maxSimulationAreaMemUsage);
    if (errorCode != CHUNK_SUCCESS) {
        return errorCode;
    }
    ChunkAndCompute();
}

void Simulation::InitializeSimulationArea(cl_float * p, cl_float * rhs, int halo, int iterations, 
                                          SimulationRange coreDimensions, SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.rhs = rhs;
    simulationArea.halo = halo;
    simulationArea.iterations = iterations;
    simulationArea.coreDimensions = coreDimensions;
    simulationArea.chunkDimensions = (chunkDimensions.getDimensions() == 0) ? SimulationRange(coreDimensions) : chunkDimensions;
    simulationArea.halChunkDimensions = SimulationRange(chunkDimensions);
    simulationArea.halChunkDimensions.incrementDimensionsBy(halo*2);
    
}

int Simulation::CheckSpecifiedChunkSize(float maxSimulationAreaMemUsage) {
    long maxMem = (oclSetup.deviceProperties.maxMemAllocSize * maxSimulationAreaMemUsage)/100;
    long requiredMem = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(float) * ARR_BLOCKS;
    if (requiredMem > maxMem) {
        std::cout << "Required memory for processing a chunk {" << requiredMem 
            << "} exceeds device's memory max allocation size {" << maxMem << "}. "
            << "New chunk size will be determined automatically.\n";
        return ReconfigureChunkSize(maxMem);
    }

    return CHUNK_SUCCESS;
}

int Simulation::CheckChunkDimensions() {
    int coreDimensions = simulationArea.coreDimensions.getDimensions();
    int chunkDimensions = simulationArea.chunkDimensions.getDimensions();

    if (coreDimensions != chunkDimensions) {
        std::cout << "Core and chunk must have the same dimensions, but provided core dimensions {"
                  << coreDimensions << "} and chunk dimensions {" << chunkDimensions << "} are different."
                  << " Chunking will not proceed. Exiting...\n";
        return CHUNK_AND_SIMULATION_DIMENSION_MISMATCH;
    }

    int * coreDimSizes = simulationArea.coreDimensions.getDimSizes();
    int * chunkDimSizes = simulationArea.chunkDimensions.getDimSizes();
    for (int i = 0; i < coreDimensions; i++) {
        if (chunkDimSizes[i] > coreDimSizes[i]) {
            std::cout << "Provided chunk dimension size {" << chunkDimSizes[i] << "} " <<
            "exceed core dimension size {" << coreDimSizes[i] << "} at index " << i 
            << ". Chunking will not proceed. Exiting...\n"; 
            return CHUNK_AND_SIMULATION_DIMENSION_SIZE_MISMATCH;
        }
    }

    return CHUNK_SUCCESS;
}

int Simulation::ReconfigureChunkSize(long maxMem) {
    long bytesRequiredForHalloedChunks;
    int iDimHalSize;
    if (simulationArea.coreDimensions.getDimensions() > 1) {
        int jDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[1];
        int kDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[2];
        bytesRequiredForHalloedChunks = jDimHalSize*kDimHalSize*sizeof(float)*ARR_BLOCKS;
    } else {
        bytesRequiredForHalloedChunks = sizeof(float)*ARR_BLOCKS;
    }

    iDimHalSize = (int)floor((float)maxMem/(float)bytesRequiredForHalloedChunks);
    int iDimChunkSize = iDimHalSize-simulationArea.halo*2;
    if (iDimChunkSize < 1) {
        std::cout << "Unable to determine an appropriate chunk size. Terminating.\n";
        return CHUNK_RECONFIGURE_FAILURE;
    }
    simulationArea.halChunkDimensions.updateDimSize(0, iDimHalSize);
    simulationArea.chunkDimensions.updateDimSize(0, iDimChunkSize);
    std::cout << "New chunk first dimension size is " << simulationArea.chunkDimensions.getDimSizes()[0] << " \n"
    << "New halloed chunk first dimension size is " << simulationArea.halChunkDimensions.getDimSizes()[0] << " \n";
    
    return CHUNK_SUCCESS;
}

void Simulation::ChunkAndCompute() {
    int iDim = simulationArea.coreDimensions.getDimSizes()[0];
    int jDim = simulationArea.coreDimensions.getDimSizes()[1];
    int kDim = simulationArea.coreDimensions.getDimSizes()[2];

    size_t coreSize = simulationArea.coreDimensions.getSimulationSize();
    float *p2 = (float*)malloc(sizeof(float)*coreSize);
    // float * p1 = simulationArea.p;
    // Didn't work properly when program was set with just pointing to simulationArea. TODO: check later if it can be fixed.
    float * p1 = (float*)malloc(sizeof(float)*coreSize);
    std::copy(simulationArea.p, simulationArea.p + coreSize, p1);

    int iChunk = simulationArea.chunkDimensions.getDimSizes()[0];
    int jChunk = simulationArea.chunkDimensions.getDimSizes()[1];
    int kChunk = simulationArea.chunkDimensions.getDimSizes()[2];

    int currentIChunk = iChunk;
    int coreSizeij = iDim*jDim;
    int chunkSizeij = iChunk*jChunk;
    int noOfFullChunks = coreSizeij/chunkSizeij;
    int noOfLeftoverChunks = coreSizeij % chunkSizeij == 0 ? 0 : 1;
    int leftoverIChunk = noOfLeftoverChunks == 1 ? (coreSizeij - noOfFullChunks*chunkSizeij)/jChunk : 0;

    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    size_t haloChunkSize =simulationArea.halChunkDimensions.getSimulationSize();

    cl_int err;
    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in buffer"); 
    cl::Buffer rhsBuffer(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an rhs buffer");                    
    cl::Buffer out_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an out buffer");

    float *inChunk = (float*)malloc(sizeof(float)*haloChunkSize);
    float *rhsChunk = (float*)malloc(sizeof(float)*haloChunkSize);
    float *outChunk = (float*)malloc(sizeof(float)*haloChunkSize);

    for (int n = 0; n < simulationArea.iterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {
                    // Logic to deal with a leftover chunk.
                    if (i_start + iChunk > iDim) {
                        currentIChunk = (coreSizeij % chunkSizeij) / jChunk;
                        iHalChunk = currentIChunk + 2;
                        simulationArea.halChunkDimensions.updateDimSize(0, iHalChunk);
                        haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
                    }

                    // Build a chunk, copy values in from core.
                    for (int k = 0; k < kHalChunk; k++) {
                        if (((k_start == 0 && k == 0) ||
                            (k_start + kChunk == kDim && k + 1 == kHalChunk)) && kDim > 1) {
                            // This is halo for bottom plane or top plane; leave as zeros.
                            continue;
                        }

                        for (int j = 0; j < jHalChunk; j++) {
                            if (((j_start == 0 && j == 0) ||
                                (j_start + jChunk == jDim && j + 1 == jHalChunk)) && jDim > 1) {
                                // This is halo for sides; leave as zeros.
                                continue;
                            }

                            // TODO: done for 1-point halo: could be more general; future work.
                            // Calculation for offsets are done based on the assumption that we're chunking jChunk = jDim. 
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
                                std::copy(p1 + inputStartOffset, p1 + inputEndOffset, inChunk + chunkOffset);
                                std::copy(simulationArea.rhs + inputStartOffset, simulationArea.rhs + inputEndOffset, rhsChunk + chunkOffset);
                            }  else if (kDim == 1) {
                                int chunkIdx = j*iHalChunk;
                                int offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + i_start - 1;
                                int inputEndOffset = offset + currentIChunk + 2;

                                if (i_start == 0) {
                                    chunkIdx += 1; // halo of zero
                                    offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk; 
                                    inputEndOffset = offset + iChunk + 1;
                                }
                                
                                if (i_start + iChunk >= iDim) {
                                    inputEndOffset -= 1;
                                }

                                std::copy(p1 + offset, p1 + inputEndOffset, inChunk + chunkIdx); 
                                std::copy(simulationArea.rhs + offset, simulationArea.rhs + inputEndOffset, rhsChunk + chunkIdx);   
                            } else {
                                int offset = i_start + (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + k*iDim*jDim + (k_start-1)*iDim*jDim - 1;
                                int chunkIdx = j*iHalChunk + k*iHalChunk*jHalChunk;
                                int inputEndOffset = offset + currentIChunk + 2;

                                if (i_start == 0) {
                                   chunkIdx += 1; 
                                   offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + k*iDim*jDim + (k_start-1)*iDim*jDim;
                                   inputEndOffset = offset + currentIChunk + 1;
                                }

                                if (k_start == 0) {
                                    offset = i_start + (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + (k-1)*iDim*jDim - 1;
                                    inputEndOffset = offset + currentIChunk + 2;
                                }

                                if (k_start == 0 && i_start == 0) {
                                    offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + (k-1)*iDim*jDim;
                                    inputEndOffset = offset + currentIChunk + 1;
                                }

                                if (iChunk == iDim || i_start + iChunk >= iDim) { 
                                    inputEndOffset -= 1;
                                }

                                std::copy(p1 + offset, p1 + inputEndOffset, inChunk + chunkIdx);   
                                std::copy(simulationArea.rhs + offset, simulationArea.rhs + inputEndOffset, rhsChunk + chunkIdx);                       
                            }
                        }
                    }

                    // ****** openCL bit
                    // TODO: maybe move this bit into a separate method?
                    // Need to get halChuk size as it might have been recalculated for a leftover chunk.
                    size_t sizeOfHChunk = simulationArea.halChunkDimensions.getSimulationSize();
                    cl_int errorCode;

                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(in_p, CL_TRUE, 0, sizeOfHChunk * sizeof(float), inChunk);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer.");
                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsBuffer, CL_TRUE, 0, sizeOfHChunk * sizeof(float), rhsChunk);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer.");

                    oclSetup.kernel.setArg(0, in_p);
                    oclSetup.kernel.setArg(1, out_p);
                    oclSetup.kernel.setArg(2, rhsBuffer);
                    oclSetup.kernel.setArg(3, currentIChunk);
                    oclSetup.kernel.setArg(4, jChunk);
                    oclSetup.kernel.setArg(5, kChunk);

                    if (i_start == 0) {
                        EnqueueKernel(INFLOW, cl::NDRange(1*jHalChunk*kHalChunk));
                    } 
                    
                    if (i_start + iChunk >= iDim) {
                        EnqueueKernel(OUTFLOW, cl::NDRange(1*jHalChunk*kHalChunk));
                    } 

                    // top-bottom, only applies for 3D
                    if (kDim > 1) {
                        EnqueueKernel(TOPBOTTOM, cl::NDRange(iHalChunk*jHalChunk*1));
                    }
                    
                    // Periodic applies to each chunk since we're chunking in such manner that both j sides are included.
                    EnqueueKernel(PERIODIC, cl::NDRange(iHalChunk*1*kHalChunk));

                    cl::Event event;
                    int coreState = CORE;
                    oclSetup.kernel.setArg(6, &coreState);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(currentIChunk*jChunk*kChunk),
                        cl::NullRange, nullptr, &event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel with type: " + CORE);
                    event.wait();

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(out_p, CL_TRUE, 0, sizeOfHChunk * sizeof(float), outChunk);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device");

                    if (kDim == 1 && jDim == 1) {
                        int arrayEnd = i_start;
                        int chunkIdx = 1;
                        std::copy(outChunk + chunkIdx, outChunk + chunkIdx + currentIChunk, p2+arrayEnd);
                    } else if (kDim == 1) {
                        for (int j = 1; j < jHalChunk - 1; j++) {
                            int chunkIdx = j*iHalChunk + 1;
                            int arrayEnd = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + i_start;
                            std::copy(outChunk + chunkIdx, outChunk + chunkIdx + currentIChunk, p2+arrayEnd);
                        }
                    } else {
                        for (int k = 1; k < kHalChunk - 1; k++) {
                            for (int j = 1; j < jHalChunk - 1; j++) {
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int arrayEnd = i_start + (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + (k-1)*iChunk*jChunk*noOfFullChunks + (k-1)*leftoverIChunk*jChunk;
                                std::copy(outChunk + chunkIdx, outChunk + chunkIdx + currentIChunk, p2+arrayEnd);
                            }
                        }
                    }
                }
            }
        }
        // the output array becomes our problem/input array for next iteration
        float * intermediate = p1;
        p1 = p2;
        p2 = intermediate;
    }
    std::copy(p1, p1 + coreSize, simulationArea.p);
    
    // Fly away and be free.
    free(inChunk);
    free(rhsChunk);
    free(outChunk);

    free(p2);
    free(p1);
}

void Simulation::EnqueueKernel(int type, cl::NDRange range) {
    oclSetup.kernel.setArg(6, &type);
    cl_int errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, range, cl::NullRange, nullptr);
    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel for a condition");
}
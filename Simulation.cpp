#include "Simulation.h"

#include <iostream>
#include <math.h>
#include <omp.h>

#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName) {
    oclSetup = OCLSetup(deviceType, programFileName, kernelName);
}

void Simulation::RunSimulation(cl_float * p, int halo, int iterations,
                               float maxSimulationAreaMemUsage,
                               SimulationRange coreDimensions, 
                               SimulationRange chunkDimensions = SimulationRange()) {
    InitializeSimulationArea(p, halo, iterations, coreDimensions, chunkDimensions);
    CheckChunkDimensions();
    CheckSpecifiedChunkSize(maxSimulationAreaMemUsage);
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

void Simulation::CheckSpecifiedChunkSize(float maxSimulationAreaMemUsage) {
    long maxMem = (oclSetup.deviceProperties.maxMemAllocSize * maxSimulationAreaMemUsage)/100;
    long requiredMem = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(float) * 2;
    if (requiredMem > maxMem) {
        std::cout << "Required memory for processing a chunk {" << requiredMem 
            << "} exceeds device's memory max allocation size {" << maxMem << "}. "
            << "New chunk size will be determined automatically.\n";
        ReconfigureChunkSize(maxMem);
    }
}

void Simulation::CheckChunkDimensions() {
    int coreDimensions = simulationArea.coreDimensions.getDimensions();
    int chunkDimensions = simulationArea.chunkDimensions.getDimensions();

    if (coreDimensions != chunkDimensions) {
        std::cout << "Core and chunk must have the same dimensions, but provided core dimensions {"
                  << coreDimensions << "} and chunk dimensions {" << chunkDimensions << "} are different."
                  << " Chunking will not proceed. Exiting...\n";
        // TODO: shouldn't be abruptly exiting the program - perhaps throw exception instead?
        exit(EXIT_FAILURE); 
    }

    int * coreDimSizes = simulationArea.coreDimensions.getDimSizes();
    int * chunkDimSizes = simulationArea.chunkDimensions.getDimSizes();
    for (int i = 0; i < coreDimensions; i++) {
        if (chunkDimSizes[i] > coreDimSizes[i]) {
            std::cout << "Provided chunk dimension size {" << chunkDimSizes[i] << "} " <<
            "exceed core dimension size {" << coreDimSizes[i] << "} at index " << i 
            << ". Chunking will not proceed. Exiting...\n"; 
            // TODO: shouldn't be abruptly exiting the program - perhaps throw exception instead?
            exit(EXIT_FAILURE);
        }
    }
}

void Simulation::ReconfigureChunkSize(long maxMem) {
    long bytesRequiredForHalloedChunks;
    int iDimHalSize;
    std::cout << "Core dimensions " << simulationArea.coreDimensions.getDimensions() << " \n";
    if (simulationArea.coreDimensions.getDimensions() > 1) {
        int jDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[1];
        int kDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[2];
        bytesRequiredForHalloedChunks = jDimHalSize*kDimHalSize*sizeof(float)*2;
    } else {
        bytesRequiredForHalloedChunks = sizeof(float)*2;
    }
    iDimHalSize = (int)floor((float)maxMem/(float)bytesRequiredForHalloedChunks);
    std::cout << "idimHalsize is " << iDimHalSize << " \n";
    int iDimChunkSize = iDimHalSize-simulationArea.halo*2;
    if (iDimChunkSize < 1) {
        std::cout << "Unable to determine an appropriate chunk size. Terminating.\n";
        exit(EXIT_FAILURE);
    }
    simulationArea.halChunkDimensions.updateDimSize(0, iDimHalSize);
    simulationArea.chunkDimensions.updateDimSize(0, iDimChunkSize);
    std::cout << "New chunk first dimension size is " << simulationArea.chunkDimensions.getDimSizes()[0] << " \n"
    << "New halloed chunk first dimension size is " << simulationArea.halChunkDimensions.getDimSizes()[0] << " \n";
}

void Simulation::ChunkAndCompute() {
    int iDim = simulationArea.coreDimensions.getDimSizes()[0];
    int jDim = simulationArea.coreDimensions.getDimSizes()[1];
    int kDim = simulationArea.coreDimensions.getDimSizes()[2];

    int coreSize = simulationArea.coreDimensions.getSimulationSize();
    float *p2 = (float*)malloc(sizeof(float)*coreSize);
    std::fill(p2, p2+(coreSize), 0.0);
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

    int haloChunkSize =simulationArea.halChunkDimensions.getSimulationSize();

    cl_int err;
    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create a buffer");                    
    cl::Buffer out_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an out buffer");

    for (int n = 0; n < simulationArea.iterations; n++) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        //#pragma omp parallel for collapse(3); TODO: need to think more about whether it's possible to parallize in a way
        // that gives performance gain.
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

                    // input chunk
                    float *ch1 = (float*)malloc(sizeof(float)*haloChunkSize);
                    std::fill(ch1, ch1+(haloChunkSize), 0.0);
                    // output chunk
                    float *ch2 = (float*)malloc(sizeof(float)*haloChunkSize);
                    std::fill(ch2, ch2+(haloChunkSize), 0.0);

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
                                int offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + i_start - 1; // the iChunk must
                                // the full chunks'
                                int inputEndOffset = offset + currentIChunk + 2; // HERE iCHUNK will vary

                                if (i_start == 0) {
                                    chunkIdx += 1; // halo of zero
                                    offset = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk; 
                                    inputEndOffset = offset + iChunk + 1; // this iChunk value will always be of full chunks'
                                    // here since i_start == 0 is always full
                                }
                                
                                if (i_start + iChunk >= iDim) {
                                    inputEndOffset -= 1;
                                }

                                std::copy(p1 + offset, p1 + inputEndOffset, ch1 + chunkIdx);   
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

                                std::copy(p1 + offset, p1 + inputEndOffset, ch1 + chunkIdx);                          
                            }
                        }
                    }
                    /*
                    std::cout << "\nCore\n" << "";
                    std::cout << "i_start " << i_start << "\n";
                    for (int idx = 0; idx < haloChunkSize; idx++) {
                        std::cout << ch1[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    */

                    // ****** openCL bit
                    // TODO: maybe move this bit into a separate method?
                    //this is repeated
                    size_t sizeOfHChunk = simulationArea.halChunkDimensions.getSimulationSize();
                    cl_int errorCode;

                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(in_p, CL_TRUE, 0, sizeOfHChunk * sizeof(float), ch1);

                    errorCode = oclSetup.kernel.setArg(0, in_p);
                    ErrorHelper::testError(errorCode, "Failed to set a kernel argument");
                    errorCode = oclSetup.kernel.setArg(1, out_p);
                    ErrorHelper::testError(errorCode, "Failed to set a kernel argument");
                    oclSetup.kernel.setArg(2, currentIChunk);
                    oclSetup.kernel.setArg(3, jChunk);
                    oclSetup.kernel.setArg(4, kChunk);

                    cl::Event event;
                    if (i_start == 0) {
                        //std::cout << "inflow\n";
                        oclSetup.kernel.setArg(5, INFLOW);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(1, jHalChunk, kHalChunk),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                        // in-flow
                    } 
                    
                    if (i_start + iChunk >= iDim) {
                        // out-flow
                        //std::cout << "outflow\n";
                        // TODO: this whole thing could be factored out into a separate method
                        oclSetup.kernel.setArg(5, OUTFLOW);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(1, jHalChunk, kHalChunk),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    } 

                    // top-bottom, only applies for 3D
                    if (kDim > 1) {
                        //std::cout << "top-bottom\n";
                        oclSetup.kernel.setArg(5, TOPBOTTOM);
                        errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(iHalChunk, jHalChunk, 1),
                        cl::NullRange, nullptr);
                        ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    }
                    
                    // periodic applies to each chunk since we're chunking in such manner that both j sides are included
                    oclSetup.kernel.setArg(5, PERIODIC);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(iHalChunk, 1, kHalChunk),
                        cl::NullRange, nullptr);
                    ErrorHelper::testError(errorCode, "F••ailed to enqueue a kernel");

                    //std::cout << "core\n";
                    oclSetup.kernel.setArg(5, CORE);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NDRange(1,1,1), cl::NDRange(currentIChunk, jChunk, kChunk),
                        cl::NullRange, nullptr, &event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel");
                    event.wait();

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(out_p, CL_TRUE, 0, sizeOfHChunk * sizeof(float), ch2);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device");
                    /*
                    std::cout << "\n After SIMPLE compute\n" << "";
                    for (int idx = 0; idx < haloChunkSize; idx++) {
                        std::cout << ch2[idx] << " ";
                    }
                    std::cout << "\n\n" << "";
                    */

                    if (kDim == 1 && jDim == 1) {
                        int arrayEnd = i_start;
                        int chunkIdx = 1;
                        std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, p2+arrayEnd);
                    } else if (kDim == 1) {
                        for (int j = 1; j < jHalChunk - 1; j++) {
                            int chunkIdx = j*iHalChunk + 1;
                            int arrayEnd = (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + i_start;
                            std::copy(ch2 + chunkIdx, ch2 + chunkIdx + currentIChunk, p2+arrayEnd);
                        }
                    } else {
                        for (int k = 1; k < kHalChunk - 1; k++) {
                            for (int j = 1; j < jHalChunk - 1; j++) {
                                int chunkIdx = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int arrayEnd = i_start + (j-1)*iChunk*noOfFullChunks + (j-1)*leftoverIChunk + (k-1)*iChunk*jChunk*noOfFullChunks + (k-1)*leftoverIChunk*jChunk;
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
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
        
        // the output array becomes our problem/input array for next iteration
        float * intermediate = p1;
        p1 = p2;
        p2 = intermediate;
    }
    std::copy(p1, p1 + coreSize, simulationArea.p);
    /*
    for (int i = 0; i < coreSize; i++) {
        std::cout << simulationArea.p[i] << " ";
    }
    std::cout << "\n";
    */
    
    free(p2);
    free(p1);
}
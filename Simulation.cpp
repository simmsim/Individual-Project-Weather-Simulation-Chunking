#include "Simulation.h"

#include <iostream>
#include <math.h>

#define PERIODIC 1
#define INFLOW 2
#define OUTFLOW 3
#define TOPBOTTOM 4
#define CORE 5

#define ARR_BLOCKS 3
#define PROFILING_ENABLED 1

#if defined(MEASURE_EVENT_PERFORMANCE) 
    #define PROFILING_ENABLED 1
#else
    #define PROFILING_ENABLED 0
#endif

Simulation::Simulation(int deviceType, char * programFileName,
                       char * kernelName, int * err) {
    oclSetup = OCLSetup(deviceType, programFileName, kernelName, err);
}

int Simulation::RunSimulation(cl_float * p, cl_float * rhs,
                               int halo, int iterations,
                               SimulationRange simulationDimensions, 
                               SimulationRange chunkDimensions = SimulationRange(),
                               float maxSimulationAreaMemUsage = 100) {
    int errorCode;
    InitializeSimulationArea(p, rhs, halo, iterations, simulationDimensions, chunkDimensions);
    errorCode = CheckChunkDimensions();
    if (errorCode != CHUNK_SUCCESS) {
        return errorCode;
    }
    errorCode = CheckSpecifiedChunkSize(maxSimulationAreaMemUsage);
    if (errorCode != CHUNK_SUCCESS) {
        return errorCode;
    }
    ProcessSimulation();
    // for now return success
    return CHUNK_SUCCESS;
}

void Simulation::InitializeSimulationArea(cl_float * p, cl_float * rhs, int halo, int iterations, 
                                          SimulationRange simulationDimensions, SimulationRange chunkDimensions) {
    simulationArea.p = p;
    simulationArea.rhs = rhs;
    simulationArea.halo = halo;
    simulationArea.iterations = iterations;
    simulationArea.simulationDimensions = simulationDimensions;
    simulationArea.chunkDimensions = (chunkDimensions.getDimensions() == 0) ? SimulationRange(simulationDimensions) : chunkDimensions;
    simulationArea.halChunkDimensions = SimulationRange(chunkDimensions);
    simulationArea.halChunkDimensions.incrementDimensionsBy(halo*2);
}

int Simulation::CheckSpecifiedChunkSize(float maxSimulationAreaMemUsage) {
    if (maxSimulationAreaMemUsage > 100) {
        maxSimulationAreaMemUsage = 100;
    }

    long halChunkBlockRequiredMem = simulationArea.halChunkDimensions.getSimulationSize() * sizeof(float);
    long totalRequiredMem = halChunkBlockRequiredMem * ARR_BLOCKS;
    
    long allowedSingleBufferMem = (oclSetup.deviceProperties.maxMemAllocSize * maxSimulationAreaMemUsage)/100;
    if (halChunkBlockRequiredMem > allowedSingleBufferMem 
        || totalRequiredMem > oclSetup.deviceProperties.maxGlobalMemSize) {
            std::cout << "Required memory for processing exceeds device limits" << "\n";
            std::cout << "Required memory for 1 out of" << ARR_BLOCKS << " chunk data blocks: {" << halChunkBlockRequiredMem 
             << "} exceeds device's memory max buffer allocation size {" << allowedSingleBufferMem << "}. "
             << "New chunk size will be determined automatically.\n";
         return ReconfigureChunkSize(allowedSingleBufferMem);
        }

    return CHUNK_SUCCESS;
}

int Simulation::CheckChunkDimensions() {
    int simulationDimensions = simulationArea.simulationDimensions.getDimensions();
    int chunkDimensions = simulationArea.chunkDimensions.getDimensions();

    if (simulationDimensions != chunkDimensions) {
        std::cout << "Simulation and chunk must have the same dimensions, but provided simulation dimensions {"
                  << simulationDimensions << "} and chunk dimensions {" << chunkDimensions << "} are different."
                  << " Chunking will not proceed. Exiting...\n";
        return CHUNK_AND_SIMULATION_DIMENSION_MISMATCH;
    }

    int * coreDimSizes = simulationArea.simulationDimensions.getDimSizes();
    int * halChunkDimSizes = simulationArea.halChunkDimensions.getDimSizes();
    for (int i = 0; i < simulationDimensions; i++) {
        if (halChunkDimSizes[i] > coreDimSizes[i]) {
            std::cout << "Halloed chunk dimension size {" << halChunkDimSizes[i] << "} " <<
            "exceed simulation dimension size {" << coreDimSizes[i] << "} at index " << i 
            << ". Chunking will not proceed. Exiting...\n"; 
            return CHUNK_AND_SIMULATION_DIMENSION_SIZE_MISMATCH;
        }
    }

    return CHUNK_SUCCESS;
}

int Simulation::ReconfigureChunkSize(long maxMem) {
    // Not checking whether three data blocks for chunk (p0,p1,rhs) exceed global mem for now,
    // it seems that max mem for buffer is 1/4th of global mem anyways! Not urgent, can update later.
    long bytesRequiredForHalChunkDataBlock;
    int iDimHalSize;
    if (simulationArea.simulationDimensions.getDimensions() > 1) {
        int jDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[1];
        int kDimHalSize = simulationArea.halChunkDimensions.getDimSizes()[2];
        bytesRequiredForHalChunkDataBlock = jDimHalSize*kDimHalSize*sizeof(float);
    } else {
        bytesRequiredForHalChunkDataBlock = sizeof(float);
    }

    iDimHalSize = (int)floor((float)maxMem/(float)bytesRequiredForHalChunkDataBlock);
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

void Simulation::ProcessSimulation() {
    if (IsSimulationChunked()) {
        ChunkAndCompute();
    } else {
        ComputeFullSimulation();
    }
}

bool Simulation::IsSimulationChunked() {
    return simulationArea.halChunkDimensions.getSimulationSize() != simulationArea.simulationDimensions.getSimulationSize();
}

void Simulation::ComputeFullSimulation() {
    int iChunk = simulationArea.chunkDimensions.getDimSizes()[0];
    int jChunk = simulationArea.chunkDimensions.getDimSizes()[1];
    int kChunk = simulationArea.chunkDimensions.getDimSizes()[2];
    size_t simulationSize = simulationArea.simulationDimensions.getSimulationSize();

    cl_int err;
    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE , simulationSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in buffer"); 
    cl::Buffer rhsBuffer(oclSetup.context, CL_MEM_READ_WRITE , simulationSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an rhs buffer");                    
    cl::Buffer out_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY , simulationSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an out buffer");

    cl_int errorCode;
    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(in_p, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.p, nullptr, &oclSetup.event);
    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for simulation p.");
    if (PROFILING_ENABLED == 1) {
        oclSetup.event.wait();
        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clWriteToDevice);
    }

    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsBuffer, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.rhs, nullptr, &oclSetup.event);
    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for rhs.");
    if (PROFILING_ENABLED == 1) {
        oclSetup.event.wait();
        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clWriteToDevice);
    }
   
    oclSetup.kernel.setArg(2, rhsBuffer);
    oclSetup.kernel.setArg(3, iChunk);
    oclSetup.kernel.setArg(4, jChunk);
    oclSetup.kernel.setArg(5, kChunk);

    cl::Buffer finalResultBuffer;
    for (int iter = 1; iter <= simulationArea.iterations; iter += 1) {
        if (iter % 2 == 0) {
            CallKernel(out_p, in_p);
            finalResultBuffer = in_p;
        } else {
            CallKernel(in_p, out_p);
            finalResultBuffer = out_p;
        }     
    }

    errorCode = oclSetup.commandQueue.enqueueReadBuffer(finalResultBuffer, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.p, nullptr, &oclSetup.event);
    ErrorHelper::testError(errorCode, "Failed to read back from the device");
    if (PROFILING_ENABLED == 1) {
        oclSetup.event.wait();
        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clReadFromDevice);
    }
}

void Simulation::CallKernel(cl::Buffer in_p, cl::Buffer out_p) {
    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    int coreArea = simulationArea.chunkDimensions.getSimulationSize();

    oclSetup.kernel.setArg(0, in_p);
    oclSetup.kernel.setArg(1, out_p);
    
    EnqueueKernel(INFLOW, cl::NDRange(1*jHalChunk*kHalChunk));                
    EnqueueKernel(OUTFLOW, cl::NDRange(1*jHalChunk*kHalChunk));               
    EnqueueKernel(TOPBOTTOM, cl::NDRange(iHalChunk*jHalChunk*1));                
    EnqueueKernel(PERIODIC, cl::NDRange(iHalChunk*1*kHalChunk));
    
    int coreState = CORE;
    oclSetup.kernel.setArg(6, &coreState);
    cl_int errorCode;
    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(coreArea),
        cl::NullRange, nullptr, &oclSetup.event);
    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel with type: " + CORE);
    // clFinish(oclSetup.commandQueue.get());
    oclSetup.event.wait();

    if (PROFILING_ENABLED == 1) {
        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clKernelExecution);
    }
}

void Simulation::MeasureEventPerformance(cl::Event event, std::vector<double>& measurementsVec) {
    cl_ulong ev_start_time=(cl_ulong)0;
    cl_ulong ev_end_time=(cl_ulong)0;
    size_t return_bytes;

    clGetEventProfilingInfo(event.get(), CL_PROFILING_COMMAND_QUEUED,sizeof(cl_ulong),&ev_start_time, &return_bytes);
    clGetEventProfilingInfo(event.get(), CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &ev_end_time, &return_bytes);
    double run_time =(double)(ev_end_time - ev_start_time);
    measurementsVec.push_back(run_time);
}

void Simulation::ChunkAndCompute() {
    int iSimDim = simulationArea.simulationDimensions.getDimSizes()[0];
    int jSimDim = simulationArea.simulationDimensions.getDimSizes()[1];
    int kSimDim = simulationArea.simulationDimensions.getDimSizes()[2];

    int iDim = simulationArea.simulationDimensions.getDimSizes()[0] - 2;
    int jDim = simulationArea.simulationDimensions.getDimSizes()[1] - 2;
    int kDim = simulationArea.simulationDimensions.getDimSizes()[2] - 2;

    size_t coreSize = simulationArea.simulationDimensions.getSimulationSize();
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

    size_t haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();

    cl_int err;
    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in buffer"); 
    cl::Buffer rhsBuffer(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an rhs buffer");                    
    cl::Buffer out_p(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an out buffer");

    float *inChunk, *outChunk, *rhsChunk = NULL;
    posix_memalign((void**)&inChunk, 4096, haloChunkSize*sizeof(float));
    posix_memalign((void**)&outChunk, 4096, haloChunkSize*sizeof(float));
    posix_memalign((void**)&rhsChunk, 4096, haloChunkSize*sizeof(float));

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
                            // TODO: done for 1-point halo: could be more general; future work.
                            // Calculation for offsets are done based on the assumption that we're chunking jChunk = jDim. 
                            // TODO: 1D and 2D cases are currently not correct as there has been a change in assumptions, mainly, that user
                            // actually passes in padded data, and these cases were build in unpadded data in mind. 3D updated to work
                            // correctly as per assumptions.
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
                            // The 3D simulation, working correctly.    
                            } else {
                                int offsetStart = k*iSimDim*jSimDim + j*iSimDim + i_start;
                                int offsetEnd = offsetStart + iHalChunk;
                                int chunkIndex = k*iHalChunk*jHalChunk + j*iHalChunk;
                                
                                std::copy(p1 + offsetStart, p1 + offsetEnd, inChunk + chunkIndex);   
                                std::copy(simulationArea.rhs + offsetStart, simulationArea.rhs + offsetEnd, rhsChunk + chunkIndex);                       
                            }
                        }
                    }

                    // ****** openCL bit
                    // TODO: maybe move this bit into a separate method?
                    cl_int errorCode;
                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(in_p, CL_TRUE, 0, haloChunkSize * sizeof(float), inChunk, nullptr, &oclSetup.event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for inChunk.");
                    if (PROFILING_ENABLED == 1) {
                        oclSetup.event.wait();
                        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clWriteToDevice);
                    }
                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsBuffer, CL_TRUE, 0, haloChunkSize * sizeof(float), rhsChunk, nullptr, &oclSetup.event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for rhsChunk.");
                    if (PROFILING_ENABLED == 1) {
                        oclSetup.event.wait();
                        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clWriteToDevice);
                    }               

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

                    int coreState = CORE;
                    oclSetup.kernel.setArg(6, &coreState);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(currentIChunk*jChunk*kChunk),
                        cl::NullRange, nullptr, &oclSetup.event);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel with type: " + CORE);
                    oclSetup.event.wait();
                    if (PROFILING_ENABLED == 1) {
                        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clKernelExecution);
                    }

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(out_p, CL_TRUE, 0, haloChunkSize * sizeof(float), outChunk, nullptr, &oclSetup.event);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device");
                    if (PROFILING_ENABLED == 1) {
                        oclSetup.event.wait();
                        MeasureEventPerformance(oclSetup.event, performanceMeasurements.clReadFromDevice);
                    }

                    // TODO: 1D and 2D cases are currently not correct as there has been a change in assumptions, mainly, that user
                    // actually passes in padded data, and these cases were build in unpadded data in mind. 3D updated to work
                    // correctly as per assumptions.
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
                                int offsetStart = k*iHalChunk*jHalChunk + j*iHalChunk + 1;
                                int offsetEnd = offsetStart + currentIChunk;
                                int simulationIndex = k*iSimDim*jSimDim + j*iSimDim + i_start + 1;
                                std::copy(outChunk + offsetStart, outChunk + offsetEnd, p2 + simulationIndex);
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

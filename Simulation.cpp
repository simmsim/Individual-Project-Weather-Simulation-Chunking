#include "Simulation.h"
#include <iostream>
#include <math.h>
#include <chrono>

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
    // TODO: introduce error checing for this as well; for now return success.
    ProcessSimulation();
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

// We still want to retain the ability of processing whole chunks.
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
    float * finalResult = (float*)malloc(simulationSize * sizeof(float));

    cl_int errorCode;
    cl::Buffer inChunkBuffer;
    cl::Buffer rhsChunkBuffer;
    cl::Buffer outChunkBuffer;
    if (oclSetup.deviceProperties.deviceType == CL_DEVICE_TYPE_CPU) {
        inChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR , simulationSize * sizeof(float), simulationArea.p, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an in buffer"); 
        rhsChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR , simulationSize * sizeof(float), simulationArea.rhs, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an rhs buffer");
        outChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR , simulationSize * sizeof(float), finalResult, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an out buffer");
    } else {
        inChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_WRITE, simulationSize * sizeof(float), nullptr, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an in buffer"); 
        rhsChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_ONLY, simulationSize * sizeof(float), nullptr, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an rhs buffer");
        outChunkBuffer = cl::Buffer(oclSetup.context, CL_MEM_READ_WRITE , simulationSize * sizeof(float), nullptr, &errorCode);
        ErrorHelper::testError(errorCode, "Failed to create an out buffer");

        errorCode = oclSetup.commandQueue.enqueueWriteBuffer(inChunkBuffer, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.p, nullptr, &oclSetup.writeSimChunk);
        ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for simulation p.");
        if (PROFILING_ENABLED == 1) {
            oclSetup.writeSimChunk.wait();
            MeasureEventPerformance(oclSetup.writeSimChunk, performanceMeasurements.clWriteToDevice);
        }

        errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsChunkBuffer, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.rhs, nullptr, &oclSetup.writeRhsChunk);
        ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for rhs.");
        if (PROFILING_ENABLED == 1) {
            oclSetup.writeRhsChunk.wait();
            MeasureEventPerformance(oclSetup.writeRhsChunk, performanceMeasurements.clWriteToDevice);
        }
    }
   
    oclSetup.kernel.setArg(2, rhsChunkBuffer);
    oclSetup.kernel.setArg(3, iChunk);
    oclSetup.kernel.setArg(4, jChunk);
    oclSetup.kernel.setArg(5, kChunk);

    cl::Buffer finalResultBuffer;
    for (int iter = 1; iter <= simulationArea.iterations; iter += 1) {
        if (iter % 2 == 0) {
            CallKernel(outChunkBuffer, inChunkBuffer);
            finalResultBuffer = inChunkBuffer;
        } else {
            CallKernel(inChunkBuffer, outChunkBuffer);
            finalResultBuffer = outChunkBuffer;
        }     
    }

    if (oclSetup.deviceProperties.deviceType == CL_DEVICE_TYPE_CPU) {
        // CPU-only version is zero-copy. No need to enqueue read.
        if (simulationArea.iterations % 2 != 0) {
            float *temp = &*simulationArea.p;
            free(temp);
            simulationArea.p = &*finalResult;
        }
    } else {
        errorCode = oclSetup.commandQueue.enqueueReadBuffer(finalResultBuffer, CL_TRUE, 0, simulationSize * sizeof(float), simulationArea.p, nullptr, &oclSetup.readEvent);
        ErrorHelper::testError(errorCode, "Failed to read back from the device");
        if (PROFILING_ENABLED == 1) {
            oclSetup.readEvent.wait();
            MeasureEventPerformance(oclSetup.readEvent, performanceMeasurements.clReadFromDevice);
        }
    }  
}

void Simulation::CallKernel(cl::Buffer inChunkBuffer, cl::Buffer outChunkBuffer) {
    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    int coreArea = simulationArea.chunkDimensions.getSimulationSize();

    oclSetup.kernel.setArg(0, inChunkBuffer);
    oclSetup.kernel.setArg(1, outChunkBuffer);
    EnqueueKernel(INFLOW, cl::NDRange(1*jHalChunk*kHalChunk));                
    EnqueueKernel(OUTFLOW, cl::NDRange(1*jHalChunk*kHalChunk));               
    EnqueueKernel(TOPBOTTOM, cl::NDRange(iHalChunk*jHalChunk*1));                
    EnqueueKernel(PERIODIC, cl::NDRange(iHalChunk*1*kHalChunk));
    
    int coreState = CORE;
    oclSetup.kernel.setArg(6, &coreState);
    cl_int errorCode;
    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(coreArea),
        cl::NullRange, nullptr, &oclSetup.kernelEvent);
    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel with type: " + CORE);
    oclSetup.kernelEvent.wait();

    if (PROFILING_ENABLED == 1) {
        MeasureEventPerformance(oclSetup.kernelEvent, performanceMeasurements.clKernelExecution);
    }
}

void Simulation::MeasureEventPerformance(cl::Event event, std::vector<double>& measurementsVec) {
    cl_ulong ev_start_time=(cl_ulong)0;
    cl_ulong ev_end_time=(cl_ulong)0;
    size_t return_bytes;

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &ev_start_time);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &ev_end_time);
    double run_time =(double)(ev_end_time - ev_start_time);
    measurementsVec.push_back(run_time);
}

void Simulation::ChunkAndCompute() {
    // These are the dimensions of the core simulation area, excluding the hallos.
    int iDim = simulationArea.simulationDimensions.getDimSizes()[0] - 2;
    int jDim = simulationArea.simulationDimensions.getDimSizes()[1] - 2;
    int kDim = simulationArea.simulationDimensions.getDimSizes()[2] - 2;

    size_t coreSize = simulationArea.simulationDimensions.getSimulationSize();
    float *p2 = NULL;
    posix_memalign((void**)&p2, 4096, coreSize*sizeof(float));
    float *p1 = &*simulationArea.p;
    float *rhsChunk = &*simulationArea.rhs;
    
    // Core chunk dimensions, without hallos.
    int iChunk = simulationArea.chunkDimensions.getDimSizes()[0];
    int jChunk = simulationArea.chunkDimensions.getDimSizes()[1];
    int kChunk = simulationArea.chunkDimensions.getDimSizes()[2];

    // Some values that we need to know when dealing with cases, 
    // where the last chunk size != first n-1 chunk sizes.
    int currentIChunk = iChunk;
    int coreSizeij = iDim*jDim;
    int chunkSizeij = iChunk*jChunk;
    int noOfFullChunks = coreSizeij/chunkSizeij;
    int noOfLeftoverChunks = coreSizeij % chunkSizeij == 0 ? 0 : 1;
    int totalNoOfChunks = noOfFullChunks + noOfLeftoverChunks;

    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    size_t haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
    size_t fullHaloChunkSize = haloChunkSize;

    float *outChunk= NULL;
    posix_memalign((void**)&outChunk, 4096, haloChunkSize*sizeof(float));

    cl_int err;
    cl::Buffer inChunkBuffer(oclSetup.context, CL_MEM_READ_WRITE, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create a buffer for inChunkBuffer."); 
    cl::Buffer rhsChunkBuffer(oclSetup.context, CL_MEM_READ_ONLY, haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create a buffer for rhsChunkBuffer.");                    
    cl::Buffer outChunkBuffer(oclSetup.context, CL_MEM_WRITE_ONLY , haloChunkSize * sizeof(float), nullptr, &err);
    ErrorHelper::testError(err, "Failed to create a buffer for outChunkBuffer.");

    for (int n = 0; n < simulationArea.iterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {
                    std::chrono::steady_clock::time_point beginChunkSetup = std::chrono::steady_clock::now();
                    int currentChunkIndex = ((i_start + iChunk)/iChunk) - 1;

                    // Logic to deal with a leftover chunk.
                    if (i_start + iChunk > iDim) {
                        currentIChunk = (coreSizeij % chunkSizeij) / jChunk;
                        iHalChunk = currentIChunk + 2;
                        simulationArea.halChunkDimensions.updateDimSize(0, iHalChunk);
                        haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
                        currentChunkIndex = totalNoOfChunks - 1;
                    } 

                    // Above code changes simulationArea.halChunkDimensions object values; 
                    // When we're in a non-leftover chunk, we need to reset some values.
                    if (noOfLeftoverChunks != 0 && i_start + iChunk < iDim) {
                        currentIChunk = iChunk;
                        iHalChunk = currentIChunk + 2;
                        simulationArea.halChunkDimensions.updateDimSize(0, iHalChunk);
                        haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
                    }

                    // The logic is for a contiguous 1D. 
                    // 'start' value indicates where to start copying the chunk from the whole simulation.
                    int start = kHalChunk*jHalChunk*i_start;
                    std::chrono::steady_clock::time_point endChunkSetup = std::chrono::steady_clock::now();
                    performanceMeasurements.constructChunk.push_back(std::chrono::duration_cast<std::chrono::nanoseconds> (endChunkSetup - beginChunkSetup).count());

                    cl_int errorCode;   
                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(inChunkBuffer, CL_TRUE, 0, haloChunkSize * sizeof(float), p1 + start, nullptr, &oclSetup.writeSimChunk);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for inChunkBuffer.");
                    errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsChunkBuffer, CL_TRUE, 0, haloChunkSize * sizeof(float), rhsChunk + start, nullptr, &oclSetup.writeRhsChunk);
                    ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for rhsChunkBuffer.");

                    if (PROFILING_ENABLED == 1) {
                        oclSetup.writeSimChunk.wait();
                        oclSetup.writeRhsChunk.wait();
                        MeasureEventPerformance(oclSetup.writeSimChunk, performanceMeasurements.clWriteToDevice);
                        MeasureEventPerformance(oclSetup.writeRhsChunk, performanceMeasurements.clWriteToDevice);
                    }

                    oclSetup.kernel.setArg(0, inChunkBuffer);
                    oclSetup.kernel.setArg(1, outChunkBuffer);
                    oclSetup.kernel.setArg(2, rhsChunkBuffer);
                    oclSetup.kernel.setArg(3, currentIChunk);
                    oclSetup.kernel.setArg(4, jChunk);
                    oclSetup.kernel.setArg(5, kChunk);

                    if (i_start == 0) {
                        EnqueueKernel(INFLOW, cl::NDRange(1*jHalChunk*kHalChunk));
                    } 
                    
                    if (i_start + iChunk >= iDim) {
                        EnqueueKernel(OUTFLOW, cl::NDRange(1*jHalChunk*kHalChunk));
                    } 

                    if (kDim > 1) {
                        EnqueueKernel(TOPBOTTOM, cl::NDRange(iHalChunk*jHalChunk*1));
                    }
                    
                    // Periodic applies to each chunk since each chunk includes both sides.
                    EnqueueKernel(PERIODIC, cl::NDRange(iHalChunk*1*kHalChunk));

                    int coreState = CORE;
                    oclSetup.kernel.setArg(6, &coreState);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(currentIChunk*jChunk*kChunk),
                        cl::NullRange, nullptr, &oclSetup.kernelEvent);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a CORE kernel.");
                    // Ensure that core kernel computation finished before reading back.
                    oclSetup.kernelEvent.wait();
                    if (PROFILING_ENABLED == 1) {
                        MeasureEventPerformance(oclSetup.kernelEvent, performanceMeasurements.clKernelExecution);
                    }

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(outChunkBuffer, CL_TRUE, 0, haloChunkSize * sizeof(float), outChunk , nullptr, &oclSetup.readEvent);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device.");
                    if (PROFILING_ENABLED == 1) {
                        oclSetup.readEvent.wait();
                        MeasureEventPerformance(oclSetup.readEvent, performanceMeasurements.clReadFromDevice);
                    }

                    // Reintegration of the results into the simulation.
                    // The logic is for a contiguous 1D.
                    std::chrono::steady_clock::time_point beginReintegrate = std::chrono::steady_clock::now();
                    int bound = kHalChunk*jHalChunk;
                    int endChunk = fullHaloChunkSize - bound;
                    int startChunk = bound;
                    int pIndex = fullHaloChunkSize-bound + (fullHaloChunkSize-bound*2)*(currentChunkIndex-1);
                     
                    if (currentChunkIndex == 0) {
                        pIndex = 0;
                        startChunk = 0;
                    }

                    if (currentChunkIndex + 1 == totalNoOfChunks) {
                        endChunk = fullHaloChunkSize;
                    }

                    if (noOfLeftoverChunks == 1) {
                        endChunk = haloChunkSize;
                    }
                    
                    std::copy(outChunk + startChunk, outChunk + endChunk, p2 + pIndex);
                    std::chrono::steady_clock::time_point endReintegrate = std::chrono::steady_clock::now();
                    performanceMeasurements.reintegrateChunk.push_back(std::chrono::duration_cast<std::chrono::nanoseconds> (endReintegrate - beginReintegrate).count());
                }
            }
        }
        
        // The output array becomes our problem/input array for next iteration.
        float *temp = &*p1;
        p1 = &*p2;
        p2 = &*temp;
    }

    // Depending on the number of iterations, we might need to change pointer of the 
    // user's simulation area into the final result.
    if (simulationArea.iterations % 2 != 0 ){
        float *temp = &*simulationArea.p;
        free(temp);
        simulationArea.p = &*p1;
    } else {
        free(p2);
    }

    free(outChunk);
}

void Simulation::EnqueueKernel(int type, cl::NDRange range) {
    oclSetup.kernel.setArg(6, &type);
    cl_int errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, range, cl::NullRange, nullptr);
    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel for a condition");
}
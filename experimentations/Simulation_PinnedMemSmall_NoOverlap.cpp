// Overlap is removed here
// Pinned memory created and data placed to it at each iteration.
// Pinned memory gives fast transfers (enabled DMA) but copying data
// to pinned memory is too much of an overhead
void Simulation::ChunkAndCompute() {
    int iSimDim = simulationArea.simulationDimensions.getDimSizes()[0];
    int jSimDim = simulationArea.simulationDimensions.getDimSizes()[1];
    int kSimDim = simulationArea.simulationDimensions.getDimSizes()[2];

    int iDim = simulationArea.simulationDimensions.getDimSizes()[0] - 2;
    int jDim = simulationArea.simulationDimensions.getDimSizes()[1] - 2;
    int kDim = simulationArea.simulationDimensions.getDimSizes()[2] - 2;

    size_t coreSize = simulationArea.simulationDimensions.getSimulationSize();
    float *p2 = NULL;
    posix_memalign((void**)&p2, 4096, coreSize*sizeof(float));
    memset(p2, 0, coreSize * sizeof(float));

    float *p1 = &*simulationArea.p;
    float *rhsChunk = &*simulationArea.rhs;
    
    int iChunk = simulationArea.chunkDimensions.getDimSizes()[0];
    int jChunk = simulationArea.chunkDimensions.getDimSizes()[1];
    int kChunk = simulationArea.chunkDimensions.getDimSizes()[2];

    int currentIChunk = iChunk;
    int coreSizeij = iDim*jDim;
    int chunkSizeij = iChunk*jChunk;
    int noOfFullChunks = coreSizeij/chunkSizeij;
    int noOfLeftoverChunks = coreSizeij % chunkSizeij == 0 ? 0 : 1;
    int leftoverIChunk = noOfLeftoverChunks == 1 ? (coreSizeij - noOfFullChunks*chunkSizeij)/jChunk : 0;
    int totalNoOfChunks = noOfFullChunks + noOfLeftoverChunks;

    int iHalChunk = simulationArea.halChunkDimensions.getDimSizes()[0];
    int jHalChunk = simulationArea.halChunkDimensions.getDimSizes()[1];
    int kHalChunk = simulationArea.halChunkDimensions.getDimSizes()[2];

    int iHalChunkFullChunk = iHalChunk;

    size_t haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
    size_t fullHaloChunkSize = haloChunkSize;

    cl_int err;
    size_t bufferSize = haloChunkSize * sizeof(float);
    cl::Buffer in_p(oclSetup.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in buffer"); 
    cl::Buffer rhsBuffer(oclSetup.context, CL_MEM_READ_ONLY, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an rhs buffer");                    
    cl::Buffer out_p(oclSetup.context, CL_MEM_WRITE_ONLY, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an out buffer");

    cl::Buffer in_p_pinned(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in pinned buffer");
    cl::Buffer rhsBufferPinned(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in pinned buffer");
    cl::Buffer out_p_pinned(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, bufferSize, nullptr, &err);
    ErrorHelper::testError(err, "Failed to create an in pinned buffer");

    float * in_p_ptr = (float*)oclSetup.commandQueue.enqueueMapBuffer(in_p_pinned, CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, &err);
    float * rhs_ptr = (float*)oclSetup.commandQueue.enqueueMapBuffer(rhsBufferPinned, CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, &err);
    float * out_p_ptr = (float*)oclSetup.commandQueue.enqueueMapBuffer(out_p_pinned, CL_TRUE, CL_MAP_WRITE, 0, bufferSize, 0, NULL, &err);

    for (int n = 0; n < simulationArea.iterations; n++) {
        for (int k_start = 0; k_start < kDim; k_start += kChunk) {  
            for (int j_start = 0; j_start < jDim; j_start += jChunk) {
                for (int i_start = 0; i_start < iDim; i_start += iChunk) {
                     std::chrono::steady_clock::time_point beginChunkSetup = std::chrono::steady_clock::now();
                    // Logic to deal with a leftover chunk.
                    int rhsChunkIndex = ((i_start + iChunk)/iChunk) - 1;

                    if (i_start + iChunk > iDim) {
                        currentIChunk = (coreSizeij % chunkSizeij) / jChunk;
                        iHalChunk = currentIChunk + 2;
                        simulationArea.halChunkDimensions.updateDimSize(0, iHalChunk);
                        haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
                        rhsChunkIndex = totalNoOfChunks - 1;
                    } 

                    // Above code changes simulationArea.halChunkDimensions object values; 
                    // When we're in a non-leftover chunk, we need to reset some values.
                    if (noOfLeftoverChunks != 0 && i_start + iChunk < iDim) {
                        currentIChunk = iChunk;
                        iHalChunk = currentIChunk + 2;
                        simulationArea.halChunkDimensions.updateDimSize(0, iHalChunk);
                        haloChunkSize = simulationArea.halChunkDimensions.getSimulationSize();
                    }

                    // This is for contiguous 1D!!! Start index to know where to start copying the chunk from the whole simulation.
                    int start = kHalChunk*jHalChunk*i_start;
                    std::copy(p1 + start, p1 + start + haloChunkSize, in_p_ptr);
                    std::copy(rhsChunk + start, rhsChunk + start + haloChunkSize, rhs_ptr);

                    std::chrono::steady_clock::time_point endChunkSetup = std::chrono::steady_clock::now();
                    performanceMeasurements.constructChunk.push_back(std::chrono::duration_cast<std::chrono::nanoseconds> (endChunkSetup - beginChunkSetup).count());

                    // ****** openCL bit 
                    cl_int errorCode;   
                    // If it's the very first chunk, we need to write it first; subsequent chunks are overlapped with read down below.
                   
                        errorCode = oclSetup.commandQueue.enqueueWriteBuffer(in_p, CL_TRUE, 0, haloChunkSize * sizeof(float), in_p_ptr, nullptr, &oclSetup.writeSimChunk);
                        ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for inChunk.");
                        errorCode = oclSetup.commandQueue.enqueueWriteBuffer(rhsBuffer, CL_TRUE, 0, haloChunkSize * sizeof(float), rhs_ptr, nullptr, &oclSetup.writeRhsChunk);
                        ErrorHelper::testError(errorCode, "Failed to enqueue write buffer for rhsChunk.");

                    // Just before calling kernels, we check if everything was written to the device.
                    oclSetup.writeSimChunk.wait();
                    oclSetup.writeRhsChunk.wait();
                    if (PROFILING_ENABLED == 1) {
                        MeasureEventPerformance(oclSetup.writeSimChunk, performanceMeasurements.clWriteToDevice);
                        MeasureEventPerformance(oclSetup.writeRhsChunk, performanceMeasurements.clWriteToDevice);
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
                    oclSetup.kernel.setArg(6, coreState);
                    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(currentIChunk*jChunk*kChunk),
                        cl::NullRange, nullptr, &oclSetup.kernelEvent);
                    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel with type: " + CORE);
                    // Ensure that core kernel computation finished before reading back.
                    oclSetup.kernelEvent.wait();
                    if (PROFILING_ENABLED == 1) {
                        MeasureEventPerformance(oclSetup.kernelEvent, performanceMeasurements.clKernelExecution);
                    }

                    errorCode = oclSetup.commandQueue.enqueueReadBuffer(out_p, CL_TRUE, 0, haloChunkSize * sizeof(float), out_p_ptr , nullptr, &oclSetup.readEvent);
                    ErrorHelper::testError(errorCode, "Failed to read back from the device");

                    // Only wait for read to finish, whether write was finished will be checked in the next chunk's loop.
                    oclSetup.readEvent.wait();

                    if (PROFILING_ENABLED == 1) {
                        MeasureEventPerformance(oclSetup.readEvent, performanceMeasurements.clReadFromDevice);
                    }

                    // This is for contiguous 1D!!!
                    std::chrono::steady_clock::time_point beginReintegrate = std::chrono::steady_clock::now();
                    int bound = kHalChunk*jHalChunk;
                    int endChunk = fullHaloChunkSize - bound;
                    int startChunk = bound;
                    int pIndex = fullHaloChunkSize-bound + (fullHaloChunkSize-bound*2)*(rhsChunkIndex-1);
                     
                    if (rhsChunkIndex == 0) {
                        pIndex = 0;
                        startChunk = 0;
                    }

                    if (rhsChunkIndex + 1 == totalNoOfChunks) {
                        endChunk = fullHaloChunkSize;
                    }

                    if (noOfLeftoverChunks == 1) {
                        endChunk = haloChunkSize;
                    }
                    
                    std::copy(out_p_ptr + startChunk, out_p_ptr + endChunk, p2 + pIndex);
                    std::chrono::steady_clock::time_point endReintegrate = std::chrono::steady_clock::now();
                    performanceMeasurements.reintegrateChunk.push_back(std::chrono::duration_cast<std::chrono::nanoseconds> (endReintegrate - beginReintegrate).count());
                }
            }
        }
        
        // the output array becomes our problem/input array for next iteration
        float *temp = &*p1;
        p1 = &*p2;
        p2 = &*temp;
    }
    // Depending on the number of iterations, we might need to copy back the final result values
    // into the user's simulation area.
    if (simulationArea.iterations % 2 != 0 ){
        std::copy(p1, p1 + coreSize, simulationArea.p);
        free(p1);
        p2 = NULL;
    } else {
        free(p2);
        p1 = NULL;
    }

    // Fly away and be free.
    // free(outChunk);
    oclSetup.commandQueue.enqueueUnmapMemObject(in_p_pinned, in_p_ptr, 0, NULL);
    oclSetup.commandQueue.enqueueUnmapMemObject(rhsBufferPinned, rhs_ptr, 0, NULL);
    oclSetup.commandQueue.enqueueUnmapMemObject(out_p_pinned, out_p_ptr, 0, NULL);
}

void Simulation::EnqueueKernel(int type, cl::NDRange range) {
    oclSetup.kernel.setArg(6, type);
    cl_int errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, range, cl::NullRange, nullptr);
    ErrorHelper::testError(errorCode, "Failed to enqueue a kernel for a condition");
}


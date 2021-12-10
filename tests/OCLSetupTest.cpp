#include "../OCLSetup.h"

#include <CL/opencl.hpp>
#include <iostream>
#include <string>

int main(void) {
    cl_int errorCode;
    int deviceType = CPU;
    char * programFileName = (char*) "simpleKernel.cl";
    char * kernelName = (char*) "simple_kernel";

    OCLSetup oclSetup = OCLSetup(deviceType, programFileName, kernelName);

    int inputArraySize = 10;
    float *inputArray = (float*)malloc(sizeof(float)*inputArraySize);
    // initialize array with simple values
    for (int idx = 0; idx < inputArraySize; idx++) {
        inputArray[idx] = idx + 1;
    }

    float * outputArray = (float*)malloc(sizeof(float)*inputArraySize);

    cl::Buffer buf(oclSetup.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, inputArraySize * sizeof(float), inputArray);
    oclSetup.kernel.setArg(0, buf);

    cl::Event event;
    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(inputArraySize),
    cl::NullRange, nullptr, &event);

    event.wait();

    errorCode = oclSetup.commandQueue.enqueueReadBuffer(buf, CL_TRUE, 0, inputArraySize * sizeof(float), outputArray);

    for (int i = 0; i < inputArraySize; i++) {
        std::cout << outputArray[i] << " ";
    }
    std::cout << "\n";

    free(inputArray);
    free(outputArray);
}
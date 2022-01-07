#include "../OCLSetup.h"
#include "../chunking_algo/chunking_test_helpers.h"

#include <CL/opencl.hpp>
#include <iostream>
#include <string>

void checkOpenCLIsWorking() {
    cl_int errorCode;
    int deviceType = CPU;
    char * programFileName = (char*) "simpleKernel.cl";
    char * kernelName = (char*) "simple_kernel";
    float expected[10] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

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
    oclSetup.kernel.setArg(1, 2);
    oclSetup.kernel.setArg(2, 6);

    cl::Event event;
    errorCode = oclSetup.commandQueue.enqueueNDRangeKernel(oclSetup.kernel, cl::NullRange, cl::NDRange(inputArraySize),
    cl::NullRange, nullptr, &event);

    event.wait();

    errorCode = oclSetup.commandQueue.enqueueReadBuffer(buf, CL_TRUE, 0, inputArraySize * sizeof(float), outputArray);
    bool equals = assertEquals(expected, outputArray, inputArraySize);
    if (equals) {
        std::cout << "Test SUCCEEDED";
    } else {
        std::cout << "Test FAILED";
    }
    std::cout << " for checking whether OCLSetup is working correctly\n";

    free(inputArray);
    free(outputArray);
}

int main(void) {
    checkOpenCLIsWorking();
}
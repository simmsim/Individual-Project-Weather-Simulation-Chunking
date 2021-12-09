#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY

#include "OCLSetup.h"
#include "errorHelper.h"

#include <iostream>
#include <fstream>

OCLSetup::OCLSetup(int deviceType, char * programFileName,
                    char * kernelName) {
    CreateContext();
    SetDeviceProperties();
    CreateCommandQueue();
    CreateKernelFromProgram(programFileName, kernelName);
}

void OCLSetup::CreateContext() {
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> platformDevices;
    cl_int errorCode;

    errorCode = cl::Platform::get(&platforms);
    if (errorCode != CL_SUCCESS || platforms.size() == 0) {
        printError(errorCode, "Failed to find any platforms");
        exit(EXIT_FAILURE);
    }

    cl_device_type deviceType = deviceProperties.deviceType == 0 ? CL_DEVICE_TYPE_CPU : CL_DEVICE_TYPE_GPU;
    platforms[0].getDevices(deviceType, &platformDevices);
    if (platformDevices.size() == 0) {
        std::cout << "No devices found for the type: " << deviceType << " . Looking for other devices...";
        platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);
        testError(platformDevices.size() > 0 ? CL_SUCCESS : -1, "Failed to find any devices on platform");
    }

    device = platformDevices[0];
    context = cl::Context(device);
}

void OCLSetup::SetDeviceProperties() {
    deviceProperties.deviceName = device.getInfo<CL_DEVICE_NAME>();
    deviceProperties.maxMemAllocSize = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
}

void OCLSetup::CreateCommandQueue() {
    cl_int errorCode;
    commandQueue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &errorCode);
    // TODO: more specific descriptions, specify device?
    testError(errorCode, "Failed to create command queue for the device");
}

void OCLSetup::CreateKernelFromProgram(char * programFileName, 
                                       char * kernelName) {
    cl_int errorCode;                                       

    std::ifstream file(programFileName);
    testError(file.is_open() ? CL_SUCCESS : -1,
                "Failed to read a file that was open");

    std::string prog(
            std::istreambuf_iterator<char>(file),
            (std::istreambuf_iterator<char>())
            );

    cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length()+1));
    cl::Program program(context, source, &errorCode); 
    testError(errorCode, "Failed to create the program");
    kernel = cl::Kernel(program, kernelName, &errorCode);
    testError(errorCode, "Failed to create the kernel");
}


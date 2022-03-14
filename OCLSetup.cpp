#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY

#include "OCLSetup.h"

#include <iostream>
#include <fstream>

OCLSetup::OCLSetup(int deviceType, char * programFileName,
                   char * kernelName, int * err) {
    *err = OCLSETUP_SUCCESS;
    if (CreateContext(deviceType) != CL_SUCCESS) {
        *err = OCLSETUP_FAILURE;
    }
    SetDeviceProperties();
    if (CreateCommandQueue() != CL_SUCCESS) {
        *err = OCLSETUP_FAILURE;
    }
    if (CreateKernelFromProgram(programFileName, kernelName) != CL_SUCCESS) {
        *err = OCLSETUP_FAILURE;
    }
}

int OCLSetup::CreateContext(int deviceType_) {
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> platformDevices;
    cl_int errorCode;

    errorCode = cl::Platform::get(&platforms);
    if (errorCode != CL_SUCCESS || platforms.size() == 0) {
        ErrorHelper::printError(errorCode, "Failed to find any platforms");
        return errorCode;
    }

    deviceProperties.deviceType = deviceType_ == CPU ? CL_DEVICE_TYPE_CPU : CL_DEVICE_TYPE_GPU;
    platforms[0].getDevices(deviceProperties.deviceType, &platformDevices);
    if (platformDevices.size() == 0) {
        // deviceType is only a number. TODO: nitpick, provide with device type name
        std::cout << "No devices found for the type: " << deviceProperties.deviceType << ". Will use any other available device\n";
        platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);
        ErrorHelper::testError(platformDevices.size() > 0 ? CL_SUCCESS : -1, "Failed to find any devices on platform");
        return CL_DEVICE_NOT_AVAILABLE;
    }

    device = platformDevices[0];
    context = cl::Context(device);
    return CL_SUCCESS;
}

void OCLSetup::SetDeviceProperties() {
    deviceProperties.deviceName = device.getInfo<CL_DEVICE_NAME>();
    deviceProperties.maxMemAllocSize = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
    deviceProperties.maxGlobalMemSize = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
}

int OCLSetup::CreateCommandQueue() {
    cl_int errorCode;
    commandQueue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &errorCode);
    ErrorHelper::testError(errorCode, "Failed to create command queue for the device");
    return errorCode;
}

int OCLSetup::CreateKernelFromProgram(char * programFileName, 
                                      char * kernelName) {
    cl_int errorCode;                                       

    std::ifstream file(programFileName);
    ErrorHelper::testError(file.is_open() ? CL_SUCCESS : -1,
                "Failed to open file for reading");

    std::string prog(
            std::istreambuf_iterator<char>(file),
            (std::istreambuf_iterator<char>())
            );

    cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length()+1));
    cl::Program kernelProgram(context, source, &errorCode); 
    ErrorHelper::testError(errorCode, "Failed to create the program");
    std::vector<cl::Device> devices;
    devices.push_back(device);
    errorCode = kernelProgram.build(devices);
    ErrorHelper::testError(errorCode, "Failed to build the program");
    cl_build_status status = program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device);
    if (status == CL_BUILD_ERROR) {
        std::string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cerr << "Build log for " << deviceProperties.deviceName << ":" << std::endl
                << buildlog << std::endl;
    }
    program = kernelProgram;
    kernel = cl::Kernel(program, kernelName, &errorCode);
    ErrorHelper::testError(errorCode, "Failed to create the kernel");

    return errorCode;
}

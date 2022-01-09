#include <CL/opencl.hpp>
#include <string>

#define CL_HPP_TARGET_OPENCL_VERSION 200

#include "ErrorHelper.h"

#define CPU 0
#define GPU 1

typedef struct devicePropertiesStruct {
    cl_device_type deviceType; 
    std::string deviceName;
    long maxMemAllocSize;
} devicePropertiesStruct;

class OCLSetup {
    public:
        cl::Device device;
        cl::Context context;
        cl::Program program;
        cl::Kernel kernel;
        cl::CommandQueue commandQueue;
        devicePropertiesStruct deviceProperties;

        OCLSetup() = default;
        OCLSetup(int deviceType, char * programFileName,
                 char * kernelName);

        void CreateContext(int deviceType);
        void SetDeviceProperties();
        void CreateCommandQueue();
        void CreateKernelFromProgram(char * programFileName, 
                                     char * kernelName);
};
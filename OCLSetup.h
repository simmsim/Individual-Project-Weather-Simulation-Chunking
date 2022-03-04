#include <CL/opencl.hpp>
#include <string>
// CL/cl.hpp

#include "ErrorHelper.h"
#include "ErrorCodes.h"

#define CPU 0
#define GPU 1

typedef struct devicePropertiesStruct {
    cl_device_type deviceType; 
    std::string deviceName;
    long maxMemAllocSize;
    long maxGlobalMemSize;
} devicePropertiesStruct;

class OCLSetup {
    public:
        cl::Device device;
        cl::Context context;
        cl::Program program;
        cl::Kernel kernel;
        cl::CommandQueue commandQueue;
        cl::Event event;
        cl::Event kernelEvent;
        cl::Event readEvent;
        devicePropertiesStruct deviceProperties;

        OCLSetup() = default;
        OCLSetup(int deviceType, char * programFileName,
                 char * kernelName, int * err);

        int CreateContext(int deviceType);
        void SetDeviceProperties();
        int CreateCommandQueue();
        int CreateKernelFromProgram(char * programFileName, 
                                     char * kernelName);
};
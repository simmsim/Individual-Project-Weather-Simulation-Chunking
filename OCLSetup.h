#include <CL/opencl.hpp>
#include <string>

#define CPU 0
#define GPU 1

class OCLSetup {
    public:
        cl::Device device;
        cl::Context context;
        cl::Program program;
        cl::Kernel kernel;
        cl::CommandQueue commandQueue;

        struct deviceProperties {
            int deviceType; // User defines to use CPU or GPU, 
            // if specified type not available, just default to 
            // what's available :-)
            std::string deviceName;
            cl_ulong maxMemAllocSize;
            // TODO: Later, add any properties needed
            // to determine optimal chunk size; 
            // or to check if user's not exceeding available
            // mem on device
        } deviceProperties;

        OCLSetup() = default;
        OCLSetup(int deviceType, char * programFileName,
                    char * kernelName);

        void CreateContext();
        void SetDeviceProperties();
        void CreateCommandQueue();
        void CreateKernelFromProgram(char * programFileName, 
                    char * kernelName);

                    

};
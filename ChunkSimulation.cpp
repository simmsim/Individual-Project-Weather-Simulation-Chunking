#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY

#include <CL/opencl.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>


//#include "errorHelper.h"
#include "Simulation.h"


cl::Context CreateContext(Simulation simulation) 
{
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> platformDevices, allDevices, ctxDevices;
    cl::string device_name;
    cl_uint i;
    cl_int errNum;

    errNum = cl::Platform::get(&platforms);
    if (errNum != CL_SUCCESS || platforms.size() == 0) {
        std::cerr << "Failed to find any platforms" << std::endl;
        return NULL;
    }
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);
    simulation.setDevice(platformDevices[0]);
    

    cl::Context context(platformDevices);
    simulation.setContext(context);
    ctxDevices = simulation.getContext().getInfo<CL_CONTEXT_DEVICES>();
    for (i=0; i<ctxDevices.size(); i++) {
        device_name = ctxDevices[i].getInfo<CL_DEVICE_NAME>();
        std::cout << "Device: " << device_name.c_str() << std::endl;
        std::cout << "Device compute units: " << ctxDevices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        std::cout << "Device max work group size " << ctxDevices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
        std::cout << "Device max mem alloc size" << ctxDevices[i].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << std::endl;
        std::cout << "Device max mem align" << ctxDevices[i].getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>() << std::endl;
    }

    return context;
}

cl::CommandQueue CreateCommandQueue(Simulation simulation) {
    cl::CommandQueue queue = cl::CommandQueue(simulation.getContext(), simulation.getDevice(), CL_QUEUE_PROFILING_ENABLE);
}

void chunkAndSend(Simulation simulation) {

}

int main(void) {
    Simulation simulation;

    cl::Context context;
    context = CreateContext(simulation);

    cl::CommandQueue commandQueue;
    commandQueue = CreateCommandQueue(simulation);

    std::ifstream file("sor_kernel.cl");

    std::string prog(
            std::istreambuf_iterator<char>(file),
            (std::istreambuf_iterator<char>())
            );

    cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length()+1));

    cl::Program program(context, source);
    simulation.program = program;
 
    cl::Kernel kernel(program, "sor_superkernel");
    simulation.kernel = kernel;

    //chunkAndSend
}



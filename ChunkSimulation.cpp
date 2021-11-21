#ifdef __APPLE__
#include <OpenCL/opencl.hpp>
#else
#include <CL/opencl.hpp>
#endif

#include <iostream>

#include "errorHelper.h"
#include "Simulation.h"

using namespace std;


cl::Context CreateContext(Simulation simulation) 
{
    vector<cl::Platform> platforms;
    vector<cl::Device> platformDevices, allDevices, ctxDevices;
    cl::string device_name;
    cl_uint i;
    cl_int errNum;

    errNum = cl::Platform::get(&platforms);
    if (errNum != CL_SUCCESS || platforms.size() == 0) {
        cerr << "Failed to find any platforms" << endl;
        return NULL;
    }
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);
    simulation.setDevice(platformDevices[0]);
    

    cl::Context context(platformDevices);
    simulation.setContext(context);
    ctxDevices = context.getInfo<CL_CONTEXT_DEVICES>();
    for (i=0; i<ctxDevices.size(); i++) {
        device_name = ctxDevices[i].getInfo<CL_DEVICE_NAME>();
        cout << "Device: " << device_name.c_str() << endl;
        cout << "Device compute units: " << ctxDevices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl;
        cout << "Device max work group size " << ctxDevices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl;
        cout << "Device max mem alloc size" << ctxDevices[i].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << endl;
        cout << "Device max mem align" << ctxDevices[i].getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>() << endl;
    }

    return context;
}

cl::CommandQueue CreateCommandQueue(Simulation simulation) {
    cl::CommandQueue queue = cl::CommandQueue(simulation.getContext(), simulation.getDevice(), CL_QUEUE_PROFILING_ENABLE);
}

int main(void) {
    Simulation simulation;

    cl::Context context;
    context = CreateContext(simulation);


    cl::CommandQueue commandQueue;
    commandQueue = CreateCommandQueue(simulation);
    

}


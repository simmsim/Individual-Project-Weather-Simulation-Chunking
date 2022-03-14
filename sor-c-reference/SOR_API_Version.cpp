#include "../Simulation.h"
#include "sor_api_params.h"
#include "../utility/domain_generator.h"
#include "../utility/print_exec_time_to_file.h"

#if defined(MEASURE_EVENT_PERFORMANCE) 
    #define PROFILING_ENABLED 1
#else
    #define PROFILING_ENABLED 0
#endif

int main(int argc, char* argv[]) {
    SimulationRange coreRange = SimulationRange(ip, jp, kp);
    SimulationRange chunkRange = SimulationRange(cip, cjp, ckp);

    char * programFileName = (char*) "../../sor_kernel.cl";
    char * kernelName = (char*) "sor_superkernel";

    int iterations = atoi(argv[2]);
    int deviceType = atoi(argv[3]);
    int halo = 1;
    float maxSimulationAreaMemUsage = 100;

    int pSize = coreRange.getSimulationSize();
    float *p, *rhs = NULL;
    posix_memalign((void**)&p, 4096, pSize*sizeof(float));
    posix_memalign((void**)&rhs, 4096, pSize*sizeof(float));

    generateDomain(p, rhs, coreRange);
    int err;
    std::chrono::steady_clock::time_point beginWithSetup = std::chrono::steady_clock::now();
    Simulation simulation = Simulation(deviceType, programFileName, kernelName, &err);
    if (err != 0) {
        std::cout << "OpenCL setup failed with error code " << err << "\n";
        return -1;
    }
    std::chrono::steady_clock::time_point beginSimulationOnly = std::chrono::steady_clock::now();
    err = simulation.RunSimulation(p, rhs, halo, iterations, coreRange, chunkRange, maxSimulationAreaMemUsage); 
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    if (err != 0) {
        std::cout << "Simulation execution failed with error code " << err << "\n";
        return -1;
    }
   
   float * actualSimulationArea = simulation.getSimulationArea().p;
   int index = F3D2C(jp, kp, 0,0,0, 1,1,1);
    float actualValue =  actualSimulationArea[index];
    std::cout << "Value was " << actualValue << " for index " << index << "\n";

    char * fileName = (char *)malloc(strlen(argv[1]) + 1);
    strcpy(fileName, argv[1]);

    print_to_file(beginWithSetup, end, fileName, "ElapsedOCL");
    print_to_file(beginSimulationOnly, end, fileName, "ElapsedSimulationOnly");
    if (PROFILING_ENABLED) {
        print_total_to_file(simulation.getPerformanceMeasurements().clKernelExecution, fileName, "Kernel");
        print_total_to_file(simulation.getPerformanceMeasurements().clWriteToDevice, fileName, "Write");
        print_total_to_file(simulation.getPerformanceMeasurements().clReadFromDevice, fileName, "Read");
        print_total_to_file(simulation.getPerformanceMeasurements().constructChunk, fileName, "ConstructChunk");
        print_total_to_file(simulation.getPerformanceMeasurements().reintegrateChunk, fileName, "ReintegrateChunk");
    }
    print_new_line_to_file(fileName);

    free(fileName);
    free(actualSimulationArea);
    free(rhs);
}
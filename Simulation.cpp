#include "Simulation.h"

Simulation::Simulation(int deviceType, char * programFileName,
                    char * kernelName) {
    OCLSetup(deviceType, programFileName, kernelName);
}
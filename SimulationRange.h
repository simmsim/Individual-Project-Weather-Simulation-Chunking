#include <iostream>

class SimulationRange {
    private:
        int dimSizes[3];
        int dimensions;
        size_t simulationSize;

    public:    
        SimulationRange() : dimensions(0) {
            dimSizes[0] = 0;
            dimSizes[1] = 0;
            dimSizes[2] = 0;
            simulationSize = 0;
        }

        SimulationRange(int dim0) : dimensions(1) {
            dimSizes[0] = dim0;
            dimSizes[1] = 1;
            dimSizes[2] = 1;
            simulationSize = dim0;
        }

        SimulationRange(int dim0, int dim1) : dimensions(2) {
            dimSizes[0] = dim0;
            dimSizes[1] = dim1;
            dimSizes[2] = 1;
            simulationSize = dim0 * dim1;
        }

        SimulationRange(int dim0, int dim1, int dim2) : dimensions(3) {
            dimSizes[0] = dim0;
            dimSizes[1] = dim1;
            dimSizes[2] = dim2;
            simulationSize = dim0 * dim1 * dim2;
        }

        int getDimensions() {
            return dimensions;
        }

        int* getDimSizes() {
            return dimSizes;
        }

        int getSimulationSize() {
            return simulationSize;
        }

        void incrementDimensionsBy(int value) {
            simulationSize = 1;
            for (int i = 0; i < dimensions; i++) {
                dimSizes[i] += value;
                simulationSize *= dimSizes[i];
            }
        }

        void updateDimSize(int index, int newDimSize) {
            if (index > 3 || index < 0) {
                std::cout << "Invalid index value is out of range.\n";
                return;
            }

            simulationSize = (simulationSize/dimSizes[index]) * newDimSize;
            dimSizes[index] = newDimSize;
        }
}; 
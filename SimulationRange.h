class SimulationRange {
    private:
        int dimSizes[3];
        int dimensions;

    public:
        SimulationRange() : dimensions(0) {
            dimSizes[0] = 0;
            dimSizes[1] = 0;
            dimSizes[2] = 0;
        }

        SimulationRange(int dim0) : dimensions(1) {
            dimSizes[1] = dim0;
            dimSizes[1] = 0;
            dimSizes[2] = 0;
        }

        SimulationRange(int dim0, int dim1) : dimensions(2) {
            dimSizes[1] = dim0;
            dimSizes[1] = dim1;
            dimSizes[2] = 0;
        }

        SimulationRange(int dim0, int dim1, int dim2) : dimensions(3) {
            dimSizes[1] = dim0;
            dimSizes[1] = dim1;
            dimSizes[2] = dim2;
        }

        int getDimensions() {
            return dimensions;
        }

        int* getDimSizes() {
            return dimSizes;
        }
}; 
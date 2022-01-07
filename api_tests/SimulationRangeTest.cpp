#include "../SimulationRange.h"

#include <iostream>

#define IS_TRUE(result) {std::cout << "Test for " << __FUNCTION__ << ""; if (!result) std::cout << " failed on line " << __LINE__ << std::endl; else std::cout << " succeeded.\n"; }

bool assertEquals(SimulationRange expected, SimulationRange actual) {
    bool dimEqual = true;
    for (int i = 0; i < expected.getDimensions() && i < actual.getDimensions(); i++) {
        if (expected.getDimSizes()[i] != actual.getDimSizes()[i]) {
            dimEqual = false;
        }
    }

    return (expected.getDimensions() == actual.getDimensions()) &&
           (expected.getSimulationSize() == actual.getSimulationSize()) &&
           dimEqual;
}

void rangeIsCopiedAndIncrementedCorrectly() {
    SimulationRange range = SimulationRange(4, 6, 4);

    SimulationRange actualhHalRange = SimulationRange(range);
    actualhHalRange.incrementDimensionsBy(2);

    SimulationRange expectedHalRange = SimulationRange(6, 8, 6);
    
    IS_TRUE(assertEquals(expectedHalRange, actualhHalRange));
}

int main(void) {
    rangeIsCopiedAndIncrementedCorrectly();
}
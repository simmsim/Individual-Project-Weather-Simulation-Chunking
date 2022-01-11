#include "../SimulationRange.h"
#include "../utility/assertions.h"


void testRangeIsCopiedAndIncrementedCorrectly() {
    SimulationRange range = SimulationRange(4, 6, 4);

    SimulationRange actualhHalRange = SimulationRange(range);
    actualhHalRange.incrementDimensionsBy(2);

    SimulationRange expectedHalRange = SimulationRange(6, 8, 6);
    
    assertEquals(expectedHalRange, actualhHalRange, "testRangeIsCopiedAndIncrementedCorrectly");
}

void testRangeDimSizeIsUpdatedAtIndexCorrectly() {
    SimulationRange range = SimulationRange(10, 5, 5);

    range.updateDimSize(0, 2);

    assertEquals(2, range.getDimSizes()[0], "testRangeDimSizeIsUpdatedAtIndexCorrectly: index updated.");
    assertEquals(50, range.getSimulationSize(), "testRangeDimSizeIsUpdatedAtIndexCorrectly: simulation size updated.");
}

int main(void) {
    testRangeIsCopiedAndIncrementedCorrectly();
    testRangeDimSizeIsUpdatedAtIndexCorrectly();

    return 0;
}
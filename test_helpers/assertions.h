#include <iostream>
#include <string>

void assertEquals(float * expected, float * actual, int coreSize,
                  std::string testName) {
    for (int i = 0; i < coreSize; i++) {
        if (expected[i] != actual[i]) {
            std::cout << "Values were different at index " << i << ": expected {"
            << expected[i] << "} but actual was {" << actual[i] << "}\n";
            std::cout << "Test FAILED for " << testName << std::endl;
            return;
        }
    }
    std::cout << "Test SUCCEEDED for " << testName << std::endl;  
}
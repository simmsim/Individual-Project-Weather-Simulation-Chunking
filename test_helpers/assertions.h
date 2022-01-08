#include <iostream>
#include <string>


void printTestResultMessage(bool succeeded, std::string testName) {
    if (succeeded) {
        std::cout << "Test SUCCEEDED for " << testName << std::endl; 
    } else {
        std::cout << "Test FAILED for " << testName << std::endl;
    }
}

void assertEquals(float * expected, float * actual, int coreSize,
                  std::string testName) {
    bool succeded = true;
    for (int i = 0; i < coreSize; i++) {
        if (expected[i] != actual[i]) {
            std::cout << "Values were different at index " << i << ": expected {"
            << expected[i] << "} but actual was {" << actual[i] << "}\n";
            succeded = false;
        }
    }
    printTestResultMessage(succeded, testName); 
}

void assertEquals(int expected, int actual, std::string testName) {
    bool succeeded = true;
    if (expected != actual) {
        std::cout << "Values are different: expected {" << expected << 
        "} but actual was {" << actual << "}\n";
        succeeded = false;
    }
    printTestResultMessage(succeeded, testName);
}
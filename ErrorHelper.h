#include <CL/opencl.hpp>
#include <stdlib.h>
#include <string>

class ErrorHelper {
    public:
        static void printError(cl_int errorCode, const char* description);
        static int testError(cl_int errorCode, std::string description);
        static int testError(cl_int errorCode);
        static const char* getErrorCodeDescription(cl_int errorCode);
};
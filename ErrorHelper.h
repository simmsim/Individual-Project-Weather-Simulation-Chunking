#include <CL/opencl.hpp>
#include <stdlib.h>
#include <string>

#define CL_HPP_TARGET_OPENCL_VERSION 200

class ErrorHelper {
    public:
        static void printError(cl_int errorCode, const char* description);
        static void testError(cl_int errorCode, std::string description);
        static const char* getErrorCodeDescription(cl_int errorCode);
};
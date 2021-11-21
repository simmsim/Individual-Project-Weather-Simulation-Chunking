#include <CL/opencl.hpp>

class Simulation {
    private:
        cl::Device device;
        cl::Context context;

    public:
        Simulation() = default;

        ~Simulation();

        void setDevice(cl::Device newDevice) {
            device = newDevice;
        }

        void setContext(cl::Context newContext) {
            context = newContext;
        }

        cl::Device getDevice() {
            return device;
        }

        cl::Context getContext() {
            return context;
        }
};
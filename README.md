# Individual-Project-Weather-Simulation-Chunking

## Topic Description
Many physical models (e.g. weather simulations) have a very large problem space (e.g. the portion of the atmosphere being simulated). Frequently, the problem size is limited by the available memory, esp. on consumer grade accelerators such as GPUs.

The aim of this project is to investigate processing full simulation in chunks so that it becomes possible to run much larger problems by consecutively running part of the data on the device.

The goal is to develop and assess the performance of a C++ API that would perform chunking of the simulation, send chunks to the device for processing, read back and reintegrate the result chunks. 

## How to Run

Navigate to performance_testing/api_version/. It contains a bashscript called `runApiVersion.sh` where you can specify the size of the simulation (including 1-point halo), the size of the chunk, specifying how core should be chunked, number of runs, number of iterations and device number (0 for CPU, 1 for GPU). Then run the script by:
```
$ ./runApiVersion.sh
```
Make sure that the user has the executable rights on the script.

The script will run SOR_API_Version located in simulation_runners/ in which the synthetic simulation data is generated and where the call to run the simulation is made. SOR_API_Version also prints performance execution times to a log file, which at the end of the processing is scanned by bashcript which calculates average values.

## Unit Tests

There is a set of test suites in the unit_tests/ folder: `ChunkingSimulationTest.cpp`, `OCLSetupTest.cpp`, `SimulationRangeTest.cpp`. They can be used to confirm the correctness of various parts of the program. To run a single test suite, for example, ChunkingSimulationTest.cpp:
```
$ make ChunkingSimulationTest
$ ./ChunkingSimulationTest
```

To run all test suites:
```
$ make
$ make run
```
After running tests, you should get an output in the console specifying which tests succeeded and which of them failed. If a test fails, the output will indicate what was the expected value and what it actually got.

To clean up the executables:
```
$ make clean
```
## Other Notes
Older than 2.0 versions of OpenCL seem to fail execution if the state code for the kernel is a pointer. If kernel execution is failing with vague errors, try changing the state code from a pointer to a simple int.

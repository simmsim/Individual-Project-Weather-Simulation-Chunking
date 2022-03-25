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

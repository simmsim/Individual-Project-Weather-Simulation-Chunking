# Individual-Project-Weather-Simulation-Chunking

## Topic Description
Many physical models (e.g. weather simulations) have a very large problem space (e.g. the portion of the atmosphere being simulated). Frequently the problem size is limited by the available memory, esp. on accelerators such as GPUs.

The aim of this project is to investigate partitioning of the data in memory so that it becomes possible to run much larger problems by consecutively running part of the data on the device.

The goal is to develop and assess the performance of a C++ API that would perform chunking of a problem space, send chunks to the device for processing, read back and reintegrate the result chunks. 


## How to Run
```
$ g++ ChunkSimulation.cpp -lOpenCL -o chunkSimulation
$ ./chunkSimulation
```

## Unit Tests
Folder `tests` contains a number of unit tests for various parts of the API. (TBD)

Folder `chunking_algo` contains the chunking logic and files that test the logic for 1D, 2D and 3D cases. This folder contains a Makefile, so to run all the unit tests in this folder:
```
$ make
$ make run
```

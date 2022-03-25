CPP=g++
CC=-g -std=c++11 -O3 -Wall
LIBS=-lOpenCL

targets=ChunkingSimulationTest OCLSetupTest SimulationRangeTest
all: $(targets)
.PHONY: all

ChunkingSimulationTest:
	$(CPP) $(CC) ChunkingSimulationTest.cpp -o ChunkingSimulationTest ../Simulation.cpp ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)

OCLSetupTest:
	$(CPP) $(CC) -DOCLSETUPTEST OCLSetupTest.cpp -o OCLSetupTest ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)

SimulationRangeTest:
	$(CPP) $(CC) SimulationRangeTest.cpp -o SimulationRangeTest

run: $(targets)
	./ChunkingSimulationTest
	./OCLSetupTest
	./SimulationRangeTest

clean:
	rm $(targets)
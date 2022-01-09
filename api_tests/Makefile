CPP=g++
CC=-g
LIBS=-lOpenCL

targets=ChunkingSimulationTest OCLSetupTest SimulationRangeTest
all: $(targets)

$(targets): 
	$(CPP) $(CC) ChunkingSimulationTest.cpp -o ChunkingSimulationTest ../Simulation.cpp ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)
	$(CPP) $(CC) OCLSetupTest.cpp -o OCLSetupTest ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)
	$(CPP) $(CC) SimulationRangeTest.cpp -o SimulationRangeTest

run: $(targets)
	./ChunkingSimulationTest
	./OCLSetupTest
	./SimulationRangeTest

clean:
	rm $(targets)
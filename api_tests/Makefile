CPP=g++
CC=-g
LIBS=-lOpenCL

targets=ChunkingSimulationTest OCLSetupTest
all: $(targets)

$(targets): 
	$(CPP) $(CC) ChunkingSimulationTest.cpp -o ChunkingSimulationTest ../Simulation.cpp ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)
	$(CPP) $(CC) OCLSetupTest.cpp -o OCLSetupTest ../OCLSetup.cpp ../ErrorHelper.cpp $(LIBS)

run: $(targets)
	./ChunkingSimulationTest
	./OCLSetupTest

clean:
	rm $(targets)
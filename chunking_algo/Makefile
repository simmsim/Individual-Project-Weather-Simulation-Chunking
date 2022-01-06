CPP=g++

targets=Chunking1DTest Chunking2DTest Chunking3DTest
all: $(targets)

$(targets): %: %.cpp
	$(CPP) $< -o $@ 

run: $(targets)
	./$<

clean:
	rm $(targets)


FLAGS=-Wall -pedantic -std=c++11 -fopenmp -O3
OUTPUT_DIR=output
BINARY=$(OUTPUT_DIR)/a.out
# In the case CXX isn't set to g++ in default, uncomment the following line
#CXX=g++

.PHONY: run

run: compile
#	./$(BINARY)

compile: $(BINARY)

$(BINARY): taskparallel.cpp
	$(CXX) $^ -o $@ $(FLAGS)

.PHONY: clean

clean:
	rm -rf $(OUTPUT_DIR)/*

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Werror
SANITIZERS := -fsanitize=address,undefined

BIN_DIR := bin

run: raii_wrapper
	./$(BIN_DIR)/raii_wrapper

bin:
	mkdir -p bin

lifetime_demo: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/lifetime_demo.cpp \
        -o $(BIN_DIR)/lifetime_demo

raii_wrapper: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/raii_wrapper.cpp \
        -o $(BIN_DIR)/raii_wrapper

clean:
	rm -r $(BIN_DIR)/*

.PHONY: run bin clean

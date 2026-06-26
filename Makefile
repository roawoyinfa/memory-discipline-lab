CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Werror
SANITIZERS := -fsanitize=address,undefined

BIN_DIR := bin

run: struct_analysis_tool
	./$(BIN_DIR)/struct_analysis_tool

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

move_only_type: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/move_only_type.cpp \
        -o $(BIN_DIR)/move_only_type

bump_allocator: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/bump_allocator.cpp \
        -o $(BIN_DIR)/bump_allocator

struct_analysis_tool: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/struct_analysis_tool.cpp \
        -o $(BIN_DIR)/struct_analysis_tool

clean:
	rm -r $(BIN_DIR)/*

.PHONY: run bin clean

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Werror
SANITIZERS := -fsanitize=address,undefined

BIN_DIR := bin

run: lifetime_demo
	./$(BIN_DIR)/lifetime_demo

lifetime_demo: bin
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
        src/lifetime_demo.cpp \
        -o $(BIN_DIR)/lifetime_demo
bin:
	mkdir -p bin

clean:
	rm -r $(BIN_DIR)/*

.PHONY: run bin clean

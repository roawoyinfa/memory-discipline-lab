CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Werror
SANITIZERS := -fsanitize=address,undefined

BIN_DIR := bin

bin:
	mkdir -p bin

clean:
	rm -r $(BIN_DIR)/*

.PHONY: bin clean

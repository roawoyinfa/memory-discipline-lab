CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Werror
SANITIZERS := -fsanitize=address,undefined

SRC_DIR := src
BIN_DIR := bin

PROGRAMS := \
	lifetime_demo \
	raii_wrapper \
	move_only_type \
	bump_allocator \
	struct_analysis_tool

all: $(PROGRAMS)

# Each program depends on its executable in bin/
$(PROGRAMS): %: $(BIN_DIR)/%

# Build an executable from the corresponding source file
$(BIN_DIR)/%: $(SRC_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS)  $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean


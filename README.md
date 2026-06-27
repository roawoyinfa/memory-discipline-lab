# Memory Discipline Lab

A suite of small, focused C++ programs demonstrating object lifetime, resource
ownership, move semantics, memory allocation, and undefined behavior. Each 
implementation is accompanied with design rationale, invariants, and supporting 
documentation.

## Overview

The primary goals of the project are to:

- Develop a precise understanding of object lifetime and ownership.
- Demonstrate correct resource management using **Resource Acquisition Is Initialization (RAII)**.
- Explore move semantics and exclusive ownership.
- Implement a simple allocator while reasoning about memory layout and allocation correctness.
- Document common sources of undefined behavior and explain them from both the C++ language and hardware perspectives.

## Deliverables

| Deliverable                 | Description                                                              |
| --------------------------- | ------------------------------------------------------------------------ |
| RAII Wrapper                | Manages a POSIX file descriptor using deterministic lifetime management. |
| Move-only Type              | Demonstrates ownership transfer and moved-from object semantics.         |
| Bump Allocator              | Implements a simple arena allocator with reset semantics.                |
| Struct Layout Analysis Tool | Reports member offsets, padding, and overall structure layout.           |

## Documentation

The repository includes the following design documents.

| Document                | Purpose                                                                                                                                                                      |
| ----------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `DESIGN.md`             | Explains non-obvious implementation decisions and the rationale behind them.                                                                                                 |
| `INVARIANTS.md`         | Defines the correctness invariants that each deliverable must maintain.                                                                                                      |
| `UNDEFINED_BEHAVIOR.md` | Documents common undefined behavior patterns encountered during development, including minimal reproducers, sanitizer output, and language- and hardware-level explanations. |

## Requirements

The project was developed and tested with the following tools:

- GNU Make
- A C++20-compatible compiler (tested with g++)
- Linux or another POSIX-compatible operating system
- AddressSanitizer (ASan)
- UndefinedBehaviorSanitizer (UBSan)

## Building

Build all deliverables by running:

```bash
make
```

The executables are placed in the `bin/` directory.

To build an individual deliverable, invoke its corresponding target:

```bash
make lifetime_demo
make raii_wrapper
make move_only_type
make bump_allocator
make struct_analysis_tool
```

Remove all generated executables with:

```bash
make clean
```

## Running

The default run target builds every deliverable and executes the struct layout
analysis tool:

```bash
make run
```

Alternatively, execute any deliverable directly from the `bin/` directory:

```bash
./bin/lifetime_demo
./bin/raii_wrapper
./bin/move_only_type
./bin/bump_allocator
./bin/struct_analysis_tool
```

## Verification

The project was validated using the following tools:

- AddressSanitizer (ASan)
- UndefinedBehaviorSanitizer (UBSan)

The sanitizer diagnostics discussed throughout the project are reproduced in `UNDEFINED_BEHAVIOR.md`.

## References

This project was developed while studying:

- *Computer Systems: A Programmer's Perspective* — Bryant & O'Hallaron
- *Effective Modern C++* — Scott Meyers
- LearnCpp.com
- cppreference.com

## License

Licensed under the MIT License. See the `LICENSE` file for details.

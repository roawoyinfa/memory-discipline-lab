# Undefined Behavior Catalogue

## Purpose

This document explains five common undefined behavior patterns encountered 
during this project.

## How to Read This Document

Each section documents an undefined behavior pattern. Every section includes a 
minimal reproducer, the corresponding sanitizer output, an explanation of why 
the program has undefined behavior according to the C++ language rules, and 
description of the observed hardware behavior.

## 1. Uninitialized Variable Read

### Pattern

Reading the value of an uninitialized variable.

### Minimal Reproducer

```cpp
#include <iostream>

int main()
{
    int x;

    if (x > 0) {
        std::cout << "Positive\n";
    } else {
        std::cout << "Negative or Zero\n";
    }
}
```

### Sanitizer Output

```bash
==378303==WARNING: MemorySanitizer: use-of-uninitialized-value

  Uninitialized value was created by an allocation of 'x' in the stack frame of function 'main'

SUMMARY: MemorySanitizer: use-of-uninitialized-value
Exiting
```

### Language-Level Explanation

The local variable `x` is default-initialized and therefore holds an 
indeterminate value. Reading the value of the variable produces undefined
behavior. 

The C++ standard imposes no requirement on the program's execution after an 
invalid read occurs. Therefore, compilers may assume that such read never
occurs and optimize the program under that assumption. For example, the compiler
may erase entire sections of code that use the uninitialized variable because 
they are classified as impossible. As a result, the program's behavior become 
unpredictable and is not limited to producing arbitrary garbage values.

### Hardware-Level Behavior

On most systems, an uninitialized variable occupies stack memory whose content
includes leftover bits from previous use of that memory. Therefore, reading an
uninitialized variables retrieves whatever bit pattern already exists in that
memory location. 

The observed value may differ between program executions, optimization levels,
or machines. Furthermore, the retrieved bit pattern doesn't determine the 
program's behavior because the C++ standard already classifies reading an 
uninitialized variable as undefined behavior. The physical bit pattern is 
irrelevant because the compiler may assume the read never occurs and optimize
accordingly.

## 2. Signed Integer Overflow

### Pattern

Producing or storing a value that falls outside the representable range of a 
signed integer type.

### Minimal Reproducer

```cpp
#include <limits>

int main()
{
    int max_val{std::numeric_limits<int>::max()};

    // Undefined behavior: signed integer overflow 
    int overflowed_val{max_val + 1};
}
```

### Sanitizer Output

```bash
runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

### Language-Level Explanation

The local variable `max_val` is initialized to the largest representable value 
of type `int`. Evaluating the expression `max_val + 1` produces a value that 
exceeds the representable range of `int`, resulting in undefined behavior.

Historically, the standard did not mandate a specific representation of signed
integers, leaving signed integer overflow undefined to give implementations 
freedom. Since C++20, however, the standard requires signed integers to use two's
representation. Despite this change, signed integer overflow remains undefined
behavior. This allows compilers to assume signed integer overflow never occurs 
and to optimize programs based on that assumption. 

As a result, if a program triggers signed integer overflow, the compiler may 
transform, eliminate, or simplify the program in a way that may not be valid 
if overflow were allowed to occur.

### Hardware-Level Behavior

Many modern systems represent signed integers using two's complement 
representation. In a fixed-width two's complement representation, if the 
mathematical result of an expression greater than largest representable positive
value, the result overflows and wraps around to the negative end of the range.
Likewise, if the mathematical result is less than smallest representable negative
value, the result overflows and wraps around to the positive end of the range. 

However, the hardware behavior doesn't determine the behavior of a C++ program  
that triggers signed integer overflow because the C++ standard already classifies 
signed integer overflow as undefined behavior. Consequently, a conforming 
program cannot rely on wrap-around behavior occurring. 

## 3. Unsequenced Side Effects

### Pattern

Performing multiple unsequenced operations on the same variable within a single
expression.

### Minimal Reproducer

```cpp
int main()
{
    int i{};

    // Undefined behavior: Multiple unsequenced modifications
    i = ++i + ++i; 
}
```

### Sanitizer Output

Instrumentation tools such as UndefinedbehaviorSanitizer detect undefined 
behavior during program execution. However, unsequenced side effects are 
generally diagnosed at compile-time rather than at runtime because the order of
evaluation is unspecified. As a result, the compiler may optimize the program
before any instrumentation is applied.

Consequently, detecting unsequenced side effects requires compiler diagnostics,
such as Clang's `-Wunsequenced` or GCC's `-Wsequence-point`, often enabled 
through broader warning options such as `-Wall`. 

```bash
warning: multiple unsequenced modifications to 'i' [-Wunsequenced]
    i = ++i + ++i;
        ^     ~~
1 warning generated.
```

### Language-Level Explanation

The expression `++i + ++i` exhibits undefined behavior because the two `++i`
side effects are unsequenced relative to each other. Since the C++ standard does
not define the order in which these side effects occur, the compiler is allowed
to assume that unsequenced side effects never occurs and optimize the program 
accordingly by reordering, combining, or eliminating the operations entirely.

### Hardware-Level Behavior

The processor executes only the instructions emitted by the compiler. Because 
the expression exhibits undefined behavior, the compiler is free to emit
any instruction sequence it considers valid or eliminate the computation 
entirely. 

## 4. Dangling Pointer Dereference

### Pattern

Dereferencing a dangling pointer.

### Minimal Reproducer

```cpp
#include <iostream>

int main()
{
    int* ptr{};
    
    {
        int y{5};
        ptr = &y;
    }

    std::cout << *ptr; 
}
```

### Sanitizer Output

```bash
==410661==ERROR: AddressSanitizer: stack-use-after-scope...READ of size 4...
```

### Language-Level Explanation

The pointer variable `ptr` becomes dangling after the block because it still 
holds the address of `y`, but the lifetime of `y` has ended when the block is
exited. Dereferencing `ptr` therefore attempts to access an object that no 
longer exists, which is undefined behavior.

Since the C++ standard imposes no requirement on the behavior of a program 
that dereferences a dangling pointer, the compiler may assume such accesses
never occur and optimize the program under that assumption.

### Hardware-Level Behavior

At the hardware level, a pointer is simply a memory address. After `y`'s 
lifetime ended, the numerical address stored in `ptr` remains unchanged, but
no valid C++ object exists at that address.

Depending on the subsequent program execution, the referenced storage may

- still contain the previous bytes of `y`,
- have been reused by another object,
- have been rewritten, or
- be inaccessible

Dereferencing the pointer simply loads the content of that address. The hardware
has no notion of lifetimes.

However, this hardware behavior does not determine the behavior of a C++ program
that dereferences a dangling pointer. The C++ standard classifies dereferencing a 
dangling pointer as undefined behavior, so a conforming program cannot rely on 
reading the previous object, a new object, or any other particular result.

## 5. Out-of-Bounds Array Access

### Pattern

Reading from or writing to an array outside its bounds.

### Minimal Reproducer

```cpp
int main()
{
    int arr[3]{1, 2, 3};
    
    // Undefined behavior: write outside the bounds of the array.
    arr[3] = 42;
}
```

### Sanitizer Output

```bash
==417927==ERROR: AddressSanitizer: stack-buffer-overflow on address... WRITE of size 4...
```

### Language-Level Explanation

The array `arr` contains storage for exactly three elements of type `int`, 
indexed from `0` to `2`. The statement `arr[3] = 42` writes an element 
beyond the bounds of the array, which is undefined behavior.

Because the C++ standard imposes no requirements on the behavior of a program
that accesses an array outside its bounds, the compiler may assume such accesses
never occur and optimize the program under that assumption.

### Hardware-Level Behavior

At the hardware level, an array is a contiguous block of storage. The processor
simply performs memory accesses using addresses and does not where a C++ array
begins or ends. 

Consequently, an out-of-bounds access simply reads from or writes to the 
computed address. Depending on the address, the access may:

- overwrite adjacent objects,
- read unrelated data,
- access unmapped memory and trigger an hardware fault, or
- appear to work without any visible failure.

However, this hardware behavior doesn't determine the behavior of a C++ program
that reads from or writes to an array outside it bounds. The C++ standard
classifies out-of-bound array access as undefined behavior, so a conforming 
program cannot rely on any particular outcome.

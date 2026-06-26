# DESIGN

## Purpose

This document explains non-obvious design decisions made throughout the project.
It focuses on decisions that are not immediately apparent from the source code,
including implementation tradeoffs, resource-management strategies, ownership models,
and allocator behavior.

## Deliverable: RAII Wrapper for a file handle and socket fd

### D1. Sentinel value for invalid file descriptors

#### context

The RAII wrapper must represent both owned and unowned state.

#### Decision

Use `fd_ = -1` as the invalid state.

#### Alternatives considered

- Define a separate `bool` flag (e.g., `bool owns_resource`)
- Use `std::optional<int>`

#### Rationale

POSIX guarantees that valid file descriptors are non-negative. Using `-1` as a
sentinel removes redundant state, avoids synchronization between the boolean
ownership flag and `fd_`, and eliminates the need for `std::optional<int>`'s
engaged-state checks when accessing `fd_`.

#### Consequences

The implementation must ensure that moved-from and objects that have released
ownership set `fd_` to `-1`.

### D2. Ownership Release via std::exchange

#### Context

`release()` returns the managed file descriptor while giving up ownership.

#### Decision

Use `std::exchange()` to replace `fd_` with `-1` and to return the original file
descriptor value.

#### Alternatives

Manually replacing `fd_` with `-1` and returning the original value by creating
an temporary variable.

```cpp
[[nodiscard]] int release() noexcept {
    int actual_fd = fd_;
    fd_ = -1;
    return actual_fd;
}
```

#### Rationale

`std::exchange()` explicitly communicates intent in a single operation, improving
readability. It makes the operation atomic in the sense that there's no
intermediate state where `fd_` has been reset to `-1` but the original value has
not yet been captured.

#### Consequence

The implementation depends on `<utility>` and assumes `-1` remains the sentinel
value for representing non-owning state.

### D3. Self-reset protection

#### Context

`reset()` releases the currently owned `fd_` and optionally acquires ownership
of a replacement descriptor.

#### Decision

Preventing self-reset by checking whether the replacement file descriptor is
already owned before closing the current descriptor.

#### Rationale

Without the check, calling `reset(fd_)` would close the currently owned file
descriptor and then store the same, already-closed file descriptor back in
the wrapper. The wrapper would subsequently believe it owns a valid file
descriptor even though the underlying resource has been release.

Preventing self-reset preserves the ownership model by ensuring the wrapper
does not transition into a state where it believes it owns an invalid file
descriptor.

#### Consequence

Callers may safely pass the currently owned file descriptor to `reset()` without
risking accidental resource invalidation.

## Deliverable: Bump allocator

### B1. Arena storage ownership

#### Context

Before allocations can occur, a contiguous block of memory must be reserved.
The allocator then fulfills allocation requests from this reserved region.

#### Decision

Allocate the arena as a fixed-size heap allocation managed by `std::unique_ptr`.
The arena storage shall be allocated with an alignment sufficient to satisfy
the maximum alignment requirement supported by the allocator.

#### Alternatives

- Allocate the arena on the stack
- Use `std::vector<std::byte>`
- Use raw heap allocation

#### Rationale

The operating system imposes a limit on the maximum stack size available to each
thread. Exceeding this limit causes stack overflow, regardless of how much stack
space the program would theoretically require. Consequently, if the arena is
large enough to exceed the maximum available stack space or the size is not known
at compile-time, the heap is generally a safer option. That said, placing the
arena on the stack may be appropriate if it is small, short-lived, and its
lifetime is tied to the current scope.

`std::vector<std::byte>` provides automatic resource management through RAII,
reducing the risk of memory leaks and simplifying lifetime management. However,
if the vector's capacity is exceeded, it may reallocate its storage to a larger
memory block. When this occurs, the underlying storage is relocated and all
pointers into the previous storage become invalid. Although preallocating
sufficient capacity with `std::vector::reserve()` can prevent reallocation,
this approach relies on never exceeding the reserved capacity. Arena allocators
typically require a fixed-size memory region whose address remains stable
for the allocator entire lifetime.

Raw heap allocation via `new`, on the other hand, provides fixed-address
semantics, but requires explicit memory management.

`::operator new(size, std::align_val_t{alignof(std::max_align_t)})`, managed by
`std::unique_ptr`, combines the safety of automatic resource management and the
stability of fixed-address semantics.

To efficiently and predictably produce correctly aligned allocations, the arena
is allocated with an explicit alignment corresponding to the most strictly
aligned fundamental scalar type supported by the implementation.

If support for over-aligned types is required, the arena should instead be
allocated with an explicitly specified alignment appropriate for those
requirements.

`std::unique_ptr<std::byte[]>` is not used because it does not guarantee
over-aligned allocations; its allocation is only guaranteed to satisfy the
alignment requirements of the allocated type, which for `std::byte` is typically
1-byte.

#### Consequence

The aligned allocation must be paired with a corresponding aligned deallocation
that calls `::operator delete` with the matching alignment.

### B2. Allocator-owned arena storage

#### Context

The allocator requires a contiguous memory region from which allocation requests
are fulfilled. A design decision is required to determine whether the allocator
owns this reserved region or merely operates on memory provided by a caller.

#### Decision

The allocator allocates and owns the arena internally. Arena storage is acquired
during allocator construction and released during allocator destruction.

#### Alternatives

Pass an externally allocated memory region to the allocator and treat the arena
as borrowed storage rather than owned storage.

#### Rationale

Allocator-owned arena storage simplifies the ownership model by binding the
arena's lifetime to the allocator's. This ensures that the allocator can never
outlive the arena, thereby preventing dangling references and the resulting
undefined behavior.

An externally provided arena offers flexibility because the caller can choose
the memory source. However, it introduces additional lifetime requirements
that must be documented and enforced by the caller.

Because this project is intended as a self-contained demonstration of bump
allocation, simplicity and correctness are prioritized over configurability.

#### Consequence

The allocator owns and manages its arena storage and cannot operate directly on
caller-provided memory without changes to the interface.

### B3. Allocation interface return-type

#### Context

The allocator must return a pointer to the allocated storage. The appropriate
pointer type for representing the allocated memory must be defined.

#### Decision

Expose allocated storage through a `void*` return type.

#### Alternatives

- `std::byte*`
- `char*`

#### Rationale

Returning `void*` mirrors the type-agnostic semantic of low-level allocation
facilities such as `malloc`, allowing the allocator to service allocation
requests for arbitrary types without requiring the allocation interface to
be templated.

#### Consequence

The caller is responsible for explicitly casting the returned `void*` to the
appropriate pointer type before constructing or accessing objects in the
allocated storage.

Internally, the allocator performs pointer arithmetic using `std::byte*`
because arithmetic on `void*` is not supported.

### B4. Exhaustion policy

#### Context

An allocation request may exceed the arena's available capacity.

#### Decision

Return `nullptr` when an allocation request cannot be fulfilled.

#### Alternatives

- Throw `std::bad_alloc`.
- Terminate the program using `assert` or `std::abort`.

#### Rationale

Returning `nullptr` mirrors the interface of low-level allocation facilities
such as `malloc` and allows the caller to decide how allocation failure
should be handled.

Throwing exceptions would require the allocation interface to participate in
exception propagation and will impose additional requirements on the caller.
Since the bump allocator is intended to be simple, lightweight, low-level
primitive, explicit failure signaling through `nullptr` return value keeps
the interface simple and predictable.

Terminating the program on exhaustion would prevent the caller from implementing
alternative strategies such as releasing memory or falling back to another
allocator. Returning `nullptr` preserves these options while keep the allocator
independent of application-specific error handling.

#### Consequence

The allocation interface can be declared `noexcept`, reflecting allocation
failure is reported through the return value rather than by throwing an
exception.

Callers must treat `nullptr` return value as an allocation failure and check the
return value before accessing the allocated memory. Failure to do so results in
undefined behavior.

### B5. Alignment verification using offset

#### Context

The allocator must ensure every returned allocation satisfies the requested
alignment requirement.

#### Decision

Verify allocation alignment using the current allocation offset and the
requested alignment.

#### Rationale

The arena is allocation is aligned to `max_alignment_`.

Furthermore,

- `max_alignment_` is a non-negative power of two.
- Every valid requested alignment is a non-negative power of two.
- Every valid requested alignment is less than or equal to `max_alignment_`.

Therefore, every valid requested alignment is a divisor of `max_alignment_`:

$$
\mathtt{arena\_base\_address} \bmod \mathtt{alignment} = 0
$$

An allocation is aligned when:

$$
(\mathtt{arena\_base\_address} + \mathtt{offset\_}) \bmod \mathtt{alignment} = 0
$$

Consequently, aligning an allocation reduces to aligning the offset:

$$
\mathtt{offset\_} \bmod \mathtt{alignment} = 0
$$

#### Consequence

If a larger alignment than `max_alignment_` were permitted, this invariant
would no longer hold because the arena base itself might not satisfy the
requested alignment.

### B6. Two-stage capacity validation to prevent integer overflow

#### Context

Before fulfilling the allocation request, the allocator must determine whether
sufficient spaces remains in the arena after accounting for both alignment
padding and the requested allocation size. This calculation involve unsigned
integer arithmetic which can wrap around on overflow and produce incorrect
capacity checks if not handled carefully.

#### Decision

Validate allocation capacity in two stages.

First, verify that the required alignment padding fits in the arena's available
space.

```cpp
if (padding > available_space)  {
    return nullptr;
}
```

Second, verify that the requested allocation size fits after accounting for the
alignment padding

```cpp
if (size > available_space - padding) {
    return nullptr;
}
```

#### Alternatives

- Compute capacity using a single expression `padding + size > available_space`.
- Compute capacity using a single subtraction `size > available_space - padding`.

#### Rationale

A capacity check such as `padding + size > available_space` is not safe because
the addition may overflow before the comparison is performed. For example,
if size == SIZE_MAX and padding > 0, unsigned integer arithmetic wraps modulo
SIZE_MAX + 1, producing an incorrect result.

A capacity check such as `size > available_space - padding` is also not safe on
its own because the subtraction may underflow when `padding > available_space`.
Unsigned underflow likewise wraps, yielding a large value and potentially allowing
an allocation that should fail.

To avoid both failure modes, the allocator performs capacity validation in two
stages:

1. Verify that the required alignment padding fits within the remaining arena
   capacity.
2. Verify that the requested allocation size fits within the remaining capacity
   after accounting for the padding.

This approach avoids intermediate arithmetic that could overflow or underflow
before the capacity check is evaluated.

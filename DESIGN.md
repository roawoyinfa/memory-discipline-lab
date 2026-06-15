# DESIGN

## Purpose

This document explains non-obvious design decisions made throughout the project. 
It focuses on decisions that are not immediately apparent from the source code,
including implementation tradeoffs, resource-management strategies, ownership models,
and allocator behaviour.

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

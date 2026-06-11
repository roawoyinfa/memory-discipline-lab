# Invariants

## Purpose

This document defines the correctness invariants and safety guarantees of the 
Memory Discipline Lab deliverables. These invariants describe the conditions 
that must always remain true during program execution. Violating any invariant
indicates undefined behaviour, memory corruption, ownership violations, or 
logical errors.

## Deliverable: RAII Wrapper 

### Purpose 

The RAII wrapper manages an operating-system **file descriptor (fd)** for an open
file or socket. The wrapper prevents resource leaks, double cleanup, and ownership
errors that can occur when resource management is done manually. The RAII wrapper
achieves this by binding resource management to object lifetime. When the object 
is constructed, it acquires ownership of the fd. When the object is 
destroyed, it automatically releases the fd.

### Invariants

#### R1. Release uniqueness

Every successfully acquired fd is released exactly once.

#### R2. Release obligation

Every successfully acquired fd is eventually released.

#### R3. Unique ownership

Exactly one RAII wrapper object owns a given fd at any time. 

#### R4. Moved-from validity 

After ownership transfer, the moved-from object remains valid for reassignment
and destruction.

## Deliverable: Move-only Type 

### Purpose

Some types represent exclusive ownership of a resource or capability. Allowing
copy operations would cause multiple objects to incorrectly believe they exclusively 
own the same resource or capability, resulting in double cleanup and undefined
behaviour. To preserve unique ownership, move-only types disable copy operations 
(i.e., the copy constructor and copy assignment operator). Ownership can instead
be transferred through move construction or assignment. 

### Invariants

#### M1. Unique ownership

Exactly one object owns a given resource at any time.

#### M2. Ownership transfer

Move construction and move assignment transfer ownership from the source object
to the destination object.

#### M3. Post-Move validity

A moved-from object remains valid for destruction and reassignment but does not
own the transferred resource.

## Deliverable: Bump Allocator

### Purpose

A bump allocator (also called linear or arena allocator) is a simple memory 
allocator that allocates memory by advancing a pointer through a preallocated
memory block. A large memory block is reserved up front, and the allocator 
maintains a pointer to the next available allocation location. Each allocation 
returns the current pointer and advances it by the requested size. Individual 
allocations are not freed. Instead, all memory is reclaimed at once when the
allocator is reset or destroyed.

Because allocation consists of only pointer arithmetic, bump allocators provide 
extremely fast allocation with minimal overhead. They also exhibit good spatial 
and temporal locality, reducing access latency since objects allocated near each
other are likely to reside in the same cache line. 

The major drawback is that individual allocations cannot be efficiently freed. 
Memory can only be reclaimed in bulk. Consequently, bump allocators are most
effective when objects have similar lifetimes. When objects have significantly
different lifetimes, memory can be retained longer than necessary because 
short-lived  objects cannot be reclaimed independently of long-lived ones.

### Invariants

#### B1. Arena bounds

Every allocation returned by the allocator always lies within the arena's memory
block.

#### B2. Monotonic allocations

The next allocation position never moves backward except during reset.

#### B3. Non-overlapping allocations

The allocator never returns overlapping memory regions for two active allocations.

#### B4. Alignment correctness

Every returned allocation satisfies the requested alignment requirement.

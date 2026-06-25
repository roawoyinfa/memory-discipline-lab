#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <new>

class BumpAllocator {
public:
    using size_type = std::size_t;

    // Preconditions:
    // - alignment must be a non-zero power of two
    // - alignment <= arena_alignment_ for allocate()
    // Violations are programming errors, enforced with assert.
    explicit BumpAllocator(
        size_type capacity,
        size_type alignment = alignof(std::max_align_t)) noexcept
        : arena_{allocate_arena(capacity, alignment), ArenaDeleter{alignment}}
        , max_alignment_{alignment}
        , capacity_{capacity}
        , offset_{0}
    {
    }

    // Defaulted because arena_ owns and releases the arena.
    ~BumpAllocator() = default;

    // Bump allocator has unique ownership of its arena
    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    // Every member provides correct move semantics and no custom behaviour
    // is needed.
    BumpAllocator(BumpAllocator&&) = default;
    BumpAllocator& operator=(BumpAllocator&&) = default;

    [[nodiscard]] void* allocate(
        size_type size,
        size_type alignment = alignof(std::max_align_t)) noexcept
    {
        if (size == 0
            || alignment == 0
            || !std::has_single_bit(alignment)
            || alignment > max_alignment_) {
            return nullptr;
        }

        if (offset_ == capacity_) {
            return nullptr;
        }

        // The remainder indicates how far offset_ is past the previous
        // aligned position.
        const size_type remainder{offset_ % alignment};

        // To reach the next aligned position, advance by the amount required
        // to complete the current alignment boundary.
        const size_type aligned_offset{
            remainder == 0
                ? offset_
                : offset_ + (alignment - remainder)};
        const size_type padding{aligned_offset - offset_};

        // Equivalent branchless form:
        //
        // const size_type aligned_offset{
        //     (offset + alignment - 1) & ~(alignment - 1)};
        // const size_type padding{aligned_offset - offset};
        //
        // Since alignment is guaranteed to be a power of two:
        //
        //     alignment      = 00010000  (16)
        //     alignment - 1  = 00001111
        //
        // The value ~(alignment - 1) forms a mask that clears the low
        // bits responsible for misalignment.
        //
        // Adding (alignment - 1) ensures that any non-aligned offset is
        // advanced into the next alignment boundary. Applying the mask
        // then rounds the result down to that boundary, yielding the
        // smallest aligned offset greater than or equal to offset_.
        //
        // Example:
        //
        //     offset_ = 13
        //     alignment = 8
        //
        //     (13 + 7) & ~7
        //     = 20 & 11111000
        //     = 16
        //
        // Thus aligned_offset is the next multiple of alignment.

        const size_type available_space{capacity_ - offset_};

        if (padding > available_space) {
            return nullptr;
        }

        if (size > available_space - padding) {
            return nullptr;
        }

        offset_ += padding + size;

        return static_cast<void*>(arena_.get() + aligned_offset);
    }

    void reset() noexcept
    {
        offset_ = 0;
    }

    [[nodiscard]] size_type capacity() const noexcept
    {
        return capacity_;
    }

    [[nodiscard]] size_type used() const noexcept
    {
        return offset_;
    }

    [[nodiscard]] size_type remaining() const noexcept
    {
        return capacity_ - offset_;
    }

private:
    struct ArenaDeleter {
        size_type arena_alignment{};
        void operator()(std::byte* ptr) const noexcept
        {
            if (!ptr) {
                return;
            }

            ::operator delete(ptr, std::align_val_t{arena_alignment});
        }
    };

    std::byte* allocate_arena(
        size_type capacity,
        size_type alignment) noexcept
    {
        assert(alignment != 0);
        assert(std::has_single_bit(alignment));

        if (capacity == 0) {
            return nullptr;
        }

        return static_cast<std::byte*>(
            ::operator new(
                capacity,
                std::align_val_t{alignment},
                std::nothrow));
    }

    std::unique_ptr<std::byte[], ArenaDeleter> arena_;

    size_type max_alignment_{};
    size_type capacity_{};
    size_type offset_{};
};

int main()
{
    BumpAllocator allocator{64, 16};

    std::cout << "Capacity: " << allocator.capacity() << '\n';
    std::cout << "Used: " << allocator.used() << '\n';
    std::cout << "Remaining: " << allocator.remaining() << '\n';

    // ---------------------------------------------------------
    // Basic allocation
    // ---------------------------------------------------------

    void* p1{allocator.allocate(8, 8)};

    assert(p1 != nullptr);
    assert(reinterpret_cast<std::uintptr_t>(p1) % 8 == 0);

    std::cout << "\nAfter allocating 8 bytes aligned to 8:\n";
    std::cout << "Used: " << allocator.used() << '\n';
    std::cout << "Remaining: " << allocator.remaining() << '\n';

    // ---------------------------------------------------------
    // Alignment padding
    // ---------------------------------------------------------

    void* p2 = allocator.allocate(1, 16);

    assert(p2 != nullptr);
    assert(reinterpret_cast<std::uintptr_t>(p2) % 16 == 0);

    std::cout << "\nAfter allocating 1 byte aligned to 16:\n";
    std::cout << "Used: " << allocator.used() << '\n';
    std::cout << "Remaining: " << allocator.remaining() << '\n';

    // ---------------------------------------------------------
    // Verify allocations don't overlap
    // ---------------------------------------------------------

    constexpr std::size_t p1_size{8};

    const auto p1_begin{reinterpret_cast<std::uintptr_t>(p1)};
    const auto p1_end{p1_begin + p1_size};

    const auto p2_begin{reinterpret_cast<std::uintptr_t>(p2)};

    assert(p1_end <= p2_begin);

    // ---------------------------------------------------------
    // Multiple valid alignments
    // ---------------------------------------------------------

    void* p3 = allocator.allocate(4, 4);
    void* p4 = allocator.allocate(8, 8);

    assert(p3 != nullptr);
    assert(p4 != nullptr);

    assert(reinterpret_cast<std::uintptr_t>(p3) % 4 == 0);
    assert(reinterpret_cast<std::uintptr_t>(p4) % 8 == 0);

    // ---------------------------------------------------------
    // Invalid size
    // ---------------------------------------------------------

    assert(allocator.allocate(0, 8) == nullptr);

    // ---------------------------------------------------------
    // Invalid alignment (not power of two)
    // ---------------------------------------------------------

    assert(allocator.allocate(8, 3) == nullptr);

    // ---------------------------------------------------------
    // Invalid alignment (greater than arena alignment)
    // ---------------------------------------------------------

    assert(allocator.allocate(8, 32) == nullptr);

    // ---------------------------------------------------------
    // Exhaust remaining capacity
    // ---------------------------------------------------------

    while (allocator.allocate(1, 1) != nullptr) {
    }

    std::cout << "\nArena exhausted:\n";
    std::cout << "Used: " << allocator.used() << '\n';
    std::cout << "Remaining: " << allocator.remaining() << '\n';

    // Allocation must now fail.

    assert(allocator.allocate(1, 1) == nullptr);

    // ---------------------------------------------------------
    // Reset
    // ---------------------------------------------------------

    allocator.reset();

    assert(allocator.used() == 0);
    assert(allocator.remaining() == allocator.capacity());

    std::cout << "\nAfter reset:\n";
    std::cout << "Used: " << allocator.used() << '\n';
    std::cout << "Remaining: " << allocator.remaining() << '\n';

    // Arena can be reused.

    void* p5 = allocator.allocate(16, 16);

    assert(p5 != nullptr);
    assert(reinterpret_cast<std::uintptr_t>(p5) % 16 == 0);

    std::cout << "\nAllocator successfully reused after reset.\n";

    // ---------------------------------------------------------
    // Empty arena
    // ---------------------------------------------------------

    BumpAllocator empty{0};

    assert(empty.capacity() == 0);
    assert(empty.allocate(1, 1) == nullptr);

    std::cout << "\nEmpty arena behaves correctly.\n";

    std::cout << "\nAll tests passed.\n";
}

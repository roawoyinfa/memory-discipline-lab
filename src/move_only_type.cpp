#include <cassert>
#include <iostream>
#include <unordered_set>

class Reservation {
public:
    ~Reservation()
    {
        release();
    }

    Reservation(const Reservation&) = delete;
    Reservation& operator=(const Reservation&) = delete;

    Reservation(Reservation&& source) noexcept
        : reservation_id_{source.reservation_id_}
    {
        source.reservation_id_ = kInvalidId;
    }
    Reservation& operator=(Reservation&& source) noexcept
    {
        if (this == &source) { return *this; }

        release();

        reservation_id_ = source.reservation_id_;
        source.reservation_id_ = kInvalidId;

        return *this;
    }

    [[nodiscard]] int id() const noexcept
    {
        return reservation_id_;
    }
    [[nodiscard]] bool valid() const noexcept
    {
        return reservation_id_ != kInvalidId;
    }

    [[nodiscard]] static Reservation acquire()
    {
        const int id{next_id_++};
        active_reservations_.insert(id);

        return Reservation{id};
    }

    [[nodiscard]] static std::size_t active_count() noexcept
    {
        return active_reservations_.size();
    }

private:
    explicit Reservation(int reservation_id) noexcept
        : reservation_id_{reservation_id}
    {
    }

    void release()
    {
        if (!valid()) {
            return;
        }

        active_reservations_.erase(reservation_id_);
        reservation_id_ = kInvalidId;
    }

private:
    static constexpr int kInvalidId{-1};

    int reservation_id_{kInvalidId};

    // The static states are not thread-safe
    inline static int next_id_{0};
    inline static std::unordered_set<int> active_reservations_{};
};

int main()
{
    std::cout << "=== Move-only Type Demonstrating Stolen-from State ===\n";

    // ---------------------------------------------------------
    // Initial state
    // ---------------------------------------------------------

    assert(Reservation::active_count() == 0);

    std::cout << "Active reservations: "
              << Reservation::active_count() << '\n';

    // ---------------------------------------------------------
    // Resource acquisition
    // ---------------------------------------------------------

    auto alice{Reservation::acquire()};

    assert(alice.valid());
    assert(alice.id() >= 0);
    assert(Reservation::active_count() == 1);

    std::cout << "\nAlice acquires reservation #" << alice.id() << '\n';
    std::cout << "Active reservations: "
              << Reservation::active_count() << '\n';

    // ---------------------------------------------------------
    // Move construction
    // ---------------------------------------------------------

    const int reservation_id{alice.id()};

    auto bob{std::move(alice)};

    assert(!alice.valid());
    assert(alice.id() == -1);

    assert(bob.valid());
    assert(bob.id() == reservation_id);

    assert(Reservation::active_count() == 1);

    std::cout << "\nAfter move construction:\n";
    std::cout << "  Alice valid: " << std::boolalpha << alice.valid()
              << ", id = " << alice.id() << '\n';
    std::cout << "  Bob valid:   " << bob.valid()
              << ", id = " << bob.id() << '\n';
    std::cout << "  Active reservations: "
              << Reservation::active_count() << '\n';

    // ---------------------------------------------------------
    // Move assignment
    // ---------------------------------------------------------

    auto charlie{Reservation::acquire()};

    const int charlie_original_id{charlie.id()};

    assert(Reservation::active_count() == 2);

    std::cout << "\nCharlie acquires reservation #"
              << charlie_original_id << '\n';

    charlie = std::move(bob);

    assert(!bob.valid());
    assert(bob.id() == -1);

    assert(charlie.valid());
    assert(charlie.id() == reservation_id);

    assert(Reservation::active_count() == 1);

    std::cout << "\nAfter move assignment:\n";
    std::cout << "  Bob valid:      " << bob.valid()
              << ", id = " << bob.id() << '\n';
    std::cout << "  Charlie valid:  " << charlie.valid()
              << ", id = " << charlie.id() << '\n';
    std::cout << "  Charlie's previous reservation (#"
              << charlie_original_id
              << ") was released.\n";
    std::cout << "  Active reservations: "
              << Reservation::active_count() << '\n';

    // ---------------------------------------------------------
    // Automatic cleanup
    // ---------------------------------------------------------

    std::cout << "\nAutomatic cleanup:\n";
    std::cout << "  Active reservations before scope: "
              << Reservation::active_count() << '\n';

    {
        auto temporary{Reservation::acquire()};

        assert(temporary.valid());
        assert(Reservation::active_count() == 2);

        std::cout << "  Temporary reservation #"
                  << temporary.id() << '\n';
        std::cout << "  Active reservations inside scope: "
                  << Reservation::active_count() << '\n';
    }

    assert(Reservation::active_count() == 1);

    std::cout << "  Active reservations after scope: "
              << Reservation::active_count() << '\n';

    // ---------------------------------------------------------
    // Final cleanup
    // ---------------------------------------------------------

    std::cout << "\nAll tests passed.\n";
}

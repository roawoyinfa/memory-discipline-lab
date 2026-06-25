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
    std::cout << "=== Move-only type demonstrating stolen-from state ===\n";

    // ---------------------------------------------------------
    // Move construction
    // ---------------------------------------------------------

    auto alice{Reservation::acquire()};

    std::cout << "Alice acquires reservation #"
              << alice.id() << '\n';

    auto bob{std::move(alice)};

    std::cout << "\nAfter move construction:\n";

    std::cout << "  Alice valid: "
              << std::boolalpha
              << alice.valid()
              << ", id = " << alice.id() << '\n';

    std::cout << "  Bob valid: "
              << bob.valid()
              << ", id = " << bob.id() << "\n\n";

    // ---------------------------------------------------------
    // Move assignment
    // ---------------------------------------------------------

    auto charlie{Reservation::acquire()};

    std::cout << "Charlie acquires reservation #"
              << charlie.id() << '\n';

    const int charlie_original_id{charlie.id()};

    std::cout << "Active reservations before assignment: "
              << Reservation::active_count() << '\n';

    charlie = std::move(bob);

    std::cout << "\nAfter move assignment:\n";

    std::cout << "  Bob valid: "
              << bob.valid()
              << ", id = " << bob.id() << '\n';

    std::cout << "  Charlie valid: "
              << charlie.valid()
              << ", id = " << charlie.id() << '\n';

    std::cout << "\nCharlie's previous reservation (#"
              << charlie_original_id
              << ") was released before ownership transfer.\n";

    std::cout << "Active reservations after assignment: "
              << Reservation::active_count() << "\n\n";

    // ---------------------------------------------------------
    // Destructor cleanup
    // ---------------------------------------------------------

    std::cout << "Demonstrating automatic release on destruction\n";

    std::cout << "Active reservations before scope: "
              << Reservation::active_count() << '\n';

    {
        auto temporary{Reservation::acquire()};

        std::cout << "  Acquired temporary reservation #"
                  << temporary.id() << '\n';

        std::cout << "  Active reservations inside scope: "
                  << Reservation::active_count() << '\n';
    }

    std::cout << "Active reservations after scope: "
              << Reservation::active_count() << '\n';

    std::cout << "\nThe temporary reservation was automatically "
                 "released when its destructor ran.\n";
}

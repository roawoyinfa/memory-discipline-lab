#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

class UniqueFd {
public:
    UniqueFd() noexcept = default;

    explicit UniqueFd(int fd) noexcept
        : fd_{fd}
    {
    }

    ~UniqueFd()
    {
        if (fd_ < 0) {
            return;
        }

        ::close(fd_);
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& source) noexcept
        : fd_{source.fd_}
    {
        source.fd_ = -1;
    }
    UniqueFd& operator=(UniqueFd&& source) noexcept
    {
        if (this == &source) {
            return *this;
        }

        if (fd_ >= 0) {
            ::close(fd_);
        }

        fd_ = source.fd_;
        source.fd_ = -1;

        return *this;
    }


    [[nodiscard]] int get() const noexcept
    {
        return fd_;
    }

    [[nodiscard]] int release() noexcept
    {
        return std::exchange(fd_, -1);
    }

    void reset(int new_fd = -1) noexcept
    {
        if (fd_ >= 0 && fd_ != new_fd) {
            ::close(fd_);
        }

        fd_ = new_fd;
    }

    explicit operator bool() const noexcept
    {
        return fd_ >= 0;
    }

private:
    int fd_{-1};
};

UniqueFd open_file(const char* path)
{
    int fd = ::open(path, O_RDONLY);

    if (fd < 0) {
        throw std::runtime_error("open failed");
    }

    return UniqueFd{fd};
}

UniqueFd create_tcp_socket()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        throw std::runtime_error("socket failed");
    }

    return UniqueFd{fd};
}

int main()
{
    std::cout << "=== RAII Wrapper for file handle and socket fd ===\n";

    try {
        // ---------------------------------------------------------
        // Default construction
        // ---------------------------------------------------------

        UniqueFd empty;

        assert(!empty);
        assert(empty.get() == -1);

        std::cout << "\nDefault construction:\n";
        std::cout << "  fd = " << empty.get() << '\n';

        // ---------------------------------------------------------
        // Resource acquisition
        // ---------------------------------------------------------

        auto file{open_file("README.md")};

        assert(file);
        assert(file.get() >= 0);

        std::cout << "\nAfter acquiring a file:\n";
        std::cout << "  Wrapper owns fd #" << file.get() << '\n';

        // ---------------------------------------------------------
        // Release ownership
        // ---------------------------------------------------------

        int raw_fd{file.release()};

        assert(raw_fd >= 0);
        assert(!file);
        assert(file.get() == -1);

        std::cout << "\nAfter release():\n";
        std::cout << "  Raw fd:     " << raw_fd << '\n';
        std::cout << "  Wrapper fd: " << file.get() << '\n';

        // ---------------------------------------------------------
        // Reattach ownership
        // ---------------------------------------------------------

        file.reset(raw_fd);

        assert(file);
        assert(file.get() == raw_fd);

        std::cout << "\nAfter reset(fd):\n";
        std::cout << "  Wrapper owns fd #" << file.get() << '\n';

        // ---------------------------------------------------------
        // Move construction
        // ---------------------------------------------------------

        UniqueFd moved{std::move(file)};

        assert(!file);
        assert(file.get() == -1);

        assert(moved);
        assert(moved.get() == raw_fd);

        std::cout << "\nAfter move construction:\n";
        std::cout << "  Source fd:      " << file.get() << '\n';
        std::cout << "  Destination fd: " << moved.get() << '\n';

        // ---------------------------------------------------------
        // Move assignment
        // ---------------------------------------------------------

        UniqueFd assigned;

        assigned = std::move(moved);

        assert(!moved);
        assert(moved.get() == -1);

        assert(assigned);
        assert(assigned.get() == raw_fd);

        std::cout << "\nAfter move assignment:\n";
        std::cout << "  Source fd:      " << moved.get() << '\n';
        std::cout << "  Destination fd: " << assigned.get() << '\n';

        // ---------------------------------------------------------
        // Explicit resource release
        // ---------------------------------------------------------

        assigned.reset();

        assert(!assigned);
        assert(assigned.get() == -1);

        std::cout << "\nAfter reset():\n";
        std::cout << "  Resource explicitly released.\n";

        // ---------------------------------------------------------
        // Socket resource
        // ---------------------------------------------------------

        auto socket{create_tcp_socket()};

        assert(socket);
        assert(socket.get() >= 0);

        std::cout << "\nSocket acquired:\n";
        std::cout << "  Socket fd #" << socket.get() << '\n';

        socket.reset();

        assert(!socket);
        assert(socket.get() == -1);

        std::cout << "  Socket released.\n";

        // ---------------------------------------------------------
        // Automatic cleanup
        // ---------------------------------------------------------

        {
            auto scoped_file{open_file("README.md")};

            assert(scoped_file);

            std::cout << "\nAutomatic cleanup:\n";
            std::cout << "  File fd #" << scoped_file.get()
                      << " will be closed automatically at scope exit.\n";
        }

        std::cout << "  Scope exited successfully.\n";

        std::cout << "\nAll tests passed.\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}

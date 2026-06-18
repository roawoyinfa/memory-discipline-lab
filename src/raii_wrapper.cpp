#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <utility>
#include <sys/socket.h>

class UniqueFd {
public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_{fd} {}
    ~UniqueFd() {
        if (fd_ < 0) {
            return;
        }
        ::close(fd_);
    }

    UniqueFd(const UniqueFd& source) = delete;
    UniqueFd &operator=(const UniqueFd& source) = delete;

    UniqueFd(UniqueFd&& source) noexcept : fd_{source.fd_} {
        source.fd_ = -1;
    }
    UniqueFd &operator=(UniqueFd&& source) noexcept {
        if (this == &source) { return *this; }

        if (fd_ >= 0) {
            ::close(fd_);
        }

        fd_ = source.fd_;
        source.fd_ = -1;

        return *this;
    }


    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] int release() noexcept {
        return std::exchange(fd_, -1);
    }
    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0 && fd_ != new_fd) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

    explicit operator bool() const noexcept { return fd_ >= 0; }

private:
    int fd_{-1};
};

UniqueFd open_file(const char* path) {
    int fd = ::open(path, O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("open failed");
    }

    return UniqueFd{fd};
}

UniqueFd create_tcp_socket() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket failed");
    }

    return UniqueFd{fd};
}

int main() {
    std::cout << "=== RAII Wrapper for file handle and socket fd ===\n";

    try {
        std::cout << "[1] Acquire file resource\n";

        auto file{open_file("README.md")};

        std::cout << "  Wrapper owns fd #" << file.get() << '\n';

        std::cout << "[2] Release ownership from wrapper\n";

        int raw_fd{file.release()};

        std::cout << "  Raw fd received: " << raw_fd << '\n';
        std::cout << "  Wrapper fd:      " << file.get() << "\n\n";

        std::cout << "  Ownership now belongs to the caller.\n\n";

        std::cout << "[3] Return ownership to wrapper\n";

        file.reset(raw_fd);

        std::cout << "  Wrapper fd:    " << file.get() << "\n\n";

        std::cout << "[4] Acquire socket resource\n";

        auto socket{create_tcp_socket()};

        std::cout << "  Wrapper owns socket fd #" << socket.get() << "\n\n";

        std::cout << "[5] Explicitly release socket resource\n";

        socket.reset();

        std::cout << "  Socket resource closed.\n\n";

        std::cout << "[6] Leaving scope\n";
        std::cout << "  Remaining valid wrappers clean up automatically.\n";
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

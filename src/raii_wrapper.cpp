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
    try {
        std::cout << "=== RAII wrapper for a file handle and a socket fd ===\n";

        auto file{open_file("README.md")};

        std::cout << "File fd: " << file.get() << '\n';

        auto raw_fd{file.release()};

        std::cout << "AFTER RELEASE\n";
        std::cout << "  Raw fd: " << raw_fd << '\n';
        std::cout << "  Wrapper fd: " << file.get() << '\n';

        file.reset(raw_fd);

        std::cout << "AFTER RESET\n";
        std::cout << "   Wrapper fd: " << file.get() << "\n";

        auto socket{create_tcp_socket()};

        std::cout << "Socket fd: " << socket.get() << '\n';

        socket.reset();
        std::cout << "Socket closed via reset()\n";
    } catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }

    return 0;
}

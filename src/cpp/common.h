// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_COMMON_H_INCLUDED
#define HEADER_SPTEX_COMMON_H_INCLUDED


#if SPTEX_HAS_DLFCN

    struct dl_closer {
        void operator()(void * handle) const {
            if (handle)
                dlclose(handle);
        }
    };

    using dl_ptr = std::unique_ptr<void, dl_closer>;

#endif

struct Empty {};
struct Failed {};

template<class... Ts>
struct Overloads : Ts... { using Ts::operator()...; };

template<class T>
constexpr bool dependentFalse = false;

struct FreeDeleter {
	void operator()(void * ptr) { free(ptr); }
};

template<class T>
requires(
	(!std::is_array_v<T> && std::is_trivially_destructible_v<T>) ||
	(std::is_array_v<T> && std::is_trivially_destructible_v<std::remove_all_extents_t<T>>)
)
using unqiue_malloc_membuf = std::unique_ptr<T, FreeDeleter>;

#ifndef _WIN32

class FileDescriptor {
public:
    FileDescriptor() noexcept = default;
    explicit FileDescriptor(int fd) noexcept : m_fd(fd) {
    }
    ~FileDescriptor() noexcept {
        if (m_fd >= 0)
            ::close(m_fd);
    }
    FileDescriptor(FileDescriptor && src) noexcept : m_fd(src.m_fd) {
        src.m_fd = -1;
    }
    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor & operator=(FileDescriptor src) noexcept {
        swap(src, *this);
        return *this;
    }
    
    void close() noexcept {
        *this = FileDescriptor();
    }
    
    static auto open(const char * path, int oflag, mode_t mode, std::error_code & err) -> FileDescriptor {

        auto fd = ::open(path, oflag, mode);
        if (fd < 0) {
            fd = -1;
            err = std::error_code(errno, std::system_category());
        } else {
            err.clear();
        }
        return FileDescriptor(fd);
    }

    static auto open(const char * path, int oflag, mode_t mode) -> FileDescriptor {

        auto fd = ::open(path, oflag, mode);
        if (fd < 0) {
            fd = -1;
            throw std::system_error(std::error_code(errno, std::system_category()));
        }
        return FileDescriptor(fd);
    }

    friend void swap(FileDescriptor & lhs, FileDescriptor & rhs) noexcept {
        std::swap(lhs.m_fd, rhs.m_fd);
    }
    
    auto get() const noexcept -> int {
        return m_fd;
    }
    
    auto detach() noexcept -> int {
        int ret = m_fd;
        m_fd = -1;
        return ret;
    }
    
    explicit operator bool() const noexcept {
        return m_fd != -1;
    }
private:
    int m_fd = -1;
};

inline auto readFile(const FileDescriptor & desc, void * buf, io_size_t nbyte, 
                     std::error_code & err) -> io_ssize_t {
    auto fd = desc.get();
    auto ret = ::read(fd, buf, nbyte);
    if (ret < 0)
        err = std::error_code(errno, std::system_category());
    else
        err.clear();
    return ret;
}

inline auto readFile(const FileDescriptor & desc, void * buf, io_size_t nbyte) -> io_ssize_t {
    auto fd = desc.get();
    auto ret = ::read(fd, buf, nbyte);
    if (ret < 0)
        throw std::system_error(std::error_code(errno, std::system_category()));
    return ret;
}

#endif

#endif

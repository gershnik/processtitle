// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "common.h"
#include "logger.h"

template<class T>
static inline void appendFormat(std::string & dest) {
    if constexpr (!std::is_void_v<T>) {
        if constexpr (sizeof(T) == sizeof(unsigned long long)) {
            dest += "%llu";
        } else if constexpr (sizeof(T) == sizeof(unsigned long)) {
            dest += "%lu";
        } else if constexpr (sizeof(T) == sizeof(unsigned int)) {
            dest += "%u";
        } else if constexpr (sizeof(T) == sizeof(unsigned short)) {
            dest += "%hu";
        } else {
            static_assert(dependentFalse<T>);
        }
    } else {
        dest += "%*u";
    }
}


static inline char * skipField(char * buf) {
    char * buf_ptr = buf;
    char c = *buf_ptr;
    if (!c)
        return nullptr;
    if (c == ' ')
        return buf_ptr + 1;
    
    for (++buf_ptr ; ; ++buf_ptr) {
        char c = *buf_ptr;
        if (!c)
            return buf_ptr;
        if (c == ' ')
            return buf_ptr + 1;
    }
}

static inline char * skipFields(char * buf, int count) {
    char * buf_ptr = buf;
    for (int i = 0; i < count; ++i) {
        buf_ptr = skipField(buf_ptr);
        if (!buf_ptr)
            return nullptr;
    }
    return buf_ptr;
}

static inline char * skipDelimitedField(char * buf, char start, char end) {
    if (!*buf)
        return nullptr;
    if (*buf != start)
        return skipFields(buf, 1);

    char * buf_ptr = strchr(buf + 1, end);
    if (!buf_ptr)
        return nullptr;
    ++buf_ptr;
    if (*buf_ptr) {
        if (*buf_ptr != ' ')
            return nullptr;
        ++buf_ptr;
    }
    return buf_ptr;
}

static std::string readAll(const FileDescriptor & fd) {
    std::string line;
    line.resize(32);
    size_t offset = 0;
    for ( ; ; ) {
        std::error_code err;
		ssize_t bytes_read = readFile(fd, line.data() + offset, line.size() - offset, err);
        if (err) {
            if (err.value() == EINTR)
                continue;
            throw std::system_error(err);
        }
        if (bytes_read == 0) {
            line.resize(offset);
            return line;
        }
        offset += bytes_read;
        if (offset == line.size()) {
            line.resize(line.size() + 32);
        }
	}
}

void linuxPrepare() {

}

static bool useSetMm(const char * title) {

    /* sanity check that our struct matches kernel's */
    unsigned int struct_size;
    int res = prctl(PR_SET_MM, (unsigned long)(PR_SET_MM_MAP_SIZE), &struct_size,
                    (unsigned long)0, (unsigned long)0);
    if (res != 0 || struct_size != sizeof(prctl_mm_map)) {
        logDebug("unexpected PR_SET_MM size");
        return false;
    }
    
    std::error_code err;
    auto fd = FileDescriptor::open("/proc/self/stat", O_RDONLY | O_CLOEXEC, 0, err);
    if (err) {
        logDebug(std::string("cannot open /proc/self/stat: ") + err.message());
        return false;
    }
    
    std::string buf = readAll(fd);
    
    prctl_mm_map prctl_map{};

    /* The column layout is:
       25 columns to ignore, second is in ()
       start_code
       end_code
       start_stack 
       19 columns to ignore
       start_data
       end_data
       start_brk
       2 columns to ignore
       env_start
       env_end
    */
    auto buf_ptr = skipFields(buf.data(), 1);
    if (!buf_ptr) {
        logDebug("unexpected /proc/self/stat format: less than 1 column");
        return false;
    }
    buf_ptr = skipDelimitedField(buf_ptr, '(', ')');
    if (!buf_ptr) {
        logDebug("unexpected /proc/self/stat format: missing comm column");
        return false;
    }

    buf_ptr = skipFields(buf_ptr, 23);
    if (!buf_ptr) {
        logDebug("unexpected /proc/self/stat format: missing 23 columns after comm");
        return false;
    }

    std::string format;

    appendFormat<decltype(prctl_map.start_code)>(format);
    appendFormat<decltype(prctl_map.end_code)>(format);
    appendFormat<decltype(prctl_map.start_stack)>(format);
    if (sscanf(buf_ptr, format.c_str(), 
               &prctl_map.start_code, &prctl_map.end_code, &prctl_map.start_stack) != 3) {

        logDebug("unexpected /proc/self/stat format: start_code - start_stack missing or invalid");
        return false;
    }

    buf_ptr = skipFields(buf_ptr, 19);
    if (!buf_ptr) {
        logDebug("unexpected /proc/self/stat format: missing 19 columns after start_stack");
        return false;
    }

    format.clear();
    appendFormat<decltype(prctl_map.start_data)>(format);
    appendFormat<decltype(prctl_map.end_data)>(format);
    appendFormat<decltype(prctl_map.start_brk)>(format);
    appendFormat<void>(format);
    appendFormat<void>(format);
    appendFormat<decltype(prctl_map.env_start)>(format);
    appendFormat<decltype(prctl_map.env_end)>(format);
    
    if (sscanf(buf_ptr, format.c_str(), 
               &prctl_map.start_data, &prctl_map.end_data, 
               &prctl_map.start_brk, &prctl_map.env_start, &prctl_map.env_end) != 5) {
        logDebug("unexpected /proc/self/stat format: start_data - env_end missing or invalid");
        return false;
    }

    size_t full_title_len = strlen(title) + 1;

    static char * proctitle = nullptr;

    std::unique_ptr<char> tmp_proctitle{(char *)malloc(full_title_len)};
    if (!tmp_proctitle) {
        logDebug("cannot allocate " + std::to_string(full_title_len) + " bytes");
        return false;
    }
    memcpy(tmp_proctitle.get(), title, full_title_len);

    
    prctl_map.arg_start = (decltype(prctl_map.arg_start))tmp_proctitle.get();
    prctl_map.arg_end = prctl_map.arg_start + full_title_len;

    prctl_map.brk = syscall(__NR_brk, 0);

    prctl_map.auxv = NULL;
    prctl_map.auxv_size = 0;
    prctl_map.exe_fd = -1;

    if (prctl(PR_SET_MM, (unsigned long)(PR_SET_MM_MAP), 
              &prctl_map, sizeof(prctl_map), (unsigned long)0) != 0) {

        auto err = std::error_code(errno, std::system_category());
        logDebug("PR_SET_MM failed: " + err.message());
        return false;
    }

    //noexcept from this point

    if (proctitle)
        free(proctitle);
    proctitle = tmp_proctitle.release();

    return true;
}

static bool usePrSetName(const char * title) {
    if (prctl(PR_SET_NAME, title) < 0) {
        auto err = std::error_code(errno, std::system_category());
        logDebug(std::string("PR_SET_NAME failed: ") + err.message());
        return false;
    }

    return true;
}

bool linuxSetProcessTitle(const char * title) {

    try {
        bool ret = false;
        if (useSetMm(title))
            ret = true;

        if (usePrSetName(title))
            ret = true;

        return ret;
        
    } catch(std::exception & ex) {
        logDebug(std::string("unhandled exception in linuxSetProcessTitle: ") + ex.what());
    }

    return false;
}
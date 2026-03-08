// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_SPTEX_PCH_H_INCLUDED
#define HEADER_SPTEX_PCH_H_INCLUDED

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif

#include <Python.h>

#if __has_include(<dlfcn.h>)
    #define SPTEX_HAS_DLFCN 1
    #include <dlfcn.h>
#endif

#if __has_include(<sys/types.h>)
    #include <sys/types.h>
#endif

#if __has_include(<unistd.h>)
    #include <unistd.h>
#endif

#if __has_include(<fcntl.h>)
    #include <fcntl.h>
#endif


#include <stdlib.h>

#include <intrusive_shared_ptr/python_ptr.h>

#include <pylib/method.h>
#include <pylib/locking.h>

#if defined(__APPLE__) && defined(__MACH__)

    #include <CoreFoundation/CoreFoundation.h>
    #include <crt_externs.h>

    #include <intrusive_shared_ptr/apple_cf_ptr.h>

#elif defined(__linux__)

    #include <sys/prctl.h>
    #include <sys/syscall.h>

    #define SPTEX_HAVE_CLEARENV 1

#elif defined(__sun__)

    #include <procfs.h>
    #include <sys/procfs.h>

    #define SPTEX_HAVE_CLEARENV 1

#elif defined(_WIN32)

    #include <windows.h>
    #include <winternl.h>

#endif


#include <memory>
#include <optional>
#include <mutex>
#include <tuple>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <variant>

#if defined(_WIN32) && !defined(__MINGW32__)
    using mode_t = int;
    using io_size_t = unsigned;
    using io_ssize_t = int;
#else
    using io_size_t = ::size_t;
    using io_ssize_t = ::ssize_t;
#endif

using isptr::py_ptr;
using isptr::py_attach;
using isptr::py_retain;

#if defined(__APPLE__) && defined(__MACH__)

using isptr::cf_ptr;
using isptr::cf_attach;
using isptr::cf_retain;

#endif

#endif

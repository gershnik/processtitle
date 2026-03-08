// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_LOCKING_H_INCLUDED
#define HEADER_PYLIB_LOCKING_H_INCLUDED

#include <mutex>


class PythonCriticalSection {
public:
    PythonCriticalSection([[maybe_unused]] const isptr::py_ptr<PyObject> & obj) noexcept
#if Py_GIL_DISABLED
        : m_obj(obj)
#endif
    {}

    PythonCriticalSection([[maybe_unused]] isptr::py_ptr<PyObject> && obj) noexcept
#if Py_GIL_DISABLED
        : m_obj(std::move(obj))
#endif
    {}

    PythonCriticalSection([[maybe_unused]] PyObject * obj) noexcept
#if Py_GIL_DISABLED
        : m_obj(isptr::py_retain(obj))
#endif
    {}
    
    PythonCriticalSection(const PythonCriticalSection &) = delete;
    PythonCriticalSection & operator=(const PythonCriticalSection &) = delete;

    void lock() noexcept {
#if Py_GIL_DISABLED
        PyCriticalSection_Begin(&m_cs, m_obj.get());
#endif
    }

    void unlock() noexcept {
#if Py_GIL_DISABLED
        PyCriticalSection_End(&m_cs);
#endif
    }
private:
#if Py_GIL_DISABLED
    PyCriticalSection m_cs{};
    isptr::py_ptr<PyObject> m_obj;
#endif
};

template<size_t N>
requires(N > 0)
class PythonLock {
private:
    template<std::size_t... Is>
    void lockArray(std::index_sequence<Is...>) {
        std::lock(m_cs[Is]...);
    }
public:
    template<class... Arg>
    requires(sizeof...(Arg) == N && (std::is_convertible_v<Arg &&, PythonCriticalSection> && ...))
    PythonLock(Arg && ...obj) noexcept:
        m_cs(std::forward<Arg>(obj)...) {
        lockArray(std::make_index_sequence<N>{});
    }
    ~PythonLock() noexcept {
        for(size_t i = 0; i < N; ++i)
            m_cs[i].unlock();
    }
    PythonLock(const PythonLock &) = delete;
    PythonLock & operator=(const PythonLock &) = delete;

private:
    PythonCriticalSection m_cs[N];
};

template<>
class PythonLock<1> {
public:
    template<class Arg>
    requires(std::is_convertible_v<Arg &&, PythonCriticalSection>)
    PythonLock(Arg && obj) noexcept:
        m_cs(std::forward<Arg>(obj)) {
        m_cs.lock();
    }
    ~PythonLock() noexcept {
        m_cs.unlock();
    }
    PythonLock(const PythonLock &) = delete;
    PythonLock & operator=(const PythonLock &) = delete;

private:
    [[no_unique_address]] PythonCriticalSection m_cs;
};

template<class... Arg>
PythonLock(Arg && ...obj) -> PythonLock<sizeof...(Arg)>;
    

#endif
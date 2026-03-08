// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_ERRORS_H_INCLUDED
#define HEADER_PYLIB_ERRORS_H_INCLUDED

class PassthroughException : public std::exception {};

#define PYLIB_EXTERNAL_PROLOG try {

#define PYLIB_EXTERNAL_EPILOG_RET(val) } catch (std::bad_alloc &) { \
    PyErr_SetNone(PyExc_MemoryError); \
    return val; \
} catch(PassthroughException &) { \
    return val; \
} catch (std::exception & ex) { \
    PyErr_SetString(PyExc_Exception, ex.what());\
    return val; \
}

#define PYLIB_EXTERNAL_EPILOG PYLIB_EXTERNAL_EPILOG_RET(nullptr)


template<class T>
inline T checkPython(T obj) {
    if (!obj)
        throw PassthroughException{};
    return obj;
}

#endif
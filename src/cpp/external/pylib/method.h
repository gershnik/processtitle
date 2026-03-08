// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_METHOD_H_INCLUDED
#define HEADER_PYLIB_METHOD_H_INCLUDED

#include "errors.h"
#include "arguments.h"

template<class Derived>
struct PythonMethod {
    static constexpr const char * name() { return Derived::descriptor().func; }

    static PyObject * fastCall(PyObject * self, PyObject * const * args, Py_ssize_t nargs, PyObject * kwnames) noexcept {
        PYLIB_EXTERNAL_PROLOG
            auto parsedArgs = parseFastCallArguments(args, nargs, kwnames, Derived::descriptor());
            if (!parsedArgs)
                return nullptr;

            return std::apply(Derived{}, std::tuple_cat(std::tuple(self), *parsedArgs));

        PYLIB_EXTERNAL_EPILOG
    }

    static PyObject * varargCall(PyObject * self, PyObject * args, PyObject * kwargs) noexcept {
        PYLIB_EXTERNAL_PROLOG
            auto parsedArgs = parseVarArgArguments(args, kwargs, Derived::descriptor());
            if (!parsedArgs)
                return nullptr;

            return std::apply(Derived{}, std::tuple_cat(std::tuple(self), *parsedArgs));

        PYLIB_EXTERNAL_EPILOG
    }

    static PyObject * noargCall(PyObject * self, PyObject * /*ignored*/) noexcept {
        PYLIB_EXTERNAL_PROLOG
            return Derived{}(self);
        PYLIB_EXTERNAL_EPILOG
    }
};

#endif
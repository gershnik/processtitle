// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_MARSHALLING_H_INCLUDED
#define HEADER_PYLIB_MARSHALLING_H_INCLUDED

#include <intrusive_shared_ptr/python_ptr.h>

#include <optional>
#include <string>

#include "util.h"

//Make call to toPython dependent
template<class T> auto callToPython(T && arg) -> isptr::py_ptr<PyObject>;

template<class T>
struct KnownType {
    PyObject * ptr = nullptr;

    explicit operator bool() const noexcept 
        { return ptr != nullptr; }

    static auto check(PyObject * ptr) -> KnownType;
    static auto badTypeMessage(const std::string_view & name) -> std::string;
};

template<class T>
struct NameProvider {
    const char * name;
};

template<class T> struct MakeFromPythonResult { using Type = std::optional<T>; };
template<class T> struct MakeFromPythonResult<isptr::py_ptr<T>> { using Type = isptr::py_ptr<T>; };

template<class T> using FromPythonResult = typename MakeFromPythonResult<T>::Type;

template<class T> auto IsOptionalHelper(std::optional<T> * t) -> std::true_type;
auto IsOptionalHelper(...) -> std::false_type;
template<class T> constexpr bool IsOptional = decltype(IsOptionalHelper((T*)nullptr))::value;


template<class T>
auto fromPython(PyObject * obj, NameProvider<T> nameProvider) -> FromPythonResult<T> {
    if (auto known = KnownType<T>::check(obj)) 
        return fromPython(known, nameProvider);

    PyErr_SetString(PyExc_TypeError, KnownType<T>::badTypeMessage(nameProvider.name).c_str());
    return FromPythonResult<T>{};
}


//PyObject

inline auto toPython(const isptr::py_ptr<PyObject> & obj) -> isptr::py_ptr<PyObject> {
    return obj;
}
inline auto toPython(isptr::py_ptr<PyObject> && obj) -> isptr::py_ptr<PyObject> {
    return std::move(obj);
}

template<> 
inline auto KnownType<isptr::py_ptr<PyObject>>::check(PyObject * obj) -> KnownType {
    return KnownType{obj};
}

template<> 
inline auto KnownType<isptr::py_ptr<PyObject>>::badTypeMessage(const std::string_view &) -> std::string {
    Py_FatalError("logic error"); //this can never be called
    abort();
}

inline auto fromPython(KnownType<isptr::py_ptr<PyObject>> obj, NameProvider<isptr::py_ptr<PyObject>>) -> isptr::py_ptr<PyObject> {
    return isptr::py_retain(obj.ptr);
}

//PyTypeObject

template<> 
inline auto KnownType<isptr::py_ptr<PyTypeObject>>::check(PyObject * obj) -> KnownType {
    return KnownType{PyType_Check(obj) ? obj : nullptr};
}

template<> 
inline auto KnownType<isptr::py_ptr<PyTypeObject>>::badTypeMessage(const std::string_view & name) -> std::string {
    return concat(name, " must be a type");
}

inline auto fromPython(KnownType<isptr::py_ptr<PyTypeObject>> obj, NameProvider<isptr::py_ptr<PyTypeObject>>) -> isptr::py_ptr<PyTypeObject> {
    return isptr::py_retain((PyTypeObject *)obj.ptr);
}

// std::optional

template<class T>
auto toPython(const std::optional<T> & obj) -> isptr::py_ptr<PyObject> {
    if (obj)
        return callToPython(*obj);
    return isptr::py_retain(Py_None);
}

template<class T>
auto toPython(std::optional<T> && obj) -> isptr::py_ptr<PyObject> {
    if (obj)
        return callToPython(std::move(*obj));
    return isptr::py_retain(Py_None);
}

//Tuple

template<size_t N>
auto makePythonTuple(isptr::py_ptr<PyObject> (&& array)[N]) -> isptr::py_ptr<PyObject> {
    auto res = isptr::py_attach(PyTuple_New(N));
    if (!res)
        return nullptr;
    for(size_t i = 0; i < N; ++i)
        PyTuple_SET_ITEM(res.get(), i, array[i].release());
    return res;
}

template<size_t I, class... Args>
auto tupleToPythonImpl(const std::tuple<Args...> & obj, isptr::py_ptr<PyObject> (&& itemsArray)[sizeof...(Args)]) -> isptr::py_ptr<PyObject> {

    constexpr size_t Length = sizeof...(Args);
    
    itemsArray[I] = callToPython(std::get<I>(obj));
    if (!itemsArray[I])
        return nullptr;

    if constexpr (I < Length - 1) {
        return tupleToPythonImpl<I + 1>(obj, std::move(itemsArray));
    } else {
        return makePythonTuple(std::move(itemsArray));
    }
}

template<size_t I, class... Args>
auto tupleToPythonImpl(std::tuple<Args...> && obj, isptr::py_ptr<PyObject> (&& itemsArray)[sizeof...(Args)]) -> isptr::py_ptr<PyObject> {

    constexpr size_t Length = sizeof...(Args);
    
    itemsArray[I] = callToPython(std::move(std::get<I>(obj)));
    if (!itemsArray[I])
        return nullptr;

    if constexpr (I < Length - 1) {
        return tupleToPythonImpl<I + 1>(std::move(obj), std::move(itemsArray));
    } else {
        return makePythonTuple(std::move(itemsArray));
    }
}

template<class... Args>
auto toPython(const std::tuple<Args...> & obj) -> isptr::py_ptr<PyObject> {
    if constexpr (sizeof...(Args) > 0) {
        isptr::py_ptr<PyObject> items[sizeof...(Args)];
        return tupleToPythonImpl<0>(obj, std::move(items));
    } else {
        return isptr::py_attach(PyTuple_New(0));
    }
}

template<class... Args>
auto toPython(std::tuple<Args...> && obj) -> isptr::py_ptr<PyObject> {
    if constexpr (sizeof...(Args) > 0) {
        isptr::py_ptr<PyObject> items[sizeof...(Args)];
        return tupleToPythonImpl<0>(std::move(obj), std::move(items));
    } else {
        return isptr::py_attach(PyTuple_New(0));
    }
}

template<class... Args>
struct NameProvider<std::tuple<Args...>> {

    template<size_t I>
    auto itemNameProvider() const {
        return std::get<I>(this->items);
    }

    const char * name;
    std::tuple<NameProvider<Args>...> items;
};

template<class... Args>
struct KnownType<std::tuple<Args...>> {
    PyObject * ptr;

    explicit operator bool() const noexcept 
        { return ptr != nullptr; }
    static auto check(PyObject * ptr) -> KnownType 
        { return KnownType{PyTuple_Check(ptr) ? ptr : nullptr}; }
    static auto badTypeMessage(const std::string & name) -> std::string 
        { return name + " must be a tuple"; }
};

template<size_t I, class... Args>
auto tupleFromPythonImpl(PyObject * tuple, NameProvider<std::tuple<Args...>> nameProvider, 
                         std::tuple<FromPythonResult<Args>...> & items) -> bool {

    constexpr size_t Length = sizeof...(Args);

    auto elem = PyTuple_GET_ITEM(tuple, I);
    auto & item = std::get<I>(items);
    using ExtractType = std::remove_cvref_t<decltype(*item)>;
    item = ::fromPython<ExtractType>(elem, nameProvider.template itemNameProvider<I>());
    if (!item)
        return false;

    if constexpr (I < Length - 1) {
        return tupleFromPythonImpl<I + 1>(tuple, nameProvider, items);
    } else {
        return true;
    }
    
}

template<class... Args>
inline auto fromPython(KnownType<std::tuple<Args...>> tuple, NameProvider<std::tuple<Args...>> nameProvider) -> FromPythonResult<std::tuple<Args...>> {

    constexpr size_t Length = sizeof...(Args);

    if (PyTuple_GET_SIZE(tuple.ptr) != Length) {
        PyErr_SetString(PyExc_TypeError, concat(nameProvider.name, " must have ", std::to_string(Length), " elements").c_str());
        return std::nullopt;
    }

    std::tuple<FromPythonResult<Args>...> items;
    if (!tupleFromPythonImpl<0>(tuple.ptr, nameProvider, items))
        return std::nullopt;
    return tupleTransform<std::tuple<Args...>>(std::move(items), [](auto && arg) {
        using ArgType = std::remove_cvref_t<decltype(arg)>;
        if constexpr (IsOptional<ArgType>)
            return std::move(*arg);
        else
            return std::move(arg);
            
    });
}


//bool 

inline auto toPython(bool val) -> isptr::py_ptr<PyObject> {
    return isptr::py_retain(val ? Py_True : Py_False);
}

template<> 
inline auto KnownType<bool>::check(PyObject * obj) -> KnownType {
    return KnownType{PyBool_Check(obj) ? obj : nullptr};
}

template<>
inline auto KnownType<bool>::badTypeMessage(const std::string_view & name) -> std::string {
    return concat(name, " must be a bool");
}

inline auto fromPython(KnownType<bool> obj, NameProvider<bool>) -> FromPythonResult<bool> {
    return obj.ptr == Py_True;
}


//unsigned integers

template<class T>
concept PythonicUnsigned =  std::is_integral_v<T> && 
                            !std::is_signed_v<T> && 
                            !std::is_same_v<T, bool> && 
                            sizeof(T) <= sizeof(unsigned long long);

template<PythonicUnsigned T>
inline auto toPython(T val) -> isptr::py_ptr<PyObject> {
    if constexpr (sizeof(T) <= sizeof(unsigned long))
        return isptr::py_attach(PyLong_FromUnsignedLong(val));
    else 
        return isptr::py_attach(PyLong_FromUnsignedLongLong(val));
}

template<PythonicUnsigned T>
struct KnownType<T> {
    PyObject * ptr;

    explicit operator bool() const noexcept 
        { return ptr != nullptr; }
    static auto check(PyObject * ptr) -> KnownType 
        { return KnownType{PyLong_Check(ptr) ? ptr : nullptr}; }
    static auto badTypeMessage(const std::string_view & name) -> std::string
        { return concat(name, " must be an integer"); }
};


template<PythonicUnsigned T>
inline auto fromPython(KnownType<T> obj, NameProvider<T> nameProvider) -> FromPythonResult<T> {
    
    using MarshalType = std::conditional_t<sizeof(T) <= sizeof(unsigned long), unsigned long, unsigned long long>;

    MarshalType ret;
    if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        ret = PyLong_AsUnsignedLong(obj.ptr);
    } else {
        ret = PyLong_AsUnsignedLong(obj.ptr);
    }
    if (PyErr_Occurred()) 
        return FromPythonResult<T>{};
    if constexpr (sizeof(T) < sizeof(MarshalType)) {
        if (ret > std::numeric_limits<T>::max()) {
            PyErr_SetString(PyExc_OverflowError, concat(nameProvider.name, " is out of range").c_str());
            return FromPythonResult<T>{};
        }
    }
    return ret;
    
}

//double

template<>
struct KnownType<double> {
    PyObject * ptr;
    bool isInteger;

    explicit operator bool() const noexcept 
        { return ptr != nullptr; }
    static auto check(PyObject * ptr) -> KnownType { 
        if (PyLong_Check(ptr))
            return KnownType{ptr, true};
        else if (PyFloat_Check(ptr))
            return KnownType{ptr, false};
        return KnownType{nullptr, true}; 
    }
    static auto badTypeMessage(const std::string_view & name) -> std::string
        { return concat(name, " must be numeric"); }
};

inline auto fromPython(KnownType<double> obj, NameProvider<double>) -> FromPythonResult<double> {
    double ret;
    if (obj.isInteger)
        ret = PyLong_AsDouble(obj.ptr);
    else
        ret = PyFloat_AS_DOUBLE(obj.ptr);
    if (PyErr_Occurred()) 
        return FromPythonResult<double>{};
    return ret;
}

//String

inline auto toPython(std::string_view str) -> isptr::py_ptr<PyObject> {
    auto obj = PyUnicode_DecodeUTF8(str.data(), str.size(), "replace");
    return isptr::py_attach(obj);
}

inline auto toPython(const char * str) -> isptr::py_ptr<PyObject> {
    return toPython(std::string_view(str));
}

template<> 
inline auto KnownType<std::string>::check(PyObject * obj) -> KnownType {
    return KnownType{PyUnicode_Check(obj) ? obj : nullptr};
}

template<>
inline auto KnownType<std::string>::badTypeMessage(const std::string_view & name) -> std::string {
    return concat(name, " must be a string");
}

inline auto fromPython(KnownType<std::string> obj, NameProvider<std::string>) -> FromPythonResult<std::string> {
    auto ptr = PyUnicode_AsUTF8(obj.ptr);
    if (!ptr) 
        return FromPythonResult<std::string>{};
    return std::string{ptr};
}


//Epilog

template<class T> auto callToPython(T && arg)  -> isptr::py_ptr<PyObject> {
    return toPython(std::forward<T>(arg));
}

#endif

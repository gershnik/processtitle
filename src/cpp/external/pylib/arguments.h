// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_ARGUMENTS_H_INCLUDED
#define HEADER_PYLIB_ARGUMENTS_H_INCLUDED


#include "marshalling.h"
#include "util.h"

template<class T, bool Required>
struct ArgumentInfo {
    using Type = T;
    using ResultType = std::conditional_t<Required, T, std::optional<T>>;
    static constexpr bool required = Required;
    NameProvider<T> nameProvider;
};

template<class T>
using ArgumentTypeFromInfo = typename T::Type;

template<class T>
using ResultTypeFromInfo = typename T::ResultType;


struct MakeArgNameProvider {

    template<class T, bool Required>
    auto operator()(const ArgumentInfo<T, Required> & arg) {
        return arg.nameProvider;
    }
};


template<class... Ts, bool... Rs>
std::true_type IsArgumentInfoTupleImpl(std::tuple<ArgumentInfo<Ts, Rs>...>);
std::false_type IsArgumentInfoTupleImpl(...);

template<class T>
concept ArgumentInfoTuple = decltype(IsArgumentInfoTupleImpl(std::declval<T>()))::value;

template<class T>
concept ArgumentsDescriptor = requires {
    {T::func} -> std::convertible_to<const char *>;
    {T::end_of_positional_only} -> std::convertible_to<size_t>;
    {T::end_of_mixed} -> std::convertible_to<size_t>;
    {T::args} -> ArgumentInfoTuple;
};

template<ArgumentInfoTuple Req, ArgumentInfoTuple Opt, size_t EndOfPos, size_t EndOfMixed>
class ArgumentsDescriptorBuilder {
    template<ArgumentInfoTuple, ArgumentInfoTuple, size_t, size_t>
    friend class ArgumentsDescriptorBuilder;
public:
    constexpr ArgumentsDescriptorBuilder(const char * f) 
        requires(std::tuple_size_v<Req> == 0 &&  
                 std::tuple_size_v<Opt> == 0 && 
                 EndOfPos == size_t(-1) &&
                 EndOfMixed == size_t(-1))
        : m_func(f)
    {}

    template<class T>
    constexpr auto required(NameProvider<T> nameProvider) {
        static_assert(std::tuple_size_v<Opt> == 0, "you cannot add required arguments after optional");

        using Ret = ArgumentsDescriptorBuilder<JoinedTuple<Req, std::tuple<ArgumentInfo<T, true>>>, Opt, EndOfPos, EndOfMixed>;
        return Ret(m_func, std::tuple_cat(m_required, std::tuple{ArgumentInfo<T, true>{nameProvider}}), m_optional);
    }

    template<class T>
    constexpr auto optional(NameProvider<T> nameProvider) {
        using Ret = ArgumentsDescriptorBuilder<Req, JoinedTuple<Opt, std::tuple<ArgumentInfo<T, false>>>, EndOfPos, EndOfMixed>;
        return Ret(m_func, m_required, std::tuple_cat(m_optional, std::tuple{ArgumentInfo<T, false>{nameProvider}}));
    }

    constexpr auto slash() {
        static_assert(EndOfPos == size_t(-1), "slash can be called only once");
        static_assert(EndOfMixed == size_t(-1), "slash cannot be called after star");
        
        constexpr size_t newEndOfPos = std::tuple_size_v<Req> + std::tuple_size_v<Opt>;
        using Ret = ArgumentsDescriptorBuilder<Req, Opt, newEndOfPos, EndOfMixed>;
        return Ret(m_func, m_required, m_optional);
    }

    constexpr auto star() {
        static_assert(EndOfMixed == size_t(-1), "slash can be called only once");
        
        constexpr size_t newEndOfMixed = std::tuple_size_v<Req> + std::tuple_size_v<Opt>;
        using Ret = ArgumentsDescriptorBuilder<Req, Opt, EndOfPos, newEndOfMixed>;
        return Ret(m_func, m_required, m_optional);
    }

    struct Result {
        const char * func;
        static constexpr size_t end_of_positional_only = (EndOfPos == size_t(-1) ? 0 : EndOfPos);
        static constexpr size_t end_of_mixed = (EndOfMixed == size_t(-1) ? std::tuple_size_v<Req> + std::tuple_size_v<Opt>: EndOfMixed);
        [[no_unique_address]] JoinedTuple<Req, Opt> args;
    };

    constexpr auto build() const {
        Result res {
            m_func,
            std::tuple_cat(m_required, m_optional)
        };
        return res;
    }
private:
    constexpr ArgumentsDescriptorBuilder(const char * f, const Req & req, const Opt & opt):
        m_func(f),
        m_required(req),
        m_optional(opt)
    {}
private:
    const char * m_func;
    Req m_required;
    Opt m_optional;
};

ArgumentsDescriptorBuilder(const char *) -> ArgumentsDescriptorBuilder<std::tuple<>, std::tuple<>, size_t(-1), size_t(-1)>;


template<ArgumentsDescriptor T>
using Arguments = TransformedTuple<ResultTypeFromInfo, decltype(T::args)>;


template<ArgumentsDescriptor T>
struct ArgErrorMaker {

    static void badArgName(const T * desc, const std::string_view & name) {
        PyErr_SetString(PyExc_TypeError, concat(desc->func, " has no argument named '", name, "'").c_str());
    }

    static void argIsPosOnly(const T * desc, const std::string_view & name) {
        PyErr_SetString(PyExc_TypeError, concat(desc->func, "() argument '", name, "' is positional-only").c_str());
    }

    static void multipleArgValues(const T * desc, const std::string_view & name) {
        PyErr_SetString(PyExc_TypeError, concat(desc->func, "() got multiple values for argument '", name, "'").c_str());
    }

    static void missingRequired(const T * desc, const std::string_view & name) {
        PyErr_SetString(PyExc_TypeError, concat(desc->func, "() missing required positional argument: '", name, "'").c_str());
    }

    static void tooManyArgs(const T * desc) {
        if constexpr (T::end_of_mixed == 0)
            PyErr_SetString(PyExc_TypeError, concat(desc->func, "() has no positional arguments").c_str());
        else if constexpr (T::end_of_mixed == 1)
            PyErr_SetString(PyExc_TypeError, concat(desc->func, "() takes at most 1 positional argument").c_str());
        else
            PyErr_SetString(PyExc_TypeError, concat(desc->func, "() takes at most ", std::to_string(T::end_of_mixed), " positional arguments").c_str());
    }
};

template<ArgumentsDescriptor T>
struct ArgumentsFromPythonConverter {

private:
    using RawTypes = TransformedTuple<ArgumentTypeFromInfo, decltype(T::args)>;
    using TempType = TransformedTuple<FromPythonResult, TransformedTuple<ArgumentTypeFromInfo, decltype(T::args)>>;

    template<size_t Idx>
    struct Transformer {
        auto operator()(auto && arg) {
            using ArgType = std::remove_cvref_t<decltype(arg)>;
            using ExtractType = std::tuple_element_t<Idx, Arguments<T>>;
            if constexpr (!std::is_same_v<ArgType, ExtractType>)
                return std::move(*arg);
            else
                return std::move(arg);
        }
    };

    static auto makeNameProvider(const T & desc) {
        using Ret = TransformedTuple<NameProvider, TransformedTuple<ArgumentTypeFromInfo, decltype(T::args)>>;
        return tupleTransform<Ret>(desc.args, MakeArgNameProvider{});
    }

    using NameProvidersType = decltype(makeNameProvider(std::declval<T>()));

    template<size_t I>
    auto setKeyWordArg(PyObject * obj, const std::string_view & name, TempType & items) const -> bool {

        constexpr size_t Length = std::tuple_size_v<TempType>;

        if constexpr (I == Length) {
            ArgErrorMaker<T>::badArgName(m_desc, name);
            return false;
            
        } else {

            auto & prov = std::get<I>(m_nameProviders);
            if (name == prov.name) {
                if (I < T::end_of_positional_only) {
                    ArgErrorMaker<T>::argIsPosOnly(m_desc, name);
                    return false;
                }
                auto & item = std::get<I>(items);
                if (item) {
                    ArgErrorMaker<T>::multipleArgValues(m_desc, name);
                    return false;
                }
                using ExtractType = std::tuple_element_t<I, RawTypes>;
                item = ::fromPython<ExtractType>(obj, prov);
                if (!item)
                    return false;
                return true;
            }

            return this->setKeyWordArg<I + 1>(obj, name, items);
        }
    }

    auto parseVarArgKeywordImpl(PyObject * kwargs, TempType & items) const -> bool {

        if (!kwargs)
            return true;

        PyObject * key, * value;
        Py_ssize_t pos = 0;

        //probably unnecessary since the kwarg *shouldn't be* shared with any other threads
        //but we can't really know...
        //PythonLock lock(kwargs);
        while (PyDict_Next(kwargs, &pos, &key, &value)) {
            auto elemName = PyUnicode_AsUTF8(key);
            bool res = this->setKeyWordArg<0>(value, elemName, items);
            if (!res)
                return false;
        }
        return true;
    }

    template<size_t I>
    auto parseVarArgPosImpl(PyObject * args, size_t argsSize, PyObject * kwargs, TempType & items) const -> bool {

        if (I == argsSize) {
            return this->parseVarArgKeywordImpl(kwargs, items);
        }

        auto elem = PyTuple_GET_ITEM(args, I);
        auto & item = std::get<I>(items);
        using ExtractType = std::tuple_element_t<I, RawTypes>;
        item = ::fromPython<ExtractType>(elem, std::get<I>(m_nameProviders));
        if (!item)
            return false;

        if constexpr (I + 1 < T::end_of_mixed) {
            return this->parseVarArgPosImpl<I + 1>(args, argsSize, kwargs, items);
        } else {
            return this->parseVarArgKeywordImpl(kwargs, items);
        }
    }

    auto parseFastCallKeywordImpl(PyObject * const * args, PyObject * kwnames, TempType & items) const -> bool {

        if (!kwnames)
            return true;

        auto count = PyTuple_GET_SIZE(kwnames);
        for(decltype(count) i = 0; i < count; ++i) {
            auto elem = PyTuple_GET_ITEM(kwnames, i);
            auto elemName = PyUnicode_AsUTF8(elem);
            bool res = this->setKeyWordArg<0>(args[i], elemName, items);
            if (!res)
                return false;
        }
        return true;
    }

    template<size_t I>
    auto parseFastCallPosImpl(PyObject * const * args, Py_ssize_t nargs, PyObject * kwnames, TempType & items) const -> bool {

        if (I == nargs) {
            return this->parseFastCallKeywordImpl(args + nargs, kwnames, items);
        }

        auto & item = std::get<I>(items);
        using ExtractType = std::tuple_element_t<I, RawTypes>;
        item = ::fromPython<ExtractType>(args[I], std::get<I>(m_nameProviders));
        if (!item)
            return false;

        if constexpr (I + 1 < T::end_of_mixed) {
            return this->parseFastCallPosImpl<I + 1>(args, nargs, kwnames, items);
        } else {
            return this->parseFastCallKeywordImpl(args + nargs, kwnames, items);
        }
    }

    template<size_t I>
    auto validateRequired(TempType & items) const -> bool {

        if constexpr (std::tuple_element_t<I, decltype(T::args)>::required) {
            if (!std::get<I>(items)) {
                ArgErrorMaker<T>::missingRequired(m_desc, std::get<I>(m_nameProviders).name);
                return false;
            }
            if constexpr (I == std::tuple_size_v<TempType> - 1)
                return true;
            else
                return this->validateRequired<I+1>(items);
        } else {
            return true;
        }
    }
public:
    ArgumentsFromPythonConverter(const T & desc): 
        m_desc(&desc),
        m_nameProviders(makeNameProvider(desc))
    {}

    auto fromVarArgs(PyObject * args, PyObject * kwargs) const -> std::optional<Arguments<T>> {

        if (!PyTuple_Check(args)) {
            Py_FatalError("args must be a tuple");
        }
        if (kwargs && !PyDict_Check(kwargs)) {
            Py_FatalError("kwargs must be a dict");
        }

        size_t argsSize = PyTuple_GET_SIZE(args);
        if (argsSize > T::end_of_mixed) {
            ArgErrorMaker<T>::tooManyArgs(m_desc);
            return std::nullopt;
        }

        TempType items;

        if constexpr (std::tuple_size_v<decltype(T::args)> > 0) {
            if (!this->parseVarArgPosImpl<0>(args, argsSize, kwargs, items))
                return std::nullopt;
            if (!this->validateRequired<0>(items))
                return std::nullopt;
        } else {
            if (!parseVarArgKeywordImpl(kwargs, items))
                return std::nullopt;
        }


        return tupleTransformIndexed<Arguments<T>, Transformer>(std::move(items));
        
    }

    auto fromFastCall(PyObject * const * args, Py_ssize_t nargs, PyObject * kwnames) const -> std::optional<Arguments<T>> {

        if (kwnames && !PyTuple_Check(kwnames)) {
            Py_FatalError("kwnames must be a tuple");
        }

        if (size_t(nargs) > T::end_of_mixed) {
            ArgErrorMaker<T>::tooManyArgs(m_desc);
            return std::nullopt;
        }

        TempType items;

        if constexpr (std::tuple_size_v<decltype(T::args)> > 0) {
            if (!this->parseFastCallPosImpl<0>(args, nargs, kwnames, items))
                return std::nullopt;
            if (!this->validateRequired<0>(items))
                return std::nullopt;
        } else {
            if (!parseFastCallKeywordImpl(args, kwnames, items))
                return std::nullopt;
        }

        return tupleTransformIndexed<Arguments<T>, Transformer>(std::move(items));
    }
private:
    const T * m_desc;
    NameProvidersType m_nameProviders;
};

template<ArgumentsDescriptor T>
auto parseVarArgArguments(PyObject * args, const T & desc) -> std::optional<Arguments<T>> {
    return ArgumentsFromPythonConverter(desc).fromVarArgs(args, nullptr);
}

template<ArgumentsDescriptor T>
auto parseVarArgArguments(PyObject * args, PyObject * kwargs, const T & desc) -> std::optional<Arguments<T>> {
    return ArgumentsFromPythonConverter(desc).fromVarArgs(args, kwargs);
}

template<ArgumentsDescriptor T>
auto parseFastCallArguments(PyObject * const * args, Py_ssize_t nargs, const T & desc) -> std::optional<Arguments<T>> {
    return ArgumentsFromPythonConverter(desc).fromFastCall(args, nargs, nullptr);
}

template<ArgumentsDescriptor T>
auto parseFastCallArguments(PyObject * const * args, Py_ssize_t nargs, PyObject * kwnames, const T & desc) -> std::optional<Arguments<T>> {
    return ArgumentsFromPythonConverter(desc).fromFastCall(args, nargs, kwnames);
}


#endif

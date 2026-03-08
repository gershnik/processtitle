// Copyright (c) 2022, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HEADER_PYLIB_UTIL_H_INCLUDED
#define HEADER_PYLIB_UTIL_H_INCLUDED

template<template<class> class Transform, class Tuple>
struct TransformedTupleImpl;

template<template<class> class Transform, class... Ts>
struct TransformedTupleImpl<Transform, std::tuple<Ts...>> {
    using Type = std::tuple<Transform<Ts>...>;
};

template<template<class> class Transform, class Tuple>
using TransformedTuple = typename TransformedTupleImpl<Transform, Tuple>::Type;

template<class... Tuples>
using JoinedTuple = decltype(std::tuple_cat(std::declval<Tuples>()...));


template <class Dest, class Src, class Func, size_t... Indices>
auto doTupleTransform(Src && src, Func func, std::index_sequence<Indices...>) -> Dest {
    return Dest(func(std::get<Indices>(std::forward<Src>(src)))...);  
}


template <class Dest, class Src, class Func>
requires(std::tuple_size_v<std::remove_cvref_t<Src>> == std::tuple_size_v<Dest>)
auto tupleTransform(Src && src, Func func) -> Dest {
    return doTupleTransform<Dest>(std::forward<Src>(src), func, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Src>>>());
}

template <class Dest, template<size_t> class Func, class Src, size_t... Indices>
auto doTupleTransformIndexed(Src && src, std::index_sequence<Indices...>) -> Dest {
    return Dest(Func<Indices>()(std::get<Indices>(std::forward<Src>(src)))...);  
}


template <class Dest, template<size_t> class Func, class Src>
requires(std::tuple_size_v<Src> == std::tuple_size_v<Dest>)
auto tupleTransformIndexed(Src && src) -> Dest {
    return doTupleTransformIndexed<Dest, Func>(std::forward<Src>(src), std::make_index_sequence<std::tuple_size_v<Src>>());
}

class Catenator {
private:
    template<class First, class... Rest>
    static void appendToString(std::string & str, const First & first, const Rest & ...rest) {
        str.append(first);
        if constexpr (sizeof...(rest) > 0)
            appendToString(str, rest...);
    }
public:
    template<class... Parts>
    requires((std::is_same_v<Parts, std::string_view> && ...))
    static auto apply(const Parts & ...parts) -> std::string {
        std::string ret;
        ret.reserve((parts.size() + ...));
        appendToString(ret, parts...);
        return ret;
    }

    template<class T>
    requires(std::same_as<T, std::string_view>)
    static auto makeParam(const T & arg) -> const std::string_view &
        { return arg; }

    template<class T>
    requires(std::convertible_to<T, std::string_view> && !std::same_as<T, std::string_view>)
    static auto makeParam(const T & arg) -> std::string_view
        { return std::string_view(arg); }
};

template<class... Parts>
requires((std::is_convertible_v<Parts, std::string_view> && ...))
auto concat(const Parts & ...parts) -> std::string {
    return Catenator::apply(Catenator::makeParam(parts)...);
}


#endif

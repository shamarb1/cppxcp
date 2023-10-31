/**
 * Copyright (C) 2023 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical concepts) contained herein is, and remains the
 * property of or licensed to Volvo Car Corporation. This information is proprietary to Volvo Car Corporation and may be
 * covered by patents or patent applications. This information is protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Volvo Car Corporation.
 */

#ifndef XCP_UTILITIES
#define XCP_UTILITIES

#include <bit>
#include <cstring>

#include "sfinae.hpp"

#ifdef __cpp_lib_bit_cast

using std::bit_cast;

#else

template <typename T2, typename T1>
T2 bit_cast(const T1* src) {
    static_assert(is_pod_v<T1>, "Requires POD input");
    static_assert(is_pod_v<T2>, "Requires POD output");

    T2 dest{};
    std::memcpy(std::addressof(dest), src, sizeof(T2));
    return dest;
}

template <typename T2, typename T1>
T2 bit_cast(T1&& src) {
    static_assert(sizeof(T1) == sizeof(T2), "Types must match sizes");
    static_assert(is_pod_v<std::remove_reference_t<T1> >, "Requires POD input");
    static_assert(is_pod_v<T2>, "Requires POD output");

    T2 dest{};
    std::memcpy(std::addressof(dest), std::addressof(src), sizeof(T1));
    return dest;
}

#endif

// Network functions

template <typename T>
auto serialize(byte* bytes, const T& object)
        -> std::enable_if_t<is_same_v<decltype(object.serialize(bytes)), byte*>, byte*> {
    return object.serialize(bytes);
}

template <typename T>
auto serialize(byte* bytes, const T& object) -> std::enable_if_t<is_fundamental_v<T>, byte*> {
    const size_t size = sizeof(T);
    std::memcpy(bytes, &object, size);
    return (bytes + size);
}

template <typename T, typename... Ts>
byte* serialize(byte* bytes, const T& object, const Ts&... objects) {
    return serialize(serialize(bytes, object), objects...);
}

template <typename T>
auto deserialize(const byte* bytes, T& object)
        -> std::enable_if_t<is_same_v<decltype(object.deserialize(bytes)), const byte*>, const byte*> {
    return object.deserialize(bytes);
}

template <typename T>
auto deserialize(const byte* bytes, T& object) -> std::enable_if_t<is_fundamental_v<T>, const byte*> {
    const size_t size = sizeof(T);
    std::memcpy(&object, bytes, size);
    return (bytes + size);
}

template <typename T, typename... Ts>
const byte* deserialize(const byte* bytes, T& object, Ts&... objects) {
    return deserialize(deserialize(bytes, object), objects...);
}

template <typename T>
auto calcNetworkSize(const T& object)
        -> std::enable_if_t<is_same_v<decltype(object.getNetworkSize()), size_t>, size_t> {
    return object.getNetworkSize();
}

template <typename T>
auto calcNetworkSize(T) -> std::enable_if_t<is_fundamental_v<T>, size_t> {
    return sizeof(T);
}

template <typename T, typename... Ts>
size_t calcNetworkSize(const T& object, const Ts&... objects) {
    return calcNetworkSize(object) + calcNetworkSize(objects...);
}

#endif

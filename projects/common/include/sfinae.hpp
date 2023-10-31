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

#ifndef XCP_SFINAE
#define XCP_SFINAE

#include <type_traits>

template <typename... T>
using void_t = void;

template <typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;
template <typename T, typename U>
constexpr bool is_same_v = std::is_same<T, U>::value;
template <typename T>
constexpr bool is_const_v = std::is_const<T>::value;
template <typename T>
constexpr bool is_pod_v = std::is_pod<T>::value;
template <typename T>
constexpr bool is_fundamental_v = std::is_fundamental<T>::value;

#endif

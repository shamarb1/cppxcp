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

#ifndef XCP_COMMON_TYPES
#define XCP_COMMON_TYPES

#include <array>
#include <cstdint>
#include <limits>

#include "span.hpp"

using byte = std::uint8_t;
using word = std::uint16_t;
using dword = std::uint32_t;

template <size_t size>
using Data = std::array<byte, size>;
using DataView = span<byte>;
using ConstDataView = span<const byte>;

template <typename T,
          T init_v = std::numeric_limits<T>::min(),
          T min_v = std::numeric_limits<T>::min(),
          T max_v = std::numeric_limits<T>::max()>
class PrimitiveValidator {
  public:
    static constexpr T min = min_v;
    static constexpr T max = max_v;
    static_assert(min <= max, "`min` shall be lower or equal than `max`.");

    constexpr PrimitiveValidator() noexcept = default;
    constexpr PrimitiveValidator(T value) noexcept : value(value) {}
    constexpr operator T() const noexcept { return value; }
    constexpr bool isValid() const noexcept { return (value >= min && value <= max); }

  protected:
    T value = init_v;
};

#endif

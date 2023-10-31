/**
 * Copyright (Container) 2023 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical concepts) contained herein is, and remains the
 * property of or licensed to Volvo Car Corporation. This information is proprietary to Volvo Car Corporation and may be
 * covered by patents or patent applications. This information is protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Volvo Car Corporation.
 */

#ifndef XCP_SPAN
#define XCP_SPAN

#include <array>

#include "sfinae.hpp"

namespace span_traits {

template <typename Type, typename Type2>
static constexpr bool is_type_compatible_v = is_same_v<std::remove_cv_t<Type>, std::remove_cv_t<Type2>> &&
                                             (is_const_v<Type> || !is_const_v<Type2>);

template <typename Type, typename P>
static constexpr bool is_pointer_compatible_v = is_pointer_v<P>&& is_type_compatible_v<Type, std::remove_pointer_t<P>>;

template <typename Type, typename I, typename = void>
static constexpr bool is_iterator_compatible_v = false;
template <typename Type, typename I>
static constexpr bool is_iterator_compatible_v<
        Type,
        I,
        std::enable_if_t<is_same_v<std::random_access_iterator_tag, typename I::iterator_category> &&
                         is_pointer_compatible_v<Type, typename I::pointer>>> = true;

template <typename Type, typename I, typename = void>
static constexpr bool is_pointer_or_iterator_compatible_v = false;
template <typename Type, typename I>
static constexpr bool is_pointer_or_iterator_compatible_v<
        Type,
        I,
        std::enable_if_t<is_pointer_compatible_v<Type, I> || is_iterator_compatible_v<Type, I>>> = true;

template <typename Type, typename C, typename = void>
static constexpr bool is_container_compatible_v = false;
template <typename Type, typename C>
static constexpr bool
        is_container_compatible_v<Type,
                                  C,
                                  std::enable_if_t<is_pointer_compatible_v<Type, decltype(std::declval<C>().data())>>> =
                true;

template <typename Type>
Type* get_ptr(Type* ptr, size_t size) {
    return (size == 0) ? nullptr : ptr;
}
template <typename Type>
auto get_ptr(Type iter, size_t size) -> decltype(iter.operator->()) {
    return (size == 0) ? nullptr : iter.operator->();
}

}  // namespace span_traits

template <typename T>
class span {

  public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    span() noexcept = default;
    template <typename Iter, typename = std::enable_if_t<span_traits::is_pointer_or_iterator_compatible_v<T, Iter>>>
    span(Iter begin, size_type size) noexcept : ptr(span_traits::get_ptr(begin, size)), sz(size) {}
    template <typename C, typename = std::enable_if_t<span_traits::is_container_compatible_v<T, C>>>
    span(C& data) noexcept : span(data.data(), data.size()) {}
    template <typename A, size_type size, typename = std::enable_if_t<span_traits::is_type_compatible_v<T, A>>>
    span(A (&arr)[size]) noexcept : span(arr, size) {}
    template <typename Iter, typename = std::enable_if_t<span_traits::is_pointer_or_iterator_compatible_v<T, Iter>>>
    span(Iter begin, Iter end) noexcept : span(begin, std::distance(begin, end)) {}

    // assign
    template <typename C, typename = std::enable_if_t<span_traits::is_container_compatible_v<T, C>>>
    span& operator=(C& data) {
        ptr = data.data();
        sz = data.size();
        return *this;
    }

    // iterators
    iterator begin() const noexcept { return ptr; }
    iterator end() const noexcept { return ptr + sz; }
    const_iterator cbegin() const noexcept { return ptr; }
    const_iterator cend() const noexcept { return ptr + sz; }
    reverse_iterator rbegin() const noexcept { return reverse_iterator{ptr + sz}; }
    reverse_iterator rend() const noexcept { return reverse_iterator{ptr}; }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{ptr + sz}; }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator{ptr}; }

    // element access
    reference front() const noexcept { return *ptr; }
    reference back() const noexcept { return ptr[sz - 1]; }
    reference operator[](size_type index) const noexcept { return ptr[index]; }
    pointer data() const noexcept { return ptr; }

    // observers
    size_type size() const noexcept { return sz; }
    size_type size_bytes() const noexcept { return sz * sizeof(element_type); }
    bool empty() const noexcept { return (sz == 0); }

    // subviews
    span subspan(size_type offset, size_type size) { return {ptr + offset, size}; }
    span first(size_type size) { return {ptr, size}; }
    span last(size_type size) { return {ptr + sz - size, size}; }

  private:
    element_type* ptr = nullptr;
    size_type sz = 0;
};

#endif

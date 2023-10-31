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

#include "udp_socket_types.hpp"

#include <arpa/inet.h>

#include <cstring>

#include "utilities.hpp"

Ip4Address::Ip4Address(const char* address) {
    if (address) {
        if (strcmp(address, "localhost") == 0) {
            address = "127.0.0.1";
        }
        value = inet_addr(address);
    }
}

Ip4Address& Ip4Address::operator=(const char* address) {
    return *this = Ip4Address{address};
}

bool Ip4Address::toString(char* str) const noexcept {
    if (isValid() && str) {
        return inet_ntop(AF_INET, &value, str, INET_ADDRSTRLEN);
    }
    return false;
}

SocketAddress::SocketAddress(Ip4Address ipAddress, const SocketPort port) noexcept {
    if (ipAddress.isValid() && port.isValid()) {
        init(port, ipAddress);
    }
}

SocketAddress::SocketAddress(const SocketPort port) noexcept {
    if (port.isValid()) {
        init(port, INADDR_ANY);
    }
}

bool SocketAddress::operator==(const SocketAddress& other) const noexcept {
    return (std::memcmp(&socketAddress, &other.socketAddress, sizeof(SocketAddress)) == 0);
}

bool SocketAddress::operator!=(const SocketAddress& other) const noexcept {
    return !operator==(other);
}

bool SocketAddress::isValid() const noexcept {
    return (socketAddress.sa_family != invalidValue);
}

Ip4Address SocketAddress::getIp4Address() const noexcept {
    return bit_cast<in_addr_t>(socketAddress.sa_data + 2);
}

SocketPort SocketAddress::getPort() const noexcept {
    return htons(bit_cast<in_port_t>(socketAddress.sa_data));
}

const sockaddr* SocketAddress::operator&() const noexcept {
    return &socketAddress;
}

sockaddr* SocketAddress::operator&() noexcept {
    return &socketAddress;
}

void SocketAddress::init(const in_port_t port, const in_addr_t address) noexcept {
    const sockaddr_in sockAddrIn{
            AF_INET,      // sin_family
            htons(port),  // sin_port
            address,      // sin_addr
            0             // sin_zero
    };
    socketAddress = bit_cast<sockaddr>(sockAddrIn);
}

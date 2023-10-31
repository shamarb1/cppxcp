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

#include "udp_socket.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

UdpSocket::UdpSocket(UdpSocket&& other) noexcept {
    value = other.value;
    other.value = -1;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
}

bool UdpSocket::init() noexcept {
    if (!isValid()) {
        value = ::socket(AF_INET, SOCK_DGRAM, 0);
    }
    return isValid();
}

bool UdpSocket::bind(SocketPort port) const noexcept {
    if (isValid() && port.isValid()) {
        const SocketAddress address{port};
        return (::bind(value, &address, INET_ADDRSTRLEN) >= 0);
    }
    return false;
}

bool UdpSocket::close() noexcept {
    if (isValid() && (::close(value) == 0)) {
        value = -1;
    }
    return !isValid();
}

bool UdpSocket::send(ConstSocketDataView data, const SocketAddress& address) const noexcept {
    if (isValid() && address.isValid() && (data.size() <= maxSocketDataSize)) {
        return (sendto(value, data.data(), data.size(), MSG_CONFIRM, &address, sizeof(sockaddr)) >= 0);
    }
    return false;
}

bool UdpSocket::receive(SocketDataView data, SocketAddress& address) const noexcept {
    if (isValid() && (data.size() <= maxSocketDataSize)) {
        socklen_t len = sizeof(sockaddr);
        const ssize_t bytes = recvfrom(value, data.data(), data.size(), MSG_WAITALL, &address, &len);
        return (bytes >= 0);
    }
    return false;
}
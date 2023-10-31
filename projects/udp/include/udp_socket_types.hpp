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

#ifndef XCP_UDP_SOCKET_TYPES
#define XCP_UDP_SOCKET_TYPES

#include <netinet/in.h>

#include "common_types.hpp"

// Maximum IP packet size is 65535 bytes.
constexpr size_t maxPacketSize = 65535;
// IP header is 20 bytes.
constexpr size_t ipHeaderSize = 20;
// UDP header is 8 bytes.
constexpr size_t udpHeaderSize = 8;
// Maximum payload size is 65507 (65535 - 20 - 8) bytes.
constexpr size_t maxSocketDataSize = maxPacketSize - ipHeaderSize - udpHeaderSize;

template <size_t size>
using SocketData = Data<size>;
using SocketDataView = DataView;
using ConstSocketDataView = ConstDataView;

using SocketPort = PrimitiveValidator<in_port_t, 0, 1>;

class Ip4Address : public PrimitiveValidator<in_addr_t,
                                             static_cast<in_addr_t>(INADDR_NONE),
                                             0,
                                             static_cast<in_addr_t>(INADDR_NONE - 1)> {
  public:
    using PrimitiveValidator::PrimitiveValidator;
    Ip4Address(const char* address);
    Ip4Address& operator=(const char* address);
    bool toString(char* str) const noexcept;
};

class SocketAddress {
  public:
    SocketAddress() noexcept = default;
    explicit SocketAddress(Ip4Address ipAddress, SocketPort port) noexcept;
    explicit SocketAddress(SocketPort port) noexcept;
    bool operator==(const SocketAddress&) const noexcept;
    bool operator!=(const SocketAddress&) const noexcept;
    bool isValid() const noexcept;
    Ip4Address getIp4Address() const noexcept;
    SocketPort getPort() const noexcept;
    const sockaddr* operator&() const noexcept;
    sockaddr* operator&() noexcept;

  private:
    void init(in_port_t port, in_addr_t address) noexcept;

    static constexpr auto invalidValue = std::numeric_limits<sa_family_t>::max();
    sockaddr socketAddress = {invalidValue, 0};
};

#endif

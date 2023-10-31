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

#ifndef XCP_UDP_SOCKET
#define XCP_UDP_SOCKET

#include "udp_socket_types.hpp"

class UdpSocket : public PrimitiveValidator<int, -1, 0> {
  public:
    using PrimitiveValidator::PrimitiveValidator;
    UdpSocket(UdpSocket&&) noexcept;
    UdpSocket& operator=(UdpSocket&&) noexcept;
    bool init() noexcept;
    bool bind(SocketPort port) const noexcept;
    bool close() noexcept;
    bool send(ConstSocketDataView data, const SocketAddress& address) const noexcept;
    bool receive(SocketDataView data, SocketAddress& address) const noexcept;
};

#endif

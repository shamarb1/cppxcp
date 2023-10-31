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

#include <gtest/gtest.h>

#include <cstring>

#include "udp_socket.hpp"

TEST(udp_socket, port) {
    SocketPort portUninitialized;
    SocketPort portZero = 0;
    const SocketPort portValid = 1024;

    EXPECT_FALSE(portUninitialized.isValid());
    EXPECT_FALSE(portZero.isValid());
    EXPECT_TRUE(portValid.isValid());

    const SocketPort portCopy = portUninitialized;
    portZero = portValid;

    EXPECT_FALSE(portCopy.isValid());
    EXPECT_TRUE(portZero.isValid());

    portUninitialized = 42;
    EXPECT_TRUE(portUninitialized.isValid());
    EXPECT_TRUE(portUninitialized == 42);
    EXPECT_TRUE(portUninitialized != 41);
    EXPECT_TRUE(portUninitialized > 0);
    EXPECT_TRUE(portUninitialized < 100);
    EXPECT_TRUE(portUninitialized >= 0);
    EXPECT_TRUE(portUninitialized <= 100);

    in_port_t value = portUninitialized;
    EXPECT_EQ(value, 42);
}

TEST(udp_socket, ip4) {
    Ip4Address ip4Uninitialized;
    EXPECT_FALSE(ip4Uninitialized.isValid());

    Ip4Address ip4Nullptr = nullptr;
    EXPECT_FALSE(ip4Nullptr.isValid());

    const Ip4Address ip4Localhost = "localhost";
    EXPECT_TRUE(ip4Localhost.isValid());

    const Ip4Address ip4Random = "foobar";
    EXPECT_FALSE(ip4Random.isValid());

    char buffer[INET_ADDRSTRLEN] = {};
    EXPECT_FALSE(ip4Random.toString(buffer));

    Ip4Address ip4Number = "192.168.48.32";
    EXPECT_TRUE(ip4Number.isValid());

    EXPECT_FALSE(ip4Number.toString(nullptr));

    EXPECT_TRUE(ip4Number.toString(buffer));
    EXPECT_EQ(strcmp(buffer, "192.168.48.32"), 0);

    ip4Uninitialized = ip4Number;
    EXPECT_TRUE(ip4Uninitialized.isValid());
    EXPECT_EQ(ip4Uninitialized, ip4Number);

    ip4Number = nullptr;
    EXPECT_FALSE(ip4Number.isValid());

    const Ip4Address ip4NullptrCopy = ip4Nullptr;
    EXPECT_FALSE(ip4NullptrCopy.isValid());
    EXPECT_EQ(ip4NullptrCopy, ip4Nullptr);

    ip4Nullptr = "localhost";
    EXPECT_TRUE(ip4Nullptr.isValid());
    EXPECT_TRUE(ip4Nullptr.toString(buffer));
    EXPECT_EQ(strcmp(buffer, "127.0.0.1"), 0);
}

TEST(udp_socket, address) {
    const SocketPort portInvalid = 0;
    const SocketPort portValid = 1024;

    const SocketAddress addressUninitialized;
    EXPECT_FALSE(addressUninitialized.isValid());

    const SocketAddress addressNullInvalid(nullptr, portInvalid);
    EXPECT_FALSE(addressNullInvalid.isValid());

    const SocketAddress addressInvalidValid("nullptr", portValid);
    EXPECT_FALSE(addressInvalidValid.isValid());

    const SocketAddress addressInvalid(portInvalid);
    EXPECT_FALSE(addressInvalid.isValid());

    const SocketAddress addressLocalhostValid("localhost", portValid);
    EXPECT_TRUE(addressLocalhostValid.isValid());

    EXPECT_EQ(addressLocalhostValid.getIp4Address(), Ip4Address{"localhost"});
    EXPECT_EQ(addressLocalhostValid.getPort(), portValid);

    const SocketAddress addressNullValid(nullptr, portValid);
    EXPECT_FALSE(addressNullValid.isValid());

    const SocketAddress addressValidValid("192.128.10.52", portValid);
    EXPECT_TRUE(addressValidValid.isValid());

    const SocketAddress addressValid(portValid);
    EXPECT_TRUE(addressValid.isValid());
}

TEST(udp_socket, socket) {
    UdpSocket socket;
    EXPECT_FALSE(socket.isValid());
    EXPECT_TRUE(socket.close());
    EXPECT_FALSE(socket.isValid());

    EXPECT_TRUE(socket.init());
    EXPECT_TRUE(socket.isValid());

    EXPECT_TRUE(socket.init());
    EXPECT_TRUE(socket.isValid());

    const SocketPort portInvalid;
    const SocketPort portValid = 1024;
    EXPECT_FALSE(socket.bind(portInvalid));
    EXPECT_TRUE(socket.isValid());
    EXPECT_TRUE(socket.bind(portValid));
    EXPECT_TRUE(socket.isValid());
    EXPECT_FALSE(socket.bind(portValid));
    EXPECT_TRUE(socket.isValid());

    UdpSocket socket2 = std::move(socket);
    EXPECT_TRUE(socket2.isValid());
    EXPECT_FALSE(socket.isValid());

    socket = std::move(socket2);
    EXPECT_TRUE(socket.isValid());
    EXPECT_FALSE(socket2.isValid());

    EXPECT_TRUE(socket.close());
    EXPECT_TRUE(socket.close());
    EXPECT_FALSE(socket.isValid());
}

TEST(udp_socket, connections) {
    const SocketPort portValid = 30000;

    SocketAddress address("localhost", portValid);
    SocketAddress addressClient1;
    SocketAddress addressClient2;

    constexpr size_t dataSize = 42;
    SocketData<0> dataZero = {};
    const SocketData<dataSize> dataAvg = {"42 data"};
    const SocketData<maxSocketDataSize> dataMax = {"max data"};

    SocketData<0> dataReceivedZero{};
    SocketData<dataSize> dataReceivedAvg{};
    SocketData<maxSocketDataSize> dataReceivedMax{};

    UdpSocket server;
    UdpSocket client;

    EXPECT_FALSE(server.receive(dataZero, addressClient1));

    EXPECT_TRUE(server.init());
    EXPECT_TRUE(server.bind(portValid));

    EXPECT_FALSE(client.send(dataZero, address));

    EXPECT_TRUE(client.init());
    EXPECT_TRUE(client.send(dataZero, address));
    EXPECT_TRUE(client.send(dataAvg, address));
    EXPECT_TRUE(client.send(dataMax, address));

    EXPECT_TRUE(server.receive(dataReceivedZero, addressClient1));
    EXPECT_TRUE(server.receive(dataReceivedAvg, addressClient1));
    EXPECT_TRUE(server.receive(dataReceivedMax, addressClient2));

    EXPECT_EQ(dataZero, dataReceivedZero);
    EXPECT_EQ(dataAvg, dataReceivedAvg);
    EXPECT_EQ(dataMax, dataReceivedMax);

    EXPECT_FALSE(addressClient1 != addressClient2);

    EXPECT_TRUE(server.send(dataMax, addressClient1));
    EXPECT_TRUE(server.send(dataAvg, addressClient1));

    EXPECT_TRUE(client.receive(dataReceivedAvg, addressClient2));
    EXPECT_TRUE(client.receive(dataReceivedMax, addressClient2));

    EXPECT_EQ(std::memcmp(dataMax.data(), dataReceivedAvg.data(), std::min(dataMax.size(), dataReceivedAvg.size())), 0);
    EXPECT_EQ(std::memcmp(dataAvg.data(), dataReceivedMax.data(), std::min(dataAvg.size(), dataReceivedMax.size())), 0);

    EXPECT_TRUE(server.close());
    EXPECT_TRUE(client.close());

    EXPECT_FALSE(server.send(dataZero, address));
    EXPECT_FALSE(server.receive(dataZero, address));
    EXPECT_FALSE(client.send(dataZero, address));
    EXPECT_FALSE(client.receive(dataZero, address));
}

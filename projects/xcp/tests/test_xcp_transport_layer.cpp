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

#include "xcp_transport_types.hpp"

TEST(transport_layer_types, constants) {
    EXPECT_EQ(transportLayerVersionMajor, 1);
    EXPECT_EQ(transportLayerVersionMinor, 5);
    EXPECT_EQ(transportLayerVersion, transportLayerVersionMajor << sizeof(char) | transportLayerVersionMinor);

    EXPECT_TRUE((MAX_CTO >= 0x08) && (MAX_CTO <= 0xFF));
    EXPECT_TRUE((MAX_DTO >= 0x0008) && (MAX_DTO <= 0xFFFF));
}

TEST(transport_layer_types, Header) {
    Header header;

    // Specification Ethernet Transport Layer 1.5: 4.3.1 HEADER
    EXPECT_EQ(header.getNetworkSize(), 4);

    Data<sizeof(Header)> data = {1, 2, 3, 4};
    header.deserialize(data.data());
    EXPECT_EQ(header.getLen(), 513);
    EXPECT_EQ(header.getCtr(), 1027);

    header.setLen(42);
    header.setCtr(43);
    EXPECT_EQ(header.getLen(), 42);
    EXPECT_EQ(header.getCtr(), 43);

    header.serialize(data.data());
    EXPECT_EQ(data[0], 42);
    EXPECT_EQ(data[2], 43);

    Header header2{24, 23};
    EXPECT_EQ(header2.getLen(), 24);
    EXPECT_EQ(header2.getCtr(), 23);
}

TEST(transport_layer_types, Message) {
    Message message;
    // TODO: fix the expected network size of the Message after the reimplementation of Packet.
    EXPECT_EQ(message.getNetworkSize(), 4);

    Data<sizeof(Header)> data = {1, 2, 3, 4};
    message.deserialize(data.data());

    Header tmpHeader{513, 1027};
    EXPECT_EQ(std::memcmp(&message.getHeader(), &tmpHeader, sizeof(Header)), 0);

    Header header{42, 43};
    message.setHeader(header);
    EXPECT_EQ(std::memcmp(&message.getHeader(), &header, sizeof(Header)), 0);

    message.serialize(data.data());
    EXPECT_EQ(data[0], 42);
    EXPECT_EQ(data[2], 43);

    Packet packet;
    message.setPacket(packet);
    EXPECT_EQ(std::memcmp(&message.getPacket(), &packet, sizeof(Packet)), 0);

    const Message message2{header, packet};
    EXPECT_EQ(std::memcmp(&message2.getPacket(), &packet, sizeof(Packet)), 0);
}

TEST(transport_layer_types, UdpClient) {
    std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
    SocketAddress address{"1.2.3.4", 42};
    UdpClient client{timeNow, address};

    EXPECT_EQ(client.getAddress(), address);
    EXPECT_EQ(client.getTimeOfLastRequest(), timeNow);

    EXPECT_FALSE(client.isConnected());
    client.connect();
    EXPECT_TRUE(client.isConnected());
    client.disconnect();
    EXPECT_FALSE(client.isConnected());

    client.setTimeOfLastPeriodicResponse(timeNow);
    EXPECT_EQ(client.getTimeOfLastPeriodicResponse(), timeNow);

    Message message{Header{42, 43}, Packet{}};
    EXPECT_TRUE(client.pushMessageToUdpFrame(message));

    const ConstSocketDataView socketData = client.getSocketData();
    using Data4 = Data<4>;
    EXPECT_EQ(std::memcmp(socketData.data(), Data4{42, 0, 43, 0}.data(), Data4{}.size()), 0);

    client.cleanUdpFrame();
    EXPECT_EQ(std::memcmp(socketData.data(), Data4{}.data(), Data4{}.size()), 0);

    for (size_t i = 0, sz = udpMtuSize / message.getNetworkSize(); i < sz; ++i) {
        EXPECT_TRUE(client.pushMessageToUdpFrame(message));
    }
    EXPECT_FALSE(client.pushMessageToUdpFrame(message));
}
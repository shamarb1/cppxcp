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

#ifndef XCP_XCP_TRANSPORT_TYPES
#define XCP_XCP_TRANSPORT_TYPES

#include <array>
#include <chrono>

#include "common_types.hpp"
#include "udp_socket_types.hpp"
#include "xcp_types.hpp"

// Specification Ethernet Transport Layer 1.5: 3.1.1 ETHERNET TRANSPORT LAYER
// Version 1.5. High byte 1, low byte 5.
constexpr byte transportLayerVersionMajor = 0x01;
constexpr byte transportLayerVersionMinor = 0x05;
constexpr word transportLayerVersion = transportLayerVersionMajor << sizeof(byte) | transportLayerVersionMinor;

// Specification Ethernet Transport Layer 1.5: 4.4 THE LIMITS OF PERFORMANCE
// Range of value 0x08 – 0xFF.
constexpr byte MAX_CTO = 0xFF;  // Maximum length of CTO ethernet packets.

/* Maximum Transmission Unit for UDP packets. Should not be set over 1472 bytes to avoid fragmentation */
constexpr uint16_t udpMtuSize = 1400;  // Range of value 0x0000 – 0xFFFF
constexpr uint16_t dtoPacketsPerUdpMtu = 4;

// Specification Ethernet Transport Layer 1.5: 4.4 THE LIMITS OF PERFORMANCE
/* Range of value 0x0008 – 0xFFFF. */
// To get a decent fill rate of a UDP packet, let DTO fit several times in a single UDP frame.
constexpr word MAX_DTO = udpMtuSize / dtoPacketsPerUdpMtu;  // Maximum length of DTO packets in bytes.

// Specification Ethernet Transport Layer 1.5: 4.3.1 HEADER
class Header {
  public:
    Header() = default;
    Header(word length, word counter);
    byte* serialize(byte*) const;
    const byte* deserialize(const byte*);
    word getLen() const;
    void setLen(word length);
    word getCtr() const;
    void setCtr(word counter);
    size_t getNetworkSize() const;

  private:
    word len{};  // LEN is the number of bytes in the original XCP Packet.
    word ctr{};  // The CTR value in the XCP Header allows detection of missing Packets.
};

// Specification Ethernet Transport Layer 1.5: 4.3 HEADER AND TAIL
class Message {
  public:
    Message() = default;
    Message(const Header&, const Packet&);
    byte* serialize(byte*) const;
    const byte* deserialize(const byte*);
    const Header& getHeader() const;
    void setHeader(const Header&);
    const Packet& getPacket() const;
    void setPacket(const Packet&);
    size_t getNetworkSize() const;

  private:
    Header header{};
    Packet packet{};
    // Specification Ethernet Transport Layer 1.5: 4.3.2 TAIL
    // Tail is managed in UDP frame constructing.
};

enum class TransportServerError : int {
    noError = 0,
    startFailed,     // Unable to start server by `run` method.
    stopFailed,      // Unable to stop server by `stop` method.
    requestFailed,   // Unable to receive data from the client.
    responseFailed,  // Unable to send data to the client.
    clientListFull,  // There is no vacant slot to add a new client.
};

/* Maximum number of clients that can be connected to XCP server at the same time */
constexpr size_t maxClients = 2;
using ClientId = PrimitiveValidator<size_t,
                                    std::numeric_limits<size_t>::max(),
                                    std::numeric_limits<size_t>::min(),
                                    maxClients - 1>;

class UdpFrame {
  public:
    bool pushMessage(const Message& message);
    void clear();
    ConstSocketDataView getSocketData() const;

  private:
    size_t position = 0;
    SocketData<udpMtuSize> data;
};

// This class manages resources per each client during communications with the server.
class UdpClient {
  public:
    UdpClient() = default;
    UdpClient(std::chrono::steady_clock::time_point requestTime, const SocketAddress& address);
    void connect();
    void disconnect();
    bool isConnected() const;
    const SocketAddress& getAddress() const;
    std::chrono::steady_clock::time_point getTimeOfLastRequest() const;
    std::chrono::steady_clock::time_point getTimeOfLastPeriodicResponse() const;
    void setTimeOfLastPeriodicResponse(std::chrono::steady_clock::time_point);
    ConstSocketDataView getSocketData() const;
    void cleanUdpFrame();
    bool pushMessageToUdpFrame(const Message& message);

  private:
    SocketAddress address{};
    std::chrono::steady_clock::time_point timeOfLastRequest{};
    std::chrono::steady_clock::time_point timeOfLastPeriodicResponse{};
    bool connected = false;
    UdpFrame udpFrame{};
};

using Clients = std::array<UdpClient, ClientId::max + 1>;

#endif

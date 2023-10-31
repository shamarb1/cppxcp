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

#include "xcp_transport_types.hpp"

#include "utilities.hpp"

// Header

Header::Header(word length, word counter) : len(length), ctr(counter) {}

byte* Header::serialize(byte* bytes) const {
    return ::serialize(bytes, len, ctr);
}

const byte* Header::deserialize(const byte* bytes) {
    return ::deserialize(bytes, len, ctr);
}

uint16_t Header::getLen() const {
    return len;
}

void Header::setLen(word length) {
    len = length;
}

uint16_t Header::getCtr() const {
    return ctr;
}

void Header::setCtr(word counter) {
    ctr = counter;
}

size_t Header::getNetworkSize() const {
    return ::calcNetworkSize(len, ctr);
}

// Message

Message::Message(const Header& header, const Packet& packet) : header(header), packet(packet) {}

byte* Message::serialize(byte* bytes) const {
    return ::serialize(bytes, header, packet);
}

const byte* Message::deserialize(const byte* bytes) {
    return ::deserialize(bytes, header, packet);
}

const Header& Message::getHeader() const {
    return header;
}

void Message::setHeader(const Header& header) {
    this->header = header;
}

const Packet& Message::getPacket() const {
    return packet;
}

void Message::setPacket(const Packet& packet) {
    this->packet = packet;
}

size_t Message::getNetworkSize() const {
    return ::calcNetworkSize(header, packet);
}

// UdpFrame

bool UdpFrame::pushMessage(const Message& message) {
    if ((position + message.getNetworkSize()) <= data.size()) {
        byte* bytes = data.data() + position;
        byte* newBytes = message.serialize(bytes);
        position += std::distance(bytes, newBytes);
        return true;
    }
    return false;
}

void UdpFrame::clear() {
    position = 0;
    data.fill(0);
}

ConstSocketDataView UdpFrame::getSocketData() const {
    return data;
}

// UdpClient

using namespace std::chrono;

UdpClient::UdpClient(steady_clock::time_point requestTime, const SocketAddress& address)
    : address(address), timeOfLastRequest(requestTime) {}

void UdpClient::connect() {
    connected = true;
}

void UdpClient::disconnect() {
    connected = false;
}

bool UdpClient::isConnected() const {
    return connected;
}

const SocketAddress& UdpClient::getAddress() const {
    return address;
}

steady_clock::time_point UdpClient::getTimeOfLastRequest() const {
    return timeOfLastRequest;
}

steady_clock::time_point UdpClient::getTimeOfLastPeriodicResponse() const {
    return timeOfLastPeriodicResponse;
}

void UdpClient::setTimeOfLastPeriodicResponse(steady_clock::time_point time) {
    timeOfLastPeriodicResponse = time;
}

ConstSocketDataView UdpClient::getSocketData() const {
    return udpFrame.getSocketData();
}

void UdpClient::cleanUdpFrame() {
    udpFrame.clear();
}

bool UdpClient::pushMessageToUdpFrame(const Message& message) {
    return udpFrame.pushMessage(message);
}

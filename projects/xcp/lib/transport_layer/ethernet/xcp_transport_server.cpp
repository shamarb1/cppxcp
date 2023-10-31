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

#include "xcp_transport_server.hpp"

ProxyTransportServer::ProxyTransportServer(TransportServer& server) : transportServer(server) {}

void ProxyTransportServer::onStart() {
    transportServer.onStart();
}

void ProxyTransportServer::onStop() {
    transportServer.onStop();
}

void ProxyTransportServer::onRequest(RequestData data, const SocketAddress& address) {
    const ClientId id = transportServer.tryAddClient(address);
    if (id.isValid()) {
        Message message;
        message.deserialize(data.data());
        transportServer.onMessage(id, message);
    } else {
        transportServer.onError(TransportServerError::clientListFull);
    }
}

void ProxyTransportServer::onError(UdpServerError error) {
    switch (error) {
        case UdpServerError::noError:
            transportServer.onError(TransportServerError::noError);
            break;
        case UdpServerError::requestFailed:
            transportServer.onError(TransportServerError::requestFailed);
            break;
        case UdpServerError::responseFailed:
            transportServer.onError(TransportServerError::responseFailed);
            break;
        case UdpServerError::startFailed:
            transportServer.onError(TransportServerError::startFailed);
            break;
        case UdpServerError::stopFailed:
            transportServer.onError(TransportServerError::stopFailed);
            break;
        default:
            transportServer.onError(TransportServerError::noError);
    }
}

TransportServer::TransportServer() : proxyServer(*this) {}

bool TransportServer::run() {
#define XCP_UDP_PORT 30000
    return proxyServer.run(XCP_UDP_PORT);
}

bool TransportServer::stop() {
    return proxyServer.stop();
}

bool TransportServer::connect(ClientId id) {
    if (id.isValid()) {
        clients[id].connect();
        return true;
    }
    return false;
}

bool TransportServer::disconnect(ClientId id) {
    if (id.isValid()) {
        clients[id].disconnect();
        return true;
    }
    return false;
}

bool TransportServer::respond(ClientId id, const Message& message) {
    if (id.isValid()) {
        SocketData<udpMtuSize> socketData;
        message.serialize(socketData.data());
        return proxyServer.respond(socketData, clients[id].getAddress());
    }
    return false;
}

bool TransportServer::respondPeriodically(ClientId id, const Message& message) {
    using namespace std::chrono;
    if (id.isValid()) {
        UdpClient& client = clients[id];
        constexpr steady_clock::duration responseTimeout = 2s;
        steady_clock::time_point timeNow = steady_clock::now();
        const steady_clock::duration periodOfDelay = timeNow - client.getTimeOfLastPeriodicResponse();
        const bool isUdpFrameFull = !client.pushMessageToUdpFrame(message);
        if (isUdpFrameFull || (periodOfDelay > responseTimeout)) {
            client.setTimeOfLastPeriodicResponse(timeNow);
            return proxyServer.respond(client.getSocketData(), client.getAddress());
        }
        return true;
    }
    return false;
}

// Add `clientAddress` to the list of clients if:
// - the clients is already in the list, or
// - there is a vacant slot for the new client, or
// - there is an abandoned client that can be replaced with the new one.
//   client considered abandoned if there were no event for the last `kickTimeout` period.
// Return valid ClientId for success, invalid otherwise.
ClientId TransportServer::tryAddClient(const SocketAddress& clientAddress) {
    using namespace std::chrono;
    const UdpClient newClient{steady_clock::now(), clientAddress};
    UdpClient* availableClientSlot = nullptr;
    for (UdpClient& client : clients) {
        // Get the first empty slot as an available one.
        if (!client.isConnected()) {
            availableClientSlot = &client;
            // If the client already exists, then return its ID.
        } else if (client.getAddress() == clientAddress) {
            client = newClient;
            return std::distance(clients.data(), &client);
            // If there are no available slots, then find and replace the abandoned client.
        } else {
            const steady_clock::duration periodOfInactivity =
                    newClient.getTimeOfLastRequest() - client.getTimeOfLastRequest();
            constexpr steady_clock::duration kickTimeout = 2min;
            if (periodOfInactivity > kickTimeout) {
                availableClientSlot = &client;
            }
        }
    }

    if (availableClientSlot) {
        *availableClientSlot = newClient;
        return std::distance(clients.data(), availableClientSlot);
    }
    return {};
}

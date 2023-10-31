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

#ifndef XCP_UDP_SERVER
#define XCP_UDP_SERVER

#include "udp_server_types.hpp"
#include "udp_socket.hpp"

template <size_t requestDataSize>
class UdpServer {
    static_assert(requestDataSize > 0, "Data size shall be greater then 0.");

    using ReceivedData = SocketData<requestDataSize>;

  public:
    using RequestData = ConstSocketDataView;
    using ResponseData = ConstSocketDataView;

    UdpServer() = default;
    UdpServer(const UdpServer&) = delete;
    UdpServer(UdpServer&&) noexcept = delete;
    UdpServer& operator=(const UdpServer&) = delete;
    UdpServer& operator=(UdpServer&&) noexcept = delete;
    virtual ~UdpServer() = default;

    bool run(SocketPort port);
    bool stop();
    bool respond(ResponseData, const SocketAddress&);

  protected:
    // LCOV_EXCL_START
    virtual void onStart(){};
    virtual void onStop(){};
    virtual void onRequest(RequestData, const SocketAddress&){};
    virtual void onError(UdpServerError){};
    // LCOV_EXCL_STOP

  private:
    UdpSocket socket{};
    bool isRunning = false;
};

template <size_t requestDataSize>
bool UdpServer<requestDataSize>::respond(ResponseData data, const SocketAddress& address) {
    if (socket.send(data, address)) {
        return true;
    }
    onError(UdpServerError::responseFailed);
    return false;
}

template <size_t requestDataSize>
bool UdpServer<requestDataSize>::run(SocketPort port) {
    if (!socket.init() || !socket.bind(port)) {
        onError(UdpServerError::startFailed);
        return false;
    }
    isRunning = true;
    onStart();
    ReceivedData receivedData{};
    SocketAddress clientAddress{};
    while (isRunning) {
        if (socket.receive(receivedData, clientAddress)) {
            onRequest(receivedData, clientAddress);
            receivedData.fill(0);
        } else {
            onError(UdpServerError::requestFailed);
        }
    }
    return true;
}

template <size_t requestDataSize>
bool UdpServer<requestDataSize>::stop() {
    if (socket.close()) {
        isRunning = false;
        onStop();
        return true;
    }
    onError(UdpServerError::stopFailed);
    return false;
}

#endif

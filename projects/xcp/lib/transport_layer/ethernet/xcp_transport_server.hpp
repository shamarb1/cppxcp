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

#ifndef XCP_XCP_TRANSPORT_SERVER
#define XCP_XCP_TRANSPORT_SERVER

#include "udp_server.hpp"
#include "xcp_transport_types.hpp"

class TransportServer;

class ProxyTransportServer : public UdpServer<udpMtuSize> {
  public:
    ProxyTransportServer(TransportServer&);

  private:
    void onStart() override;
    void onStop() override;
    void onRequest(RequestData, const SocketAddress&) override;
    void onError(UdpServerError) override;

    TransportServer& transportServer;
};

class TransportServer {
    friend class ProxyTransportServer;

  public:
    TransportServer();
    bool run();
    bool stop();

  protected:
    bool connect(ClientId);
    bool disconnect(ClientId);
    bool respond(ClientId, const Message&);
    bool respondPeriodically(ClientId, const Message&);

  private:
    // LCOV_EXCL_START
    virtual void onStart(){};
    virtual void onStop(){};
    virtual void onMessage(ClientId, const Message&){};
    virtual void onError(TransportServerError){};
    // LCOV_EXCL_STOP

    ClientId tryAddClient(const SocketAddress&);

    ProxyTransportServer proxyServer;
    Clients clients{};
};

#endif

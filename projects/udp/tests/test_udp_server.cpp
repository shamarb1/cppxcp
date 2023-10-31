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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

#include "udp_server.hpp"

class MockUdpServer : public UdpServer<255> {
  public:
    MOCK_METHOD(void, onStart, ());
    MOCK_METHOD(void, onStop, ());
    MOCK_METHOD(void, onRequest, (RequestData, const SocketAddress&));
    MOCK_METHOD(void, onError, (UdpServerError));

    FRIEND_TEST(udp_server, bad_run);
    FRIEND_TEST(udp_server, good_run);
    FRIEND_TEST(udp_server, broken_run);
};

TEST(udp_server, bad_run) {
    MockUdpServer server{};

    EXPECT_CALL(server, onStart).Times(0);
    EXPECT_CALL(server, onStop);
    EXPECT_CALL(server, onError);

    ON_CALL(server, onError).WillByDefault(testing::Invoke([&](UdpServerError error) {
        EXPECT_TRUE((error == UdpServerError::startFailed) || (error == UdpServerError::stopFailed));
        if (error == UdpServerError::startFailed) {
            server.stop();
        }
    }));

    const SocketPort badPort = 0;
    EXPECT_FALSE(server.run(badPort));
}

TEST(udp_server, good_run) {
    const SocketPort goodPort = 1024;
    const SocketAddress serverAddress{"localhost", goodPort};
    SocketData<1> data = {1};

    UdpSocket client1{};
    UdpSocket client2{};
    client1.init();
    client2.init();

    MockUdpServer server{};

    EXPECT_CALL(server, onStart);
    EXPECT_CALL(server, onStop);
    EXPECT_CALL(server, onRequest).Times(4);
    EXPECT_CALL(server, onError);

    ON_CALL(server, onStart).WillByDefault(testing::Invoke([&]() {
        client1.send(data, serverAddress);  // onRequest
    }));

    ON_CALL(server, onRequest)
            .WillByDefault(testing::Invoke([&](MockUdpServer::RequestData requestData, const SocketAddress& address) {
                switch (requestData[0]) {
                    case 1:
                        data[0] = 2;
                        client1.send(data, serverAddress);  // onRequest
                        break;
                    case 2: {
                        const SocketData<20> response = {"random data"};
                        SocketAddress invalidAddress;
                        EXPECT_FALSE(server.respond(response, invalidAddress));
                        EXPECT_TRUE(server.respond(response, address));

                        SocketData<20> receivedResponse;
                        SocketAddress receivedAddress;
                        client1.receive(receivedResponse, receivedAddress);
                        EXPECT_EQ(response, receivedResponse);
                        EXPECT_EQ(serverAddress, receivedAddress);

                        data[0] = 3;
                        client2.send(data, serverAddress);  // onRequest
                        break;
                    }
                    case 3:
                        data[0] = 4;
                        client1.send(data, serverAddress);  // onRequest
                        break;
                    case 4:
                    default:
                        server.stop();
                        break;
                };
            }));

    ON_CALL(server, onError).WillByDefault(testing::Invoke([&](UdpServerError error) {
        EXPECT_TRUE(error == UdpServerError::responseFailed);
    }));

    EXPECT_TRUE(server.run(goodPort));

    client1.close();
    client2.close();
}

TEST(udp_server, broken_run) {
    const SocketPort goodPort = 1024;
    const SocketAddress serverAddress{"localhost", goodPort};

    MockUdpServer server{};

    EXPECT_CALL(server, onStart);
    EXPECT_CALL(server, onStop);
    EXPECT_CALL(server, onError).Times(2);

    UdpSocket validStoredSocket{};
    // We want to access `UdpSocket` which is a private member of UdpServer (base class).
    // We have no chance to get access to private member of the base class in a legal way.
    // Therefore, we use a nasty hack using plain raw pointer arithmetics.
    // MockUdpServer has a virtual table. Assume that `vtable pointer size` == `dummy class ClassWithVirtualTable` and
    // shift by this value.
    class ClassWithVirtualTable {
        virtual ~ClassWithVirtualTable() = default;
    };
    char* socketPtr = reinterpret_cast<char*>(&server) + sizeof(ClassWithVirtualTable);

    ON_CALL(server, onStart).WillByDefault(testing::Invoke([&]() {
        char* tmpPtr = reinterpret_cast<char*>(&validStoredSocket);
        std::memcpy(tmpPtr, socketPtr, sizeof(UdpSocket));  // Store valid socket.
        std::memset(socketPtr, 1, sizeof(UdpSocket));       // Damage socket.
    }));

    ON_CALL(server, onError).WillByDefault(testing::Invoke([&](UdpServerError error) {
        EXPECT_TRUE((error == UdpServerError::stopFailed) || (error == UdpServerError::requestFailed));
        if (error == UdpServerError::requestFailed) {
            server.stop();  // onError(stopFailed)
        } else if (error == UdpServerError::stopFailed) {
            std::memcpy(socketPtr, &validStoredSocket, sizeof(UdpSocket));  // Restore valid socket.
            server.stop();                                                  // onStop
        }
    }));

    EXPECT_TRUE(server.run(goodPort));  // onStart -> onError(requestFailed)
}
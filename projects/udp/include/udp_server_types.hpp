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

#ifndef XCP_UDP_SERVER_TYPES
#define XCP_UDP_SERVER_TYPES

enum class UdpServerError : int {
    noError = 0,
    startFailed,     // Unable to start server by `run` method.
    stopFailed,      // Unable to stop server by `stop` method.
    requestFailed,   // Unable to receive data from the client.
    responseFailed,  // Unable to send data to the client.
};

#endif

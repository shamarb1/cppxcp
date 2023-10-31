/*
 * Copyright (C) 2021 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical concepts) contained herein is, and remains
 * the property of or licensed to Volvo Car Corporation. This information is proprietary to Volvo Car Corporation
 * and may be covered by patents or patent applications. This information is protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Volvo Car Corporation.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void XcpSlavePrintfVerbose(const char* format, ...);
void XcpSlavePrintfVerboseDebug(const char* format, ...);

void XcpSlavePrintfVerboseUint8Array(const char* p_header, const unsigned char* p_data, const unsigned length);

void XcpSlavePrintfVerbosePayload(int direction, const unsigned char* p_data, const unsigned int length);

#ifdef __cplusplus
}
#endif

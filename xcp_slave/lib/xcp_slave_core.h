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

#include <stdbool.h>
#include <stdint.h>

#include "xcp_slave_core_common.h"

/********** Seed & Key related functions******************************************************************************/
/* Access restriction to measure and calibrate is enabled via the ASAM standardized "Seed & Key" functionality. The  */
/* ASAM tool will ask the XCP server for a seed. This seed will be feed to an algorithm available to the ASAM tool   */
/* through a dll-file. The dll contains an algorithm identical to one present in the XCP server. The ASAM tool will  */
/* return the "key" received as return value from the "Seed & Key" algorithm to the XCP server which in its turn will*/
/* compare it to the "key" calculated by the XCP server version of the algorighm. If the keys matches, access to the */
/* resource in question (measurement or calibration) is granted. One Seed & Key sequence each is required to unlock  */
/* both resourced                                                                                                    */
/* The default size of the seed and the key is two bytes and the algorithm is the simplest possible: seed = key The  */
/* length and complexity of the Seed & Key algorithm can be changed. To do this, a new Seed & Key dll needs to be    */
/* generated. Also the algorithm present in the XCP server (in the UNLOCK request handling in function               */
/* "XcpSlaveCoreProcessCmd")                                                                                         */
/* must be updated to match the algorithm in the dll. How to handle the generation of a new dll is explained in the  */
/* "ASAM_AE_MCD-1_XCP_BS_2-2_How-to-make-a-Seed-&-Key-DLL-for-XCP_V1-5-0.pdf" document which is a part of the ASAM   */
/* standard. The ASAM standard also supplys a template visual studio projects with example source code. The          */
/* generated dll is made available for the ASAM tool by placing it in a certain tool dependent folder. The ASAM      */
/* standard limits the length of the seed and key to 255 bytes as a maximumm                                         */

#ifdef __cplusplus
extern "C" {
#endif

enum XcpPacketTxMethod { XcpPacketTxMethod_IMMEDIATE, XcpPacketTxMethod_WHEN_MTU_FULL };

typedef enum XcpPacketTxMethod XcpPacketTxMethod;

/**
 * @brief Remove all allocated common resources (DAQ lists, ODT lists, ODT entries) and remove references to common
 * resources in the "local" DAQ and ODT lists from the node struct of the client
 *
 * @param pclient The client
 */
void XcpSlaveCoreFreeResources(XcpSlaveCoreClient* pclient);

/**
 * @brief Initialization of xcp core.
 *
 * @param ProcBaseAddr_ Process base address
 */
void XcpSlaveCoreInit(const uint64_t ProcBaseAddr_);

/**
 * @briefHandle requests from XCP clients
 *
 * @param command_buf Request data buffer (contents of incoming request message from XCP client)
 * @param pclient Current client.
 * @param command_buffer_length Size of command  buffer in bytes
 *
 */
void XcpSlaveCoreProcessCmd(const uint8_t command_buf[],
                            const uint16_t command_buf_length,
                            XcpSlaveCoreClient* pclient);

/**
 * @brief Event processing.
 *
 * @param event The event to process
 */
void XcpSlaveCoreOnEvent(uint8_t event, XcpSlaveCoreClient* pclient);

/**
 * @brief Initialization of a client struct.
 *
 * @param pclient
 */
void XcpSlaveCoreInitClient(XcpSlaveCoreClient* pclient);

/**
 * @brief Closing a client struct making it ready for reuse.
 *
 * @param pclient
 */
void XcpSlaveCoreCloseClient(XcpSlaveCoreClient* pclient);

/**
 * @brief Checks if a client slot is a free one.
 *
 * @return
 */
bool XcpSlaveCoreIsClientFree(XcpSlaveCoreClient* pclient);

/**
 * @brief Sends data to client
 *
 * @return success/failure of send status
 */
int XcpSlaveCoreSendPacket(const int8_t client_id,
                           const uint8_t* data,
                           const uint16_t dataLen,
                           const XcpPacketTxMethod txMethod);

#ifdef __cplusplus
}
#endif

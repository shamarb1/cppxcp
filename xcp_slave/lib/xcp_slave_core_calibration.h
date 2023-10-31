/*
 * Copyright (C) 2021 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical concepts) contained herein is, and remains the
 * property of or licensed to Volvo Car Corporation. This information is proprietary to Volvo Car Corporation and may
 * be covered by patents or patent applications. This information is protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Volvo Car Corporation.
 *
 */

#pragma once

#include "xcp_slave_core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks whether a memory block is in caldata section
 *
 * @param block_start Start address of block
 * @param block_size Block size in bytes
 * @return true In caldata section
 * @return false Not in caldata section.
 */
bool XcpSlaveCoreIsBlockInCalData(const void* block_start, const uint32_t block_size);

/**
 * @brief Initialization of calibration support.
 *
 * @param ProcBaseAddr_ Process base address
 */
void XcpSlaveCoreCalSupportInit(const uint64_t ProcBaseAddr_);

/**
 * @brief Processes a calibration commmand from Xcp master
 *
 * @param command_buf CTO sent by master
 * @param pclient Current client
 * @param responseBuf Buffer to write the response to.
 * @param responseSize Size of response in bytes. Set to zero in case the command wasn't recognized.
 */
void XcpSlaveCoreProcessCalCommand(const uint8_t command_buf[],
                                   struct XcpSlaveCoreClient* pclient,
                                   uint8_t* const responseBuf,
                                   uint16_t* responseSize);

/**
 * @brief Whether calibration storage is in progress nor not.
 *
 * @return true In progress
 * @return false Not in progress.
 */
bool XcpSlaveCoreIsCalibrationStoreInProgress();

#ifdef __cplusplus
}
#endif

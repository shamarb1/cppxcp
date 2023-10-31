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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get the wp file name.
 */
const char* get_wp_file_name(void);

/**
 * @brief get the rp file name.
 */
const char* get_rp_file_name(void);

/**
 * @brief Writes calibration page to reference page file.
 *
 * @return true Success
 * @return false Failure
 */
bool XcpSlaveWritCalToRpFile(void);

/**
 * @brief Writes calibration page to working page file.
 *
 * @return true Success
 * @return false Failure
 */
bool XcpSlaveWritCalToWpFile(void);

/**
 * @brief Reads calibration page from reference page file.
 *
 * @return true Success
 * @return false Failure
 */
bool XcpSlaveReadCalFromRpFile(void);

/**
 * @brief Reads calibration page from working page file
 *
 * @return true Success
 * @return false Failure
 */
bool XcpSlaveReadCalFromWpFile(void);

/**
 * @brief Validates page file
 *
 * @param file File to validate
 * @return true Valid
 * @return false Not valid
 */
bool XcpSlaveIsPageFileValid(const char* file);

/**
 * @brief Reads calibration page from external file
 *
 * @return true Success
 * @return false Failure
 */
bool XcpSlaveReadCalPageFromExternalFile(void);

bool XcpSlaveValidateExternalCalData(const char* cal_file, const char* chechsum_file);
bool XcpSlaveGetChecksums(const char* checksum_filepath, uint16_t* code_checksum, uint16_t* data_checksum);
bool XcpSlaveGetCalData(const char* calibration_data_filepath, uint8_t* caldata, uint32_t* caladata_len);
bool XcpSlaveComputeChecksums(uint8_t* caldata, uint32_t caladata_len, uint16_t* computed_data_checksum);

bool get_line(FILE* ptr, char* line);
bool get_word(const char* line, char* str, int index);
void get_selected_segment_data(uint8_t* dest, uint8_t* src, int start_index, int size);

#ifdef __cplusplus
}
#endif

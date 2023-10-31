/*
 * Copyright (C) 2021 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical
 * concepts) contained herein is, and remains the property of or licensed
 * to Volvo Car Corporation. This information is proprietary to Volvo Car
 * Corporation and may be covered by patents or patent applications. This
 * information is protected by trade secret or copyright law. Dissemination of
 * this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Volvo Car Corporation.
 *
 */

#pragma once

#define CALIBRATION_FILE_PATH_SIZE 256  // Calibration data, max allowed absolute filepath length

#ifdef __cplusplus
extern "C" {
#endif

// These shall be created in linker script
extern void* text_section_start;
extern void* text_section_end;
extern void* bss_section_start;
extern void* bss_section_end;
extern void* data_section_start;
extern void* data_section_end;
extern void* caldata_section_start;
extern void* caldata_section_end;

// server port, that we got from user app
extern int server_port;

// Logger variables
extern void* user_data;
extern int log_level;

// Calibration Info
extern int is_calibration_used;
extern int is_calibration_data_from_file;
extern char calibration_data_filepath[CALIBRATION_FILE_PATH_SIZE];
extern char checksum_filepath[CALIBRATION_FILE_PATH_SIZE];

#ifdef __cplusplus
}
#endif

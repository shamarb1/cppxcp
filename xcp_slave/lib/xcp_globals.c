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

#include "xcp_globals.h"

// These shall be created in linker script
void* text_section_start;
void* text_section_end;
void* bss_section_start;
void* bss_section_end;
void* data_section_start;
void* data_section_end;
void* caldata_section_start;
void* caldata_section_end;

// global variable that holds the server port value
int server_port;
// global variable that holds the calibration support
int is_calibration_used;
// global variable that holds the calibration data used from a file
int is_calibration_data_from_file;
// global variable that holds the filepath for the calibration data
char calibration_data_filepath[CALIBRATION_FILE_PATH_SIZE];
// global variable that holds the filepath for the calibration data
char checksum_filepath[CALIBRATION_FILE_PATH_SIZE];

// logging
void* user_data;
int log_level;

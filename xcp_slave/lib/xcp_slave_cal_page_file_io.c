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

#include "xcp_slave_cal_page_file_io.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "xcp_globals.h"
#include "xcp_slave_log.h"

//********* Static Constants* ****************************************************************************************
static const uint16_t Crc16CcittTable[] = {
        0x0000U, 0x1021U, 0x2042U, 0x3063U, 0x4084U, 0x50A5U, 0x60C6U, 0x70E7U, 0x8108U, 0x9129U, 0xA14AU, 0xB16BU,
        0xC18CU, 0xD1ADU, 0xE1CEU, 0xF1EFU, 0x1231U, 0x0210U, 0x3273U, 0x2252U, 0x52B5U, 0x4294U, 0x72F7U, 0x62D6U,
        0x9339U, 0x8318U, 0xB37BU, 0xA35AU, 0xD3BDU, 0xC39CU, 0xF3FFU, 0xE3DEU, 0x2462U, 0x3443U, 0x0420U, 0x1401U,
        0x64E6U, 0x74C7U, 0x44A4U, 0x5485U, 0xA56AU, 0xB54BU, 0x8528U, 0x9509U, 0xE5EEU, 0xF5CFU, 0xC5ACU, 0xD58DU,
        0x3653U, 0x2672U, 0x1611U, 0x0630U, 0x76D7U, 0x66F6U, 0x5695U, 0x46B4U, 0xB75BU, 0xA77AU, 0x9719U, 0x8738U,
        0xF7DFU, 0xE7FEU, 0xD79DU, 0xC7BCU, 0x48C4U, 0x58E5U, 0x6886U, 0x78A7U, 0x0840U, 0x1861U, 0x2802U, 0x3823U,
        0xC9CCU, 0xD9EDU, 0xE98EU, 0xF9AFU, 0x8948U, 0x9969U, 0xA90AU, 0xB92BU, 0x5AF5U, 0x4AD4U, 0x7AB7U, 0x6A96U,
        0x1A71U, 0x0A50U, 0x3A33U, 0x2A12U, 0xDBFDU, 0xCBDCU, 0xFBBFU, 0xEB9EU, 0x9B79U, 0x8B58U, 0xBB3BU, 0xAB1AU,
        0x6CA6U, 0x7C87U, 0x4CE4U, 0x5CC5U, 0x2C22U, 0x3C03U, 0x0C60U, 0x1C41U, 0xEDAEU, 0xFD8FU, 0xCDECU, 0xDDCDU,
        0xAD2AU, 0xBD0BU, 0x8D68U, 0x9D49U, 0x7E97U, 0x6EB6U, 0x5ED5U, 0x4EF4U, 0x3E13U, 0x2E32U, 0x1E51U, 0x0E70U,
        0xFF9FU, 0xEFBEU, 0xDFDDU, 0xCFFCU, 0xBF1BU, 0xAF3AU, 0x9F59U, 0x8F78U, 0x9188U, 0x81A9U, 0xB1CAU, 0xA1EBU,
        0xD10CU, 0xC12DU, 0xF14EU, 0xE16FU, 0x1080U, 0x00A1U, 0x30C2U, 0x20E3U, 0x5004U, 0x4025U, 0x7046U, 0x6067U,
        0x83B9U, 0x9398U, 0xA3FBU, 0xB3DAU, 0xC33DU, 0xD31CU, 0xE37FU, 0xF35EU, 0x02B1U, 0x1290U, 0x22F3U, 0x32D2U,
        0x4235U, 0x5214U, 0x6277U, 0x7256U, 0xB5EAU, 0xA5CBU, 0x95A8U, 0x8589U, 0xF56EU, 0xE54FU, 0xD52CU, 0xC50DU,
        0x34E2U, 0x24C3U, 0x14A0U, 0x0481U, 0x7466U, 0x6447U, 0x5424U, 0x4405U, 0xA7DBU, 0xB7FAU, 0x8799U, 0x97B8U,
        0xE75FU, 0xF77EU, 0xC71DU, 0xD73CU, 0x26D3U, 0x36F2U, 0x0691U, 0x16B0U, 0x6657U, 0x7676U, 0x4615U, 0x5634U,
        0xD94CU, 0xC96DU, 0xF90EU, 0xE92FU, 0x99C8U, 0x89E9U, 0xB98AU, 0xA9ABU, 0x5844U, 0x4865U, 0x7806U, 0x6827U,
        0x18C0U, 0x08E1U, 0x3882U, 0x28A3U, 0xCB7DU, 0xDB5CU, 0xEB3FU, 0xFB1EU, 0x8BF9U, 0x9BD8U, 0xABBBU, 0xBB9AU,
        0x4A75U, 0x5A54U, 0x6A37U, 0x7A16U, 0x0AF1U, 0x1AD0U, 0x2AB3U, 0x3A92U, 0xFD2EU, 0xED0FU, 0xDD6CU, 0xCD4DU,
        0xBDAAU, 0xAD8BU, 0x9DE8U, 0x8DC9U, 0x7C26U, 0x6C07U, 0x5C64U, 0x4C45U, 0x3CA2U, 0x2C83U, 0x1CE0U, 0x0CC1U,
        0xEF1FU, 0xFF3EU, 0xCF5DU, 0xDF7CU, 0xAF9BU, 0xBFBAU, 0x8FD9U, 0x9FF8U, 0x6E17U, 0x7E36U, 0x4E55U, 0x5E74U,
        0x2E93U, 0x3EB2U, 0x0ED1U, 0x1EF0U};

#define MAX_CHARS_IN_LINE 255
#define DATA_SEGMENT "Data Checksum"
#define CODE_SEGMENT "Code Checksum"

const char* get_wp_file_name(void) {
    static char wp_file_name_buffer[128];
    snprintf(wp_file_name_buffer, sizeof(wp_file_name_buffer) - 1, "/var/working_page_%d.hex", server_port);
    return wp_file_name_buffer;
}

const char* get_rp_file_name(void) {
    static char rp_file_name_buffer[128];
    snprintf(rp_file_name_buffer, sizeof(rp_file_name_buffer) - 1, "/var/reference_page_%d.hex", server_port);
    return rp_file_name_buffer;
}

bool get_line(FILE* ptr, char* line) {
    char ch;
    memset(line, 0, MAX_CHARS_IN_LINE);
    while (!feof(ptr)) {
        ch = fgetc(ptr);
        *(line++) = ch;
        if (ch == '\n') break;
    }

    return 1;
}

bool get_word(const char* line, char* str, int index) {
    char* token = strtok((char*)line, "=");
    int count = 0;
    bool is_word_found = 0;

    while (token != NULL) {
        count++;
        if (count == index) {
            strncpy(str, token, MAX_CHARS_IN_LINE - 1);
            is_word_found = 1;
            break;
        }
        token = strtok(NULL, " ");
    }
    return is_word_found;
}

// bool retain_only_digits(char *str){
//     while((*str) != NULL)
//     {
//         if((*str) < '0' || (*str) > '9'){
//             (*str) = NULL;
//             break;
//         }
//         str++;
//     }

//     return true;
// }

void get_selected_segment_data(uint8_t* dest, uint8_t* src, int start_index, int size) {
    int src_index = start_index + size;
    int dest_index = size;

    while (src_index-- > start_index) {
        *(dest + (--dest_index)) = *(src + src_index);
    }
}

/**
 * @brief Reads calibration data from file
 *
 * @param data Buffer to write read data to
 * @param size Buffer size
 * @param file File to read
 * @param successMessage Log message in case of success
 * @param failureMessage Log message in case of failure
 * @return true Read succeeded
 * @return false Read failed.
 */
static bool XcpSlaveReadCalPageFromFile(uint8_t* data,
                                        const size_t size,
                                        const char* file,
                                        const char* successMessage,
                                        const char* failureMessage) {
    struct stat statbuf;
    size_t i = 0;
    int ch;
    bool success = true;
    FILE* const pFile = fopen(file, "rb"); /* polyspace DEFECT:INAPPROPRIATE_IO_ON_DEVICE [Not a
                                              defect:Unset] "Mathworks website recommends using stat to
                                              check that one is opening a regular file before fopen but
                                              it also acknowledges that their solution introduces a
                                              TOCTOU. Here we address that by checking after opening the
                                              file but that pattern isn't recognized by polyspace" */
    if (pFile == NULL) {
        success = false;
        XcpSlavePrintfVerbose("Failed to open %s", file);
    }
    // Now check file properties. Order is important to avoid TOCTOU
    if ((success == true) && (stat(file, &statbuf) != 0)) {
        success = false;
        XcpSlavePrintfVerbose("Failed to stat %s", file);
    }
    // Stat succeeded.
    if ((success == true) && (S_ISREG(statbuf.st_mode) != true)) {
        success = false;
        XcpSlavePrintfVerbose("%s isn't a regular file", file);
    }

    if (success == true) {
        /* We get a core dump with "buffer overflow" if we use fread here. Don't
         * know why. Hence the loop". */
        for (i = 0; i < size; i++) {
            ch = fgetc(pFile);
            if (ch == EOF) {
                success = false;
                XcpSlavePrintfVerbose("Failed to read from %s", file);
                break;
            } else {
                data[i] = (uint8_t)ch;
            }
        }
    }

    if ((pFile != NULL) && fclose(pFile) == EOF) {
        success = false;
        XcpSlavePrintfVerbose("Failed to close %s", file);
    }

    if (success == true) {
        (void)successMessage;  // If verbose output is defined off the compiler will
                               // complain about unused parameter.
        XcpSlavePrintfVerbose("%s", successMessage);
    } else {
        (void)failureMessage;  // If verbose output is defined off the compiler will
                               // complain about unused parameter.
        XcpSlavePrintfVerbose("%s", failureMessage);
    }

    return success;
}

/**
 * @brief Reads calibration data from file
 *
 * @param data Buffer to write read data to
 * @param size Buffer size
 * @param file File to Write
 * @param successMessage Log message in case of success
 * @param failureMessage Log message in case of failure
 * @return true Write succeeded
 * @return false Write failed.
 */
static bool XcpSlaveWriteCalPageToFile(const uint8_t* data,
                                       const size_t size,
                                       const char* file,
                                       char* successMessage,
                                       char* failureMessage) {
    struct stat statbuf;
    bool success = true;
    FILE* const pFile = fopen(file, "wb"); /* polyspace DEFECT:INAPPROPRIATE_IO_ON_DEVICE [Not a
                                              defect:Unset] "Mathworks website recommends using stat to
                                              check that one is opening a regular file before fopen but
                                              it also acknowledges that their solution introduces a
                                              TOCTOU. Here we address that by checking after opening the
                                              file but that pattern isn't recognized by polyspace" */
    if (pFile == NULL) {
        success = false;
        XcpSlavePrintfVerbose("Failed to create %s", file);
    }
    // Now check file properties. Order is important to avoid TOCTOU
    if ((success == true) && (stat(file, &statbuf) != 0)) {
        success = false;
        XcpSlavePrintfVerbose("Failed to stat %s", file);
    }
    // Stat succeeded.
    if ((success == true) && (S_ISREG(statbuf.st_mode) != true)) {
        success = false;
        XcpSlavePrintfVerbose("%s isn't a regular file", file);
    }
    if ((success == true) && (fwrite(data, 1, size, pFile) != size)) {
        success = false;
        XcpSlavePrintfVerbose("Failed to write to %s", file);
    }

    if ((pFile != NULL) && fclose(pFile) == EOF) {
        success = false;
        XcpSlavePrintfVerbose("Failed to close %s", file);
    }

    if (success == true) {
        XcpSlavePrintfVerbose("%s", successMessage);
    } else {
        XcpSlavePrintfVerbose("%s", failureMessage);
    }

    return success;
}

bool XcpSlaveWritCalToRpFile(void) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    return XcpSlaveWriteCalPageToFile((const uint8_t*)caldata_section_start,
                                      CalPageLength,
                                      get_rp_file_name(),
                                      "Calibration written to reference page file",
                                      "Failed to write calibration to reference page file");
}

bool XcpSlaveReadCalPageFromExternalFile(void) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    char successs_msg[300];
    sprintf(successs_msg, "Calibration loaded from external file @ \"%s\"", calibration_data_filepath);
    return XcpSlaveReadCalPageFromFile((uint8_t*)caldata_section_start,
                                       CalPageLength,
                                       calibration_data_filepath,
                                       successs_msg,
                                       "Failed to load calibration from external file");
}

bool XcpSlaveWritCalToWpFile(void) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    return XcpSlaveWriteCalPageToFile((const uint8_t*)caldata_section_start,
                                      CalPageLength,
                                      get_wp_file_name(),
                                      "Calibration written to working page file",
                                      "Failed to write calibration to working page file");
}

bool XcpSlaveReadCalFromRpFile(void) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    return XcpSlaveReadCalPageFromFile((uint8_t*)caldata_section_start,
                                       CalPageLength,
                                       get_rp_file_name(),
                                       "Calibration loaded from reference page file",
                                       "Failed to load calibration from reference page file");
}

bool XcpSlaveReadCalFromWpFile(void) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    return XcpSlaveReadCalPageFromFile((uint8_t*)caldata_section_start,
                                       CalPageLength,
                                       get_wp_file_name(),
                                       "Calibration loaded from working page file",
                                       "Failed to load calibration from working page file");
}

bool XcpSlaveIsPageFileValid(const char* file) {
    const uint32_t CalPageLength =
            (const uint8_t*)caldata_section_end - (const uint8_t*)caldata_section_start + 1; /* polyspace
                        DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These are from the linker => will be correct" */
    struct stat statbuf;
    FILE* pFile;

    bool isValid = true;

    // Check that we can open the file
    pFile = fopen(file, "rb"); /* polyspace DEFECT:INAPPROPRIATE_IO_ON_DEVICE [Not a
                                  defect:Unset] "Mathworks website recommends using stat to
                                  check that one is opening a regular file before fopen but
                                  it also acknowledges that their solution introduces a
                                  TOCTOU. Here we address that by checking after opening the
                                  file but that pattern isn't recognized by polyspace" */
    if (pFile == NULL) {
        isValid = false;
    }

    // Now check file properties. Order is important to avoid TOCTOU
    if ((isValid == true) && (stat(file, &statbuf) != 0)) {
        isValid = false;  // stat failed
    }
    // stat succeeded, proceed with next check (file size)
    if ((isValid == true) && (statbuf.st_size != CalPageLength)) {
        isValid = false;  // wrong size
    }

    if ((isValid == true) && (S_ISREG(statbuf.st_mode) != true)) {
        isValid = false;  // Not a regular file
    }

    if ((pFile != NULL) && (fclose(pFile) == EOF)) {
        isValid = false;  // Failed to close it again - something is fishy
    }

    return isValid;
}

bool XcpSlaveValidateExternalCalData(const char* cal_file, const char* chechsum_file) {
    bool isCalDataValid = false;
    uint32_t calDataLen = 0;
    uint16_t code_checksum, data_checksum, computed_data_checksum, computed_code_checksum;
    uint8_t caldata[0x000FFFFF] = {0};

    isCalDataValid =
            XcpSlaveIsPageFileValid(cal_file) && XcpSlaveGetChecksums(chechsum_file, &data_checksum, &code_checksum) &&
            XcpSlaveGetCalData(cal_file, caldata, &calDataLen) &&
            XcpSlaveComputeChecksums(caldata, calDataLen, &computed_data_checksum) &&
            XcpSlaveComputeChecksums(text_section_start,
                                     ((const uint8_t*)text_section_end - (const uint8_t*)text_section_start + 1),
                                     &computed_code_checksum) &&
            (code_checksum == computed_code_checksum) && (data_checksum == computed_data_checksum);

    return isCalDataValid;
}

bool XcpSlaveGetChecksums(const char* filepath, uint16_t* data_checksum, uint16_t* code_checksum) {
    bool isValid = false;

    FILE* ptr;
    ptr = fopen(filepath, "r");

    if (NULL == ptr) {
        XcpSlavePrintfVerbose("file can't be opened \n FILE : %s", filepath);
    }

    char line[MAX_CHARS_IN_LINE] = "\0";
    *code_checksum = 0;
    *data_checksum = 0;
    int temp_num = 0;
    while (!feof(ptr)) {
        char temp_str[MAX_CHARS_IN_LINE] = {0};
        get_line(ptr, line);
        if (strstr(line, DATA_SEGMENT)) {
            get_word(line, temp_str, 2);
            // retain_only_digits(temp_str);
            temp_num = 0;
            sscanf(temp_str, "%d", &temp_num);
            *data_checksum = (uint16_t)temp_num;
        }
        if (strstr(line, CODE_SEGMENT)) {
            get_word(line, temp_str, 2);
            // retain_only_digits(temp_str);
            temp_num = 0;
            sscanf(temp_str, "%d", &temp_num);
            *code_checksum = (uint16_t)temp_num;
            isValid = true;
            break;
        }
    }
    fclose(ptr);

    if (!isValid) {
        XcpSlavePrintfVerbose("%s file has no proper checksum data.");
    }

    return isValid;
}

bool XcpSlaveGetCalData(const char* filepath, uint8_t* caldata, uint32_t* caldata_len) {
    bool isValid = false;

    FILE* ptr;
    ptr = fopen(filepath, "r");

    if (NULL == ptr) {
        XcpSlavePrintfVerbose("file can't be opened \n FILE : %s", filepath);
    }
    *caldata_len = 0;
    while (!feof(ptr)) {
        *caldata++ = fgetc(ptr);
        (*caldata_len)++;
    }
    fclose(ptr);

    if (*caldata_len > 0) {
        isValid = true;
        (*caldata_len)--;
    }

    if (!isValid) {
        XcpSlavePrintfVerbose("%s file has no proper calibration data.");
    }

    return isValid;
}

/**
 * @brief Calculate crc16 ccitt checksum on the memory block stated by the BUILD_CHECKSUM request
 *
 * @param block_start_address Memory address of first byte in block to calculate checksum for
 * @param block_length Length of the block to calculate checksum for
 * @return uint16_t Calculated checksum
 */
static uint16_t CalChecksum(uint8_t* block_start_address, uint32_t block_length) {
    uint32_t i;
    uint16_t crc = 0xFFFFU;
    uint8_t temp;

    for (i = 0U; i < block_length; i++) {
        temp = (0xFFU) & (*block_start_address);  // Mask is just to avoid polyspace warning
        block_start_address++;
        const uint8_t tmp = (uint8_t)((crc >> 8U) ^ temp);
        crc = (uint16_t)(((crc << 8U) ^ Crc16CcittTable[tmp]) & 0xFFFFU);  // Mask is just to avoid polyspace warning
    }
    return crc;
}

bool XcpSlaveComputeChecksums(uint8_t* memory_segment, uint32_t segment_len, uint16_t* computed_data_checksum) {
    bool isValid = true;

    uint16_t iteration_block_size = 0x4000;
    uint16_t complete_block_iterations = segment_len / iteration_block_size;
    uint16_t remaining_iteration_length = segment_len % iteration_block_size;

    uint8_t segment_data[0x4000] = {0};

    int block_index = 0;
    uint32_t crc = 0;

    for (int i = 0; i < complete_block_iterations; i++) {
        get_selected_segment_data(segment_data, memory_segment, block_index, iteration_block_size);
        crc += CalChecksum(segment_data, iteration_block_size);
        block_index += iteration_block_size;
    }

    if (remaining_iteration_length > 0) {
        get_selected_segment_data(segment_data, memory_segment, block_index, remaining_iteration_length);
        crc += CalChecksum(segment_data, remaining_iteration_length);
    }

    (*computed_data_checksum) = crc % 0xFFFF;

    return isValid;
}
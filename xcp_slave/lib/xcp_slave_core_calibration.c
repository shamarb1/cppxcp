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

#include "xcp_slave_core_calibration.h"

#include <unistd.h>

#include "xcp_globals.h"
#include "xcp_slave_cal_page_file_io.h"
#include "xcp_slave_log.h"

#define CC_SET_REQUEST 0xF9U

// Calibration XCP Commands
#define CC_DOWNLOAD 0xF0U

// Page switching XCP Commands (PAG)
#define CC_SET_CAL_PAGE 0xEBU
#define CC_GET_CAL_PAGE 0xEAU
#define CC_GET_PAG_PROCESSOR_INFO 0xE9U
#define CC_SET_SEGMENT_MODE 0xE6U
#define CC_GET_SEGMENT_MODE 0xE5U
#define CC_COPY_CAL_PAGE 0xE4U

// Calibration control related defines
#define WORKING_PAGE 1U
#define REFERENCE_PAGE 0U
#define COPY_REF_PAGE_TO_WORK_PAGE 0U
#define COPY_WORK_PAGE_TO_REF_PAGE 1U

// GET_PAG_PROCESSOR_INFO related defines
#define MAX_SEGMENTS 1U    // There is one calibration segment in the application
#define PAG_PROPERTIES 1U  // The segment is possible to freeze

//********* Static Global variables* *********************************************************************************
static uint32_t CalPageLength;       // The length of the calibration page (&CalPageStart -&CalPageEnd)
static uint8_t CurrCalPage;          // Which of the calibration pages that is currently in use (working or reference)
static uint8_t OldCalPage;           // The calibration page used prior to the current one (working or reference)
static uint8_t GetSegmentMode = 0U;  // Indicate mode (enable/disable FREEZE) the SEGMENT_NUMBER 0 (CALPAGE)
static bool StoreCalAct = false;     // True if calibration store process is in progress

static uint64_t ProcBaseAddr;

bool XcpSlaveCoreIsBlockInCalData(const void* block_start, const uint32_t block_size) {
    return XcpSlaveCoreIsBlockInArea(caldata_section_start, caldata_section_end, block_start, block_size);
}

/**
 * @brief Copy contents of one calibration to another (ReferencePage <=> WorkingPage)
 *
 * @param page COPY_REF_PAGE_TO_WORK_PAGE or COPY_WORK_PAGE_TO_REF_PAGE
 */
static void XcpSlaveCopyCalPage(uint8_t page) {
    if ((WORKING_PAGE == CurrCalPage) && (COPY_REF_PAGE_TO_WORK_PAGE == page)) {
        // Read from reference page file to working page file and memory segment
        if (XcpSlaveReadCalFromRpFile() && XcpSlaveWritCalToWpFile()) {
            XcpSlavePrintfVerbose("Copy reference page to working page succeded");
        } else {
            XcpSlavePrintfVerbose("Copy reference page to working page failed");
        }
    } else if ((REFERENCE_PAGE == CurrCalPage) && (COPY_WORK_PAGE_TO_REF_PAGE == page)) {
        // Write from working page file to reference page file and memory segment
        if (XcpSlaveReadCalFromWpFile() && XcpSlaveWritCalToRpFile()) {
            XcpSlavePrintfVerbose("Copy working page to reference page succeded");
        } else {
            XcpSlavePrintfVerbose("Copy working page to reference page failed");
        }
    } else {
        // Copy from memory segment to both files
        XcpSlaveWritCalToWpFile();
        XcpSlaveWritCalToRpFile();
    }
}

bool XcpExecuteMd5sum(char* appName, char* fileName) {
    bool success = true;
    char cmd[1024] = {0};
    sprintf(cmd, "%s %s %s %s", "/usr/bin/md5sum", appName, ">", fileName);
    XcpSlavePrintfVerbose("Command formed to execute md5sum file is %s", cmd);

    FILE* in = popen(cmd, "r");
    if (!in) {
        XcpSlavePrintfVerbose("Error in execution of md5sum command");
        success = false;
    } else {
        if (pclose(in) == -1) {
            XcpSlavePrintfVerbose("Error in pclose of md5sum command");
            success = false;
        }
    }
    return success;
}
bool XcpCompareFiles(char* file1, char* file2) {
    bool equalFiles = true;
    FILE* fp1 = fopen(file1, "r");
    FILE* fp2 = fopen(file2, "r");
    if ((fp1 == NULL) || (fp2 == NULL)) {
        equalFiles = false;
    } else {
        signed char ch1 = getc(fp1);
        signed char ch2 = getc(fp2);
        int i = 0;
        while (ch1 != EOF && ch2 != EOF && i < 32) {
            if (ch1 != ch2) {
                XcpSlavePrintfVerbose("MD5Sum not matching at the point %c and %c ", ch1, ch2);
                equalFiles = false;
                break;
            }
            i++;
            ch1 = getc(fp1);
            ch2 = getc(fp2);
        }
    }
    if (fp1 != NULL) {
        fclose(fp1);
    }
    if (fp2 != NULL) {
        fclose(fp2);
    }
    return equalFiles;
}
/**
 * @brief To check if the application binary is been modified ,so we understand if the device is been flashed.
 * Fix for ARTVDD-10415 - HPAA apps does not use calibration data from DSA flash if flashed with INCA before
 *
 * @param event number stated by EVENT_CHANNEL_NUMBER in the corresponding EVENT block in the a2l-file
 */
bool XcpCheckApphash() {
    bool isAppHashValid = false;
    char binaryName[512] = {0};

    // Get the current application name
    ssize_t len = readlink("/proc/self/exe", binaryName, sizeof(binaryName) - 1);
    if (len != -1) binaryName[len] = '\0';

    if (binaryName[0] == '\0') return isAppHashValid;

    XcpSlavePrintfVerbose("Application name is %s", binaryName);

    // md5sum of the application
    // creating tmp file and calculating the md5sum of the application.
    char md5FileTmp[128];
    snprintf(md5FileTmp, sizeof(md5FileTmp) - 1, "/var/md5_file_tmp%d.txt", server_port);
    bool executedMd5sumTmp = XcpExecuteMd5sum(binaryName, md5FileTmp);
    XcpSlavePrintfVerbose("Return value of executedMd5sumTmp is %d", executedMd5sumTmp);

    char md5file[128];
    snprintf(md5file, sizeof(md5file) - 1, "/var/md5_file%d.txt", server_port);
    // Compare the old md5sum and the current one
    bool fileCompare = XcpCompareFiles(md5FileTmp, md5file);
    XcpSlavePrintfVerbose("Md5sum Files compared value is %d", fileCompare);
    if (executedMd5sumTmp && fileCompare) {
        XcpSlavePrintfVerbose("Application hash is matching so there is no change in app binary");
        isAppHashValid = true;
    } else {
        // Create the new md5sum file of the appln binary
        const bool executedMd5sum = XcpExecuteMd5sum(binaryName, md5file);
        XcpSlavePrintfVerbose("Value of executed md5sum on device is %d", executedMd5sum);
    }
    XcpSlavePrintfVerbose(remove(md5FileTmp) ? "Error : Failed to remove md5 tmp file"
                                             : "Temporarily md5 file removed successfully");
    return isAppHashValid;
}

void XcpSlaveCoreCalSupportInit(const uint64_t ProcBaseAddr_) {
    ProcBaseAddr = ProcBaseAddr_;

    CalPageLength =
            (uint32_t)((uint64_t)caldata_section_end - (uint64_t)caldata_section_start);  // Max cal page size: 4GB
    XcpSlavePrintfVerbose("Cal page length: %u", CalPageLength);

    CurrCalPage = REFERENCE_PAGE;  // Current calibration page
    OldCalPage = REFERENCE_PAGE;   // Previous calibration page

    if (is_calibration_used) {
        const bool referencePageFileValid = XcpSlaveIsPageFileValid(get_rp_file_name());
        const bool isAppHashValid = XcpCheckApphash();
        if (is_calibration_data_from_file || !referencePageFileValid || !XCP_INIT_WITH_EXISTING_CAL_FILES ||
            !isAppHashValid) {
            // reference page file does not exist or has the wrong size => create new files or overwrite old
            XcpSlavePrintfVerbose("Reference and working files will be created");
            XcpSlaveWritCalToWpFile();
            XcpSlaveWritCalToRpFile();
        } else {
            // so, copy data from file to RAM
            XcpSlavePrintfVerbose("Existing reference page file will be used");
            XcpSlaveReadCalFromRpFile();
            XcpSlaveWritCalToWpFile();
        }
    }
}

void XcpSlaveCoreProcessCalCommand(const uint8_t command_buf[],
                                   struct XcpSlaveCoreClient* pclient,
                                   uint8_t* const responseBuf,
                                   uint16_t* responseSize) {
    uint32_t i;  // For use in loops
    uint8_t* ptemp_addr;

    switch (command_buf[0]) {
        case CC_DOWNLOAD: {
            XcpSlavePrintfVerbose("DOWNLOAD");
            // Calibration and page switch access is granted if unlocked via Seed&Key
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                const uint8_t bytes_to_download = command_buf[1];
                ptemp_addr = (uint8_t*)(ProcBaseAddr + pclient->mem_trans_addr);
                // Check if MTA address including bytes_to_download is within DATA_PAGE boundaries
                if (XcpSlaveCoreIsBlockInCalData(ptemp_addr, bytes_to_download)) {
                    if (bytes_to_download <= (XCP_MAX_CTO - DOWNLOAD_REQ_HEADER_LEN)) {
                        for (i = 0U; i < (uint32_t)bytes_to_download; i++) {
                            *ptemp_addr = command_buf[DOWNLOAD_REQ_HEADER_LEN + i];
                            ptemp_addr++;
                            pclient->mem_trans_addr++;
                        }
                        *responseSize = 1;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;

                    }
                    // too much data in message
                    else {
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, responseSize);
                        XcpSlavePrintfVerbose("ERR_OUT_OF_RANGE");
                    }
                }
                // XCP client tries to access an address outside calibration page or too much data in message
                else {
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, responseSize);
                    XcpSlavePrintfVerbose("ERR_ACCESS_DENIED");
                }
            } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        case CC_UPLOAD: {
            XcpSlavePrintfVerbose("UPLOAD");
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                const uint8_t bytes_to_upload = command_buf[1];
                ptemp_addr = (uint8_t*)(ProcBaseAddr + pclient->mem_trans_addr);
                if (XcpSlaveCoreIsBlockInCalData(ptemp_addr, bytes_to_upload)) {
                    if (bytes_to_upload <= (XCP_MAX_CTO - UPLOAD_RES_HEADER_LEN)) {
                        *responseSize = bytes_to_upload + UPLOAD_RES_HEADER_LEN;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                        for (i = 0U; i < (uint32_t)bytes_to_upload; i++) {
                            responseBuf[UPLOAD_RES_HEADER_LEN + i] = *ptemp_addr;
                            ptemp_addr++;
                            pclient->mem_trans_addr++;
                        }
                    } else {
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, responseSize);
                        XcpSlavePrintfVerbose("ERR_OUT_OF_RANGE");
                    }
                }
                // XCP client tries to access an address outside calibration page
                else {
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, responseSize);
                    XcpSlavePrintfVerbose("ERR_ACCESS_DENIED");
                }
            } else {  // calibration (CAL/PAG) or measurement (DAQ) access has not been unlocked
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        case CC_SET_CAL_PAGE: {
            // Calibration and page switch access is granted if unlocked via Seed&Key
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                XcpSlavePrintfVerbose("SET_CAL_PAGE");
                CurrCalPage = command_buf[3];
                if ((REFERENCE_PAGE == CurrCalPage) && (WORKING_PAGE == OldCalPage)) {
                    if (XcpSlaveWritCalToWpFile() && XcpSlaveReadCalFromRpFile()) {
                        XcpSlavePrintfVerbose("Calibration switched from working page to reference page");
                    } else {
                        XcpSlavePrintfVerbose(
                                "Calibration switch from working page to "
                                "reference page failed");
                    }
                } else if ((WORKING_PAGE == CurrCalPage) && (REFERENCE_PAGE == OldCalPage)) {
                    if (XcpSlaveReadCalFromWpFile()) {
                        XcpSlavePrintfVerbose("Calibration switched from reference page to working page");
                    } else {
                        XcpSlavePrintfVerbose(
                                "Calibration switch from reference page "
                                "to working page failed");
                    }
                }
                OldCalPage = CurrCalPage;
                *responseSize = 1;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
            } else {  // Seed & Key routine is not performed or key is wrong so calibration access is not permitted
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        case CC_GET_CAL_PAGE: {
            XcpSlavePrintfVerbose("CC_GET_CAL_PAGE");
            // Calibration and page switch access is granted if unlocked via Seed&Key
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                XcpSlavePrintfVerbose("GET_CAL_PAGE");
                *responseSize = 4;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] = RESERVED_BYTE;
                responseBuf[2] = RESERVED_BYTE;
                responseBuf[3] = CurrCalPage;
            } else {  // Seed & Key routine is not performed or key is wrong so calibration access is not permitted
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        case CC_GET_PAG_PROCESSOR_INFO: {
            XcpSlavePrintfVerbose("GET_PAG_PROCESSOR_INFO");
            *responseSize = 3;
            responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
            responseBuf[1] = MAX_SEGMENTS;    // This app has one cal segement
            responseBuf[2] = PAG_PROPERTIES;  // The cal segment can be freezed
        } break;
        case CC_SET_SEGMENT_MODE: {
            XcpSlavePrintfVerbose("SET_SEGMENT_MODE");
            // Calibration and page switch access is granted if unlocked via Seed&Key
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                /*The CALRAM section DataA (only one CALRAM section in example a2l-file)
                 * has SEGMENT_NUMBER 0*/
                if (0U == command_buf[2]) {
                    GetSegmentMode = command_buf[1];  // 0: disable FREEZE mode, 1: enable freeze mode
                    *responseSize = 1;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                }
                // Only SEGMENT_NUMBER 0 is supported
                else {
                    XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, responseSize);
                }
            } else {  // Seed & Key routine is not performed or key is wrong so calibration access is not permitted
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        case CC_GET_SEGMENT_MODE: {
            XcpSlavePrintfVerbose("GET_SEGMENT_MODE");
            if (0 == command_buf[2]) {  // Only SEGMENT_NUMBER 0 is supported for freezing
                *responseSize = 3;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] = RESERVED_BYTE;
                responseBuf[2] = GetSegmentMode;
            } else {
                XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, responseSize);
            }
        } break;
        case CC_SET_REQUEST: {
            XcpSlavePrintfVerbose("SET_REQUEST");
            if (1U == command_buf[1]) {  // STORE_CAL_REQ is requested (the only supported SET_REQUEST)
                // Calibration and page switch access is granted if unlocked via * Seed&Key
                if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                    StoreCalAct = true;  // Info needed if several XCP clients connected simultaneously
                    XcpSlaveCopyCalPage(COPY_WORK_PAGE_TO_REF_PAGE);
                    StoreCalAct = false;
                    // Freeze working data is commanded so client will be abandoned shortly, so indicate this
                    // to make the client position avalable for new clients
                    pclient->abandon_pending = true;
                    *responseSize = 1;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                } else {  // Seed & Key routine not performed or wrong key so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
                }
            } else {
                XcpSlavePrintfVerbose("SET_REQUEST mode not supported");
                XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, responseSize);
            }
        } break;
        case CC_COPY_CAL_PAGE: {  // Copy reference page to working page
            XcpSlavePrintfVerbose("COPY_CAL_PAGE");
            // Calibration and page switch access is granted if unlocked via Seed&Key
            if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status)) {
                XcpSlaveCopyCalPage(COPY_REF_PAGE_TO_WORK_PAGE);
                *responseSize = 1;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
            } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, responseSize);
            }
        } break;
        default:
            *responseSize = 0;
            break;
    }
}

bool XcpSlaveCoreIsCalibrationStoreInProgress() {
    return StoreCalAct;
}

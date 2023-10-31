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

#include "xcp_config.h"

// This file contains stuff needed by both xcp_core and xcp_core_calibration.c/h. If the calibration parts weren't in a
// separate file (to make them easy to omit) the contents of this file would be in xcp_core.h/c

#ifdef __cplusplus
extern "C" {
#endif

#define CC_UPLOAD 0xF5U

// General request related defines
#define RESERVED_BYTE 0x00U
#define POSITIVE_RESPONSE 0xFFU
#define NEGATIVE_RESPONSE 0xFEU
#define XCP_CMD_NR_BYTE_POS 0U

// Packet length related defines
#define SET_CAL_PAGE_MSG_LEN 8U
#define START_STOP_SYNCH_MSG_LEN 2U
#define WRITE_DAQ_MSG_LEN 8U
#define BUILD_CHECKSUM_MSG_LEN 8U
#define SET_MTA_MSG_LEN 8U
#define XCP_HEADER_LEN 4U  // Ethernet control field (LEN+CTR) which is 2 bytes each
#define MAX_RES_LEN_NOT_UPLOAD 8U
#define HEADER_PID_MSGCNTR_LEN 5U   // Size in bytes of header, pid and message counter
#define DOWNLOAD_REQ_HEADER_LEN 2U  // 2-byte "Command Code" + 1 byte "Nr of data elements"
#define UPLOAD_RES_HEADER_LEN 1U    // 1 byte "Positive Response"
#define ODT_PID_LEN 1U              // PID is a 1 byte code for XCP master to identify ODT list
#define TIMESTAMP_LEN 4U            // Size of time stamp sent in first ODT list of DAQ list

// XCP Error Codes
#define ERR_CMD_SYNCH 0x00U
#define ERR_CMD_UNKNOWN 0x20U
#define ERR_OUT_OF_RANGE 0x22U
#define ERR_ACCESS_DENIED 0x24U
#define ERR_ACCESS_LOCKED 0x25U
#define ERR_MODE_NOT_VALID 0x27U
#define ERR_MEMORY_OVERFLOW 0x30U
#define ERR_RESOURCE_TEMPORARY_NOT_ACCESSIBLE 0x33U

// Response message buffer index related defines
#define RESPONSE_TYPE_IDX 0U
#define ERROR_CODE_IDX 1U

// Writing and reading ints from char arrays
#define READ_UINT16_FROM_UINT8_ARRAY(buf) *(uint16_t*)buf
#define READ_UINT32_FROM_UINT8_ARRAY(buf) *(uint32_t*)buf
#define WRITE_UINT16_TO_UINT8_ARRAY(buf, x) *(uint16_t*)buf = x
#define WRITE_UINT32_TO_UINT8_ARRAY(buf, x) *(uint32_t*)buf = x

// Seed&KKey stuff
#define RESOURCE_BIT_CAL_PAG (0x01 << 0)  // Cal/Page access bit of AccessStatus
#define RESOURCE_BIT_DAQ (0x01 << 2)      // DAQ related access bit of AccessStatus
#define RESOURCE_BIT_STIM (0x01 << 3)
#define RESOURCE_BIT_PGM (0x01 << 4)
#define RESOURCE_BIT_DBG (0x01 << 5)

// A resource is selected in GET_SEED and UNLOCK if the corresponding bit is 1. These macros are defined here because
// they are used in the implementation and can be useful in customized xcp_slave_get_seed and xcp_slave_calc_key.
#define IS_RESOURCE_BIT_SET_ONLY_CAL_PAG(x) ((x) == RESOURCE_BIT_CAL_PAG)
#define IS_RESOURCE_BIT_SET_ONLY_DAQ(x) ((x) == RESOURCE_BIT_DAQ)
#define IS_RESOURCE_BIT_SET_ONLY_STIM(x) ((x) == RESOURCE_BIT_STIM)
#define IS_RESOURCE_BIT_SET_ONLY_PGM(x) ((x) == RESOURCE_BIT_PGM)
#define IS_RESOURCE_BIT_SET_ONLY_DBG(x) ((x) == RESOURCE_BIT_DBG)

// Seed & key
// A resource is unlocked if the corresponding bit in our internal status byte is 0.
#define RESOURCE_IS_UNLOCKED_CAL_PAG(x) (0 != ((~x) & RESOURCE_BIT_CAL_PAG))
#define RESOURCE_IS_UNLOCKED_DAQ(x) (0 != ((~x) & RESOURCE_BIT_DAQ))
#define RESOURCE_IS_UNLOCKED_STIM(x) (0 != ((~x) & RESOURCE_BIT_STIM))
#define RESOURCE_IS_UNLOCKED_PGM(x) (0 != ((~x) & RESOURCE_BIT_PGM))
#define RESOURCE_IS_UNLOCKED_DBG(x) (0 != ((~x) & RESOURCE_BIT_DBG))

// Debug related defines
#define DEBUG_DOWNLOAD_PRINT_LENGTH 25U
#define DEBUG_BYTE_PRINT_LENGTH 16U     // Number of bytes to print in each row
#define DEBUG_ADDRESS_PRINT_LENGTH 16U  // Number of addresses to print in each row

typedef int8_t ClientIdType;

struct XcpSlaveCoreClient {
    ClientIdType client_id;  // If client_id > XCP_MAX_NR_OF_CLIENTS, CONNECT request will be rejected. Signed to be
                             // able to return -1 to signal no client found.
    uint8_t connected;       // Indicates if client is connected (i.e. the first command needs to be CONNECT)
    uint32_t last_cmd_epoch_time;       // EPOCH time at the time for the last command reception
    uint64_t mem_trans_addr;            // Memory pointer
    uint8_t daq;                        // DAQ list number pointed to by SET_DAQ_PTR and used by WRITE_DAQ
    uint8_t odt;                        // ODT list number pointed to by SET_DAQ_PTR and used by WRITE_DAQ
    uint8_t first_odt;                  // TRUE if ODT list is the first in the DAQ list. Used by WRITE_DAQ
    uint8_t odt_entry;                  // ODT list entry pointed to by SET_DAQ_PTR_and used by WRITE_DAQ
    uint8_t curr_prot_status;           // Current protection status. If 0, all resources are unlocked
    uint8_t seed[XCP_SEED_KEY_LENGTH];  // Seed for access
    uint8_t act_seed_res;               // Which resource (CAL/PAG, DAQ etc.)that are currently handled
    uint8_t local_daq_lists[XCP_MAX_DAQ_LISTS_PER_CLIENT];     // Global DAQ list number
    bool local_daq_list_sel[XCP_MAX_DAQ_LISTS_PER_CLIENT];     // true if DAQ list is selected by START_STOP_DAQ_LIST
    bool local_daq_list_active[XCP_MAX_DAQ_LISTS_PER_CLIENT];  // true if DAQ list transmission is active
    // Which ODT list that is allocated to DAQ
    uint8_t local_odt_lists[XCP_MAX_DAQ_LISTS_PER_CLIENT][XCP_MAX_ODT_LISTS_PER_DAQ_LIST];
    bool abandon_pending;  // Client will be abandoned as "Calibration Data Page Freeze" has been performed
    bool disconnect_after_response;
};

typedef struct XcpSlaveCoreClient XcpSlaveCoreClient;

/**
 * @brief Determines whether a certain address is located in a memory area
 *
 * @param area_start Start address of area
 * @param area_end End address or area
 * @param addr Address to check
 * @return true
 * @return false
 */
bool XcpSlaveCoreIsAddrInArea(const void* area_start, const void* area_end, const void* addr);

/**
 * @brief Auxiliary function to if a memory block is entirely within an area
 *
 * @param area_start Start of area
 * @param area_end End of area
 * @param block_start Start address of memory block to check
 * @param block_size Block size
 * @return true Block is entierly within the area
 * @return false Block isn't entierly within the area
 */
bool XcpSlaveCoreIsBlockInArea(const void* area_start,
                               const void* area_end,
                               const void* block_start,
                               const uint32_t block_size);

/**
 * @brief Writes negative response.
 *
 * @param response_code Negative response code to write
 * @param responseBuf Buffer to write the response to.
 * @param responseSize Size of response in bytes. Set to zero in case the command wasn't recognized.
 */
void XcpSlaveCoreWriteNegativResponse(const uint8_t response_code, uint8_t* const responseBuf, uint16_t* responseSize);

#ifdef __cplusplus
}
#endif

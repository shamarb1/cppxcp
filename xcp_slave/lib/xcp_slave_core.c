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

#include "xcp_slave_core.h"

#include <stdio.h>
#include <string.h>

#include "xcp.h"
#include "xcp_globals.h"
#include "xcp_slave_cal_page_file_io.h"
#include "xcp_slave_core_calibration.h"
#include "xcp_slave_log.h"

// CONNECT related defines
#define RESOURCE_DAQ_SUPPORTED 4U  // Bit set indicating that DAQ resource is supported
#define ADDRESS_GRANULARITY 0U     //(0b00 = BYTE, 0b, 0b01 = WORD, 0b10 = DWORD)
#define BYTE_ORDER_XCP 0U          // intel (MSB on higher address) DO NOT ALTER
#define XCP_PROTOCOL_VERSION 1U    // XCP Protocol Layer Version Number (Only MSB)
#define XCP_PROTOCOL_TRANSPORT_LAYER 1U

// GET_COMM_MODE_INFO request related defines
#define COMM_MODE_OPTIONAL 0x00U  // Neither BLOCK_MODE or INTERLEAVED_MODE is supported
#define QUEUE_SIZE 0x00U          // Zero as neither BLOCK_MODE or INTERLEAVED_MODE is supported
#define MAX_BS 0x00U              // Zero as neither BLOCK_MODE or INTERLEAVED_MODE is supported
#define MIN_ST 0x00U              // Zero as neither BLOCK_MODE or INTERLEAVED_MODE is supported
#define XCP_DRIVER_VERSION_NUMBER 0x10U

// START_STOP_DAQ_LIST and START_STOP_SYNCH related defines
#define STOP_DAQ_LIST 0U
#define START_DAQ_LIST 1U
#define SELECT_DAQ_LIST 2U

#define STOP_ALL_DAQ_LISTS 0U
#define START_SEL_DAQ_LISTS 1U
#define STOP_SEL_DAQ_LISTS 2U

// SET_DAQ_LIST_MODE defines
#define FIRST_PID_IDX 1U

// GET_DAQ_PROCESSOR_INFO related defines
#define MAX_DAQ_LSB 0U  // Set to zero as dynamic DAQ allocation is used
#define MAX_DAQ_MSB 0U  // Set to zero as dynamic DAQ allocation is used
#define NR_OF_EVENTS_MSB 0U
#define MIN_DAQ 0U       // The number of predefined DAQ lists
#define DAQ_KEY_BYTE 0U  // DAQ key byte parameter bit mask structure

// GET_STATUS related defines
#define STATE_NUMBER 8U       // Not supported
#define SESSION_CONFIG_ID 0U  // Not supported

// UPLOAD / SHORT_UPLOAD related defines
#define UPLOAD_LENGTH_IDX 5U

// BUILD_CHECKSUM related defines
#define CHKSUM_TYPE_CRC16CITT 0x08U

// GET_DAQ_CLOCK related defines
#define TRIGGER_INITIATOR 0x07U  // 7 = reserved as this info is not used by XCP client in legacy mode
#define PAYLOAD_FMT 0x01U        // Format of XCP clock: "size of payload containing timestamp: DWORD"

// GET_DAQ_PROCESSOR_INFO related defines
#define DAQ_PROPERTIES 0x11U

// ALLOC_ODT_ENTRY related defines
#define DAQ_ODT_OR_ENTRY_OUT_OF_RANGE 0xFFU

// WRITE_DAQ related defines
#define FIRST_ODT_NR 0  // The first ODT list in each DAQ has number 0

// GET_DAQ_RESOLUTION_INFO related defines
#define GRANULARITY_ODT_ENTRY_SIZE_DAQ 1U  // Min size of ODT Entry is one byte in DAQ direction
#define MAX_VALUE_OF_UINT8 0xFFU           // Maximum value that will fit in a byte

#if XCP_MAX_DTO <= (ODT_PID_LEN + TIMESTAMP_LEN)
// To avoid negative values in the unlikely event that XCP_MAX_DTO can not accomodate any signal
#define MAX_ODT_ENTRIES_TOTAL 0U

#elif XCP_MAX_DTO - (ODT_PID_LEN + TIMESTAMP_LEN) < 0XFF
// Remaining bytes after ODT_PID and TIMESTAMP can not accomodate all 0xFF entries
// So allocate a maximum of XCP_MAX_DTO - (ODT_PID_LEN + TIMESTAMP_LEN) entries
#define MAX_ODT_ENTRIES_TOTAL XCP_MAX_DTO - (ODT_PID_LEN + TIMESTAMP_LEN)
#else
// Max ODT entries is limited to 255 by ASAM standard
#define MAX_ODT_ENTRIES_TOTAL 0XFFU
#endif

#define GRANULARITY_ODT_ENTRY_SIZE_STIM 1U  // Min size of ODT Entry is one byte in STIM direction
#define MAX_ODT_ENTRY_SIZE_STIM 0xFFU
#define TIMESTAMP_MODE 0x5CU    // timestamp clock length 4 bytes, timestamp unit: 100us
#define TIMESTAMP_TICKS_LSB 1U  // Number of timestamp ticks per timestamp unit LSB
#define TIMESTAMP_TICKS_MSB 0U  // Number of timestamp ticks per timestamp unit MSB

// Standard XCP Commands
#define CC_CONNECT 0xFFU
#define CC_DISCONNECT 0xFEU
#define CC_GET_STATUS 0xFDU
#define CC_SYNCH 0xFCU
#define CC_GET_COMM_MODE_INFO 0xFBU
#define CC_GET_SEED 0xF8U
#define CC_UNLOCK 0xF7U
#define CC_SET_MTA 0xF6U
#define CC_SHORT_UPLOAD 0xF4U
#define CC_BUILD_CHECKSUM 0xF3U

// DATA Acquisition and Stimulation XCP Commands (DAQ/STIM)
#define CC_SET_DAQ_PTR 0xE2U
#define CC_WRITE_DAQ 0xE1U
#define CC_SET_DAQ_LIST_MODE 0xE0U
#define CC_START_STOP_DAQ_LIST 0xDEU
#define CC_START_STOP_SYNCH 0xDDU
#define CC_GET_DAQ_CLOCK 0xDCU
#define CC_GET_DAQ_PROCESSOR_INFO 0xDAU
#define CC_GET_DAQ_RESOLUTION_INFO 0xD9U
#define CC_FREE_DAQ 0xD6U
#define CC_ALLOC_DAQ 0xD5U
#define CC_ALLOC_ODT 0xD4U
#define CC_ALLOC_ODT_ENTRY 0xD3U

// Memory resources for DAQ list related defines
#define MAX_NR_OF_DAQ_LISTS (XCP_MAX_NR_OF_CLIENTS * XCP_MAX_DAQ_LISTS_PER_CLIENT)
#define MAX_NR_OF_ODT_LISTS (MAX_NR_OF_DAQ_LISTS * XCP_MAX_ODT_LISTS_PER_DAQ_LIST)

// DAQ list assignment related defines
#define DAQ_LIST_FREE 0xFFU                 // Indication that DAQ list is unassigned
#define ODT_LIST_FREE 0xFFU                 // Indication that ODT list is unassigned
#define CLIENT_POS_FREE ((ClientIdType)-1)  // Indication that client position is unassigned
#define NO_EVENT 0xFFU                      // Indication that no event is assigned to DAQ list

// Measurement control related defines
#define STOP_ALL_DAQ_LISTS 0U
#define START_SELECTED_DAQ_LISTS 1U
#define STOP_SELECTED_DAQ_LISTS 2U

//********* Static Global variables* *********************************************************************************
static uint64_t ProcBaseAddr;  // RAM address where the app is loaded

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

//********** Data structure for global variables that are different for different clients ***************************
// A number of DAQ lists, ODT lists and ODT entries are available as a common resoruce for this XCP server:
static uint8_t daq_list_client[MAX_NR_OF_DAQ_LISTS];                             // Client DAQ list is allocated to
static uint8_t daq_list_event[MAX_NR_OF_DAQ_LISTS];                              // Event DAQ list is tied to
static uint8_t odt_list_client[MAX_NR_OF_ODT_LISTS];                             // Client ODT list is allocated to
static uint8_t odt_list_entries[MAX_NR_OF_ODT_LISTS];                            /*Nr of entries of ODT list (used
                                                                                    only for bounds check)*/
static uint16_t odt_list_byte_size[MAX_NR_OF_ODT_LISTS];                         // Total number of bytes in ODT list
static uint8_t odt_list_entry_byte[MAX_NR_OF_ODT_LISTS][MAX_ODT_ENTRIES_TOTAL];  // Nr of bytes of ODT list entry
static uint64_t odt_list_entry_address[MAX_NR_OF_ODT_LISTS][MAX_ODT_ENTRIES_TOTAL];  // Address of ODT list entry

/**
 * @brief Whether a memory block is in .text section
 *
 * @param block_start Start of memory block
 * @param block_size Block size.
 * @return true
 * @return false
 */
static bool IsBlockInText(const void* block_start, const uint32_t block_size) {
    return XcpSlaveCoreIsBlockInArea(text_section_start, text_section_end, block_start, block_size);
}

/**
 * @brief Whether a memory block is in .bss section
 *
 * @param block_start Start of memory block
 * @param block_size Block size.
 * @return true
 * @return false
 */
static bool IsBlockInBss(const void* block_start, const uint32_t block_size) {
    return XcpSlaveCoreIsBlockInArea(bss_section_start, bss_section_end, block_start, block_size);
}

/**
 * @brief Whether a memory block is in .data section
 *
 * @param block_start Start of memory block
 * @param block_size Block size.
 * @return true
 * @return false
 */
static bool IsBlockInData(const void* block_start, const uint32_t block_size) {
    return XcpSlaveCoreIsBlockInArea(data_section_start, data_section_end, block_start, block_size);
}

/**
 * @brief Allocate DAQ lists
 *
 * The global DAQ list array (daq_list_client) is searched for a free DAQ. Free DAQ lists has the value DAQ_LIST_FREE.
 * When a free DAQ list is found, its value is set to the client client_id and the DAQ list number is entered to the DAQ
 * list position in the local DAQ list for the client (local_daq_list)
 *
 * @param pclient Current client
 * @param daq_list_count Number of DAQ list to be allocated
 * @return uint8_t Number of DAQ lists allocate
 */
static uint8_t XcpSlaveCoreAllocDaq(struct XcpSlaveCoreClient* pclient, uint8_t daq_list_count) {
    uint8_t i;                         // For use in outer loop
    uint8_t j;                         // For use in inner loop
    uint8_t daq_lists_allocated = 0U;  // How many DAQ lists has been allocated
    // Make sure not to allocate more DAQ lists than allowed
    if (daq_list_count > XCP_MAX_DAQ_LISTS_PER_CLIENT) {
        daq_list_count = XCP_MAX_DAQ_LISTS_PER_CLIENT;
    }
    // Find the first free DAQ list which is indicated by DAQ_LIST_FREE (0xFF)
    for (i = 0U; i < daq_list_count; i++) {
        for (j = 0U; j < MAX_NR_OF_DAQ_LISTS; j++) {
            // If below condition is false, we have ran out of free DAQ lists
            if (DAQ_LIST_FREE == daq_list_client[j]) {
                daq_list_client[j] = pclient->client_id;  // DAQ list is free so pick it for current client
                pclient->local_daq_lists[i] = j;          // local DAQ list nr is set to the global DAQ list nr
                daq_lists_allocated = i + 1U;
                break;  // One break per loop is OK according to MISRA
            }
        }
    }
    return daq_lists_allocated; /*Return how many DAQ lists that were successfully
                                   allocated*/
}

/**
 * @brief Allocate ODT lists
 *
 * The global ODT list array (odt_list_client) is searched for a free ODT. Free  ODT lists has the value ODT_LIST_FREE.
 * When a free ODT_list is found, its value is set to the client client_id and the ODT list number is entered to the DAQ
 * and ODT list position in the local ODT list for* the client (local_odt_lists)
 *
 * @param pclient Current client
 * @param daq_list_nr DAQ list to allocate ODT lists for
 * @param odt_list_count Number of ODT list to be allocated in the selected DAQ
 * @return uint8_t Number of ODT lists allocated
 */
static uint8_t XcpSlaveCoreAllocOdt(struct XcpSlaveCoreClient* pclient, uint8_t daq_list_nr, uint8_t odt_list_count) {
    uint8_t i;  // For use in outer loop
    uint8_t j;  // For use in inner loop
    uint8_t odt_lists_allocated = 0U;
    // Make sure not to allocate more ODT lists than allowed per DAQ list
    if (odt_list_count > XCP_MAX_ODT_LISTS_PER_DAQ_LIST) {
        odt_list_count = XCP_MAX_ODT_LISTS_PER_DAQ_LIST;
    }
    // Find the first free ODT list in odt_list_client array
    for (i = 0U; i < odt_list_count; i++) {
        for (j = 0U; j < MAX_NR_OF_ODT_LISTS; j++) {
            // If below condition is false, we have ran out of free ODT lists
            if (ODT_LIST_FREE == odt_list_client[j]) {
                // Allocate the found ODT list by assigning the ODT list index the client id number
                odt_list_client[j] = pclient->client_id;
                XcpSlavePrintfVerbose("ODT list client_id: %02x", odt_list_client[j]);
                // set the local ODT list nr to the global ODT list nr
                pclient->local_odt_lists[daq_list_nr][i] = j;
                odt_lists_allocated = i + 1U;
                break;  // One break per loop is OK according to MISRA
            }
        }
    }
    return odt_lists_allocated;  // Return how many ODT lists that were successfully allocated
}

/**
 * @brief Remove all allocated common resources (DAQ lists, ODT lists, ODT entries) and remove references to common
 * resources in the "local" DAQ and ODT lists from the node struct of the client
 *
 * @param pclient The client
 */
void XcpSlaveCoreFreeResources(struct XcpSlaveCoreClient* pclient) {
    uint8_t i;  // Loop variable used for DAQ lists
    uint8_t j;  // Loop variable used for ODT lists and entries
    // First free allocations of common DAQ/ODT lists by writing DAQ_LIST_FREE/ODT_LIST_FREE to relevant positions
    for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
        if (DAQ_LIST_FREE == pclient->local_daq_lists[i]) {
            break;  // One break per loop is OK according to MISRA
        } else {
            daq_list_client[pclient->local_daq_lists[i]] = DAQ_LIST_FREE;
            daq_list_event[pclient->local_daq_lists[i]] = NO_EVENT;
            for (j = 0U; j < XCP_MAX_ODT_LISTS_PER_DAQ_LIST; j++) {
                if (ODT_LIST_FREE == pclient->local_odt_lists[i][j]) {
                    break;  // One break per loop is OK according to MISRA
                } else {
                    odt_list_client[pclient->local_odt_lists[i][j]] = ODT_LIST_FREE;
                    odt_list_entries[pclient->local_odt_lists[i][j]] = 0U;  // No entries as default
                    odt_list_byte_size[pclient->local_odt_lists[i][j]] = 0U;
                }
            }
        }
    }
    // Then remove references to "local" allocate positions in the node struct for the client to be removed
    for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
        pclient->local_daq_lists[i] = DAQ_LIST_FREE;
        pclient->local_daq_list_sel[i] = false;
        pclient->local_daq_list_active[i] = false;
        for (j = 0U; j < XCP_MAX_ODT_LISTS_PER_DAQ_LIST; j++) {
            pclient->local_odt_lists[i][j] = ODT_LIST_FREE;
        }
    }
    XcpSlavePrintfVerboseUint8Array("daq_list_client", daq_list_client, MAX_NR_OF_DAQ_LISTS);
    XcpSlavePrintfVerboseUint8Array("daq_list_event", daq_list_event, MAX_NR_OF_DAQ_LISTS);
    XcpSlavePrintfVerboseUint8Array("odt_list_client", odt_list_client, MAX_NR_OF_ODT_LISTS);
    XcpSlavePrintfVerboseUint8Array("odt_list_entries", odt_list_entries, MAX_NR_OF_ODT_LISTS);

    for (i = 0U; i < MAX_NR_OF_ODT_LISTS; i++) {  // Print how many bytes long each ODT list entry is
        XcpSlavePrintfVerboseDebug("odt_list_entry_byte array number %02x : ", i);
        XcpSlavePrintfVerboseUint8Array("odt_list_entry_byte", odt_list_entry_byte[i], MAX_ODT_ENTRIES_TOTAL);
    }
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

void XcpSlaveCoreInit(const uint64_t ProcBaseAddr_) {
    uint32_t i;  // For use in loops
    uint16_t j;  // for use in loops (uint16_t to avoid "type-limits" compile errorwhen comparing with
                 // MAX_ODT_ENTRIES_TOTAL)

    ProcBaseAddr = ProcBaseAddr_;
    // Initialize DAQ list related arrays
    for (i = 0U; i < MAX_NR_OF_DAQ_LISTS; i++) {
        daq_list_client[i] = DAQ_LIST_FREE;
        daq_list_event[i] = NO_EVENT;
    }
    for (i = 0U; i < MAX_NR_OF_ODT_LISTS; i++) {
        odt_list_client[i] = ODT_LIST_FREE;
        odt_list_entries[i] = 0U;  // Initialize to no entries
        for (j = 0U; j <= MAX_ODT_ENTRIES_TOTAL; j++) {
            odt_list_entry_byte[i][j] = 0U;  // Initialize to no entries
            odt_list_entry_address[i][j] = (uint64_t)NULL;
        }
    }

    is_calibration_data_from_file = is_calibration_data_from_file &&
                                    XcpSlaveValidateExternalCalData(calibration_data_filepath, checksum_filepath);
    if (is_calibration_data_from_file) {
        XcpSlaveReadCalPageFromExternalFile();
    }

    if (is_calibration_used == 1) {
        XcpSlaveCoreCalSupportInit(ProcBaseAddr);
    }
}

// This buffer is assigned global scope for testing purposes. In case XcpSlaveCoreProcessCmd is modified
// in the future, reconsider the scope of this buffer and how it should used for testing in xcp_slave_core_test project.
uint8_t responseBuf[XCP_MAX_CTO];  // Global to avoid reallocating every call.
uint16_t responseSizeExternal = 0;

void XcpSlaveCoreProcessCmd(const uint8_t command_buf[],
                            const uint16_t command_buf_length,
                            struct XcpSlaveCoreClient* pclient) {
    uint16_t responseSize = 0;
    uint8_t start_stop_synch_mode;
    uint32_t i;  // For use in loops
    uint64_t temp_address;
    uint8_t* ptemp_addr;
    uint8_t alloc_daq_lists;
    uint8_t alloc_odt_lists;

    if (false == pclient->connected) {
        if (CC_CONNECT == command_buf[XCP_CMD_NR_BYTE_POS]) {
            XcpSlavePrintfVerbose("");
            XcpSlavePrintfVerbose(
                    "CC_CONNECT code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
            pclient->connected = true;
            responseSize = 8;
            responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
            responseBuf[1] =
                    RESOURCE_DAQ_SUPPORTED | (uint8_t)is_calibration_used;  // DAQ always supported, CAL conditional
            responseBuf[2] = (ADDRESS_GRANULARITY + BYTE_ORDER_XCP);
            responseBuf[3] = XCP_MAX_CTO;  // Max payload bytes in CTO pkt
            WRITE_UINT16_TO_UINT8_ARRAY(&responseBuf[4], XCP_MAX_DTO);
            responseBuf[6] = XCP_PROTOCOL_VERSION;
            responseBuf[7] = XCP_PROTOCOL_TRANSPORT_LAYER;

        } else {
            // Not connected and command isn't connect. This is fishy. Issue negative response and clear client struct.
            XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, &responseSize);
            pclient->disconnect_after_response = true;
        }
    } else {
        // Connected.
        switch (command_buf[XCP_CMD_NR_BYTE_POS]) {
            case CC_SYNCH: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_SYNCH code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                // Unique response code for request SYNCH, no true error
                XcpSlaveCoreWriteNegativResponse(ERR_CMD_SYNCH, responseBuf, &responseSize);

            } break;
            case CC_GET_COMM_MODE_INFO: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_GET_COMM_MODE_INFO code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                responseSize = 8;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] = RESERVED_BYTE;
                responseBuf[2] = COMM_MODE_OPTIONAL;  // BLOCK mode, not supported
                responseBuf[3] = RESERVED_BYTE;
                responseBuf[4] = MAX_BS;      // BLOCK mode, not supported
                responseBuf[5] = MIN_ST;      // BLOCK mode, not supported
                responseBuf[6] = QUEUE_SIZE;  // BLOCK mode, not supported
                responseBuf[7] = XCP_DRIVER_VERSION_NUMBER;
            } break;
            case CC_DISCONNECT: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_DISCONNECT code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                responseSize = 1;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                pclient->disconnect_after_response = true;
                XcpSlavePrintfVerbose("Client disconnected!");
            } break;
            case CC_GET_STATUS: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_GET_STATUS code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                responseSize = 6;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                // Check current session status
                start_stop_synch_mode = false;
                for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
                    if (true == pclient->local_daq_list_active[i]) {
                        start_stop_synch_mode = true;
                        break;  // One break in each loop is OK according to MISRA
                    }
                }
                responseBuf[1] = ((start_stop_synch_mode << 6U) & 0x40U);
                if (is_calibration_used == 1) {
                    if (true == XcpSlaveCoreIsCalibrationStoreInProgress()) {
                        responseBuf[1] |= 1U;  // Polyspace yields warning if or'ing with
                                               // XcpSlaveCoreIsCalibrationStoreInProgress()
                    }
                }
                responseBuf[2] = pclient->curr_prot_status;
                responseBuf[3] = STATE_NUMBER;
                WRITE_UINT16_TO_UINT8_ARRAY(&responseBuf[4], SESSION_CONFIG_ID);
            } break;
            case CC_GET_SEED: {  // Will only be called by XCP client if "current resource protection status" is != 0
#if XCP_SEED_KEY_LENGTH > (XCP_MAX_CTO - 2)
#error Seeds requiring multiple CTOs is not supported
#endif
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_GET_SEED code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);

                // XCP client requests first part of the seed (the only one part as length is 2 bytes)
                if (!command_buf[1]) {
                    responseSize = 4;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    responseBuf[1] = XCP_SEED_KEY_LENGTH;
                    // Store resource of current GET_SEED request to be used in upcomming UNLOCK request
                    pclient->act_seed_res = command_buf[2];
                    // XCP client requests seed for CAL/PAG (calibration and page switching)
                    if (IS_RESOURCE_BIT_SET_ONLY_CAL_PAG(pclient->act_seed_res)) {
                        if (is_calibration_used == 1) {
                            XcpSlaveCoreGetSeedCalPag(pclient->seed, sizeof(pclient->seed));
                            memcpy(&responseBuf[2], &pclient->seed, sizeof(pclient->seed));
                            XcpSlavePrintfVerboseUint8Array("CAL/PAG seed", pclient->seed, sizeof(pclient->seed));
                        } else {
                            // The standard doesn't way what to do in this situation but OUT_OF_RANGE seems reasonable.
                            XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                        }
                    } else if (IS_RESOURCE_BIT_SET_ONLY_DAQ(pclient->act_seed_res)) {
                        XcpSlaveCoreGetSeedDaq(pclient->seed, sizeof(pclient->seed));
                        memcpy(&responseBuf[2], &pclient->seed, sizeof(pclient->seed));
                        XcpSlavePrintfVerboseUint8Array("DAQ seed", pclient->seed, sizeof(pclient->seed));
                    } else if (IS_RESOURCE_BIT_SET_ONLY_STIM(pclient->act_seed_res)) {
                        // The standard doesn't way what to do in this situation but OUT_OF_RANGE seems reasonable.
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                    } else if (IS_RESOURCE_BIT_SET_ONLY_PGM(pclient->act_seed_res)) {
                        // The standard doesn't way what to do in this situation but OUT_OF_RANGE seems reasonable.
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                    } else if (IS_RESOURCE_BIT_SET_ONLY_DBG(pclient->act_seed_res)) {
                        // The standard doesn't way what to do in this situation but OUT_OF_RANGE seems reasonable.
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                    } else {
                        // From standard:
                        // Only one resource may be requested with one GET_SEED command. If more than one
                        // resource has to be unlocked, the (GET_SEED+UNLOCK) sequence has to be performed
                        // multiple times. If the master does not request any resource or requests multiple resources
                        // at the same time, the slave will respond with an ERR_OUT_OF_RANGE.
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                    }
                } else {
                    // The whole seed can be sent in one response so "remaning part of* seed"
                    // messages is not needed and not supported
                    XcpSlavePrintfVerbose("GET_SEED error: Mode is 1 which is not supported");
                    XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                }
            } break;
            case CC_UNLOCK: {  // Called by XCP client if current resource protection status is not 0
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_UNLOCK code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);

                if (command_buf_length == 4U) {  // UNLOCK message length is 4, indicating a key that is 2 bytes long
                    const uint8_t* const accessKey = &command_buf[2];
                    uint8_t calculatedKey[XCP_SEED_KEY_LENGTH];
                    responseSize = 2;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    if (IS_RESOURCE_BIT_SET_ONLY_CAL_PAG(pclient->act_seed_res)) {
                        if (is_calibration_used == 1) {
                            XcpSlaveCoreCalPag(
                                    pclient->seed, sizeof(pclient->seed), calculatedKey, sizeof(calculatedKey));
                            if (memcmp(accessKey, calculatedKey, XCP_SEED_KEY_LENGTH) == 0) {
                                // Clear bit for calibration and page switch access to indicate unlocked resource
                                pclient->curr_prot_status &= ~RESOURCE_BIT_CAL_PAG;
                                responseBuf[1] = pclient->curr_prot_status;
                                XcpSlavePrintfVerbose("CAL/PAG key correct!");
                            } else {  // Wrong key
                                responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                                responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                            }
                            XcpSlavePrintfVerbose("act_seed_res: %02x", responseBuf[1]);
                            XcpSlavePrintfVerboseUint8Array("CAL/PAG seed", pclient->seed, sizeof(pclient->seed));
                            XcpSlavePrintfVerboseUint8Array("CAL/PAG key", accessKey, XCP_SEED_KEY_LENGTH);
                        } else {
                            // CAL not supported
                            responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                            responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                            pclient->disconnect_after_response = true;  // This is according to XCP standard.
                        }
                    } else if (IS_RESOURCE_BIT_SET_ONLY_DAQ(pclient->act_seed_res)) {
                        XcpSlaveCoreCalcKeyDaq(
                                pclient->seed, sizeof(pclient->seed), calculatedKey, sizeof(calculatedKey));
                        if (memcmp(accessKey, calculatedKey, XCP_SEED_KEY_LENGTH) == 0) {
                            // Clear bit data acquistion access to indicate unlocked resource
                            pclient->curr_prot_status &= ~RESOURCE_BIT_DAQ;
                            responseBuf[1] = pclient->curr_prot_status;
                            XcpSlavePrintfVerbose("DAQ key correct!");
                        } else {  // Wrong key
                            responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                            responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                            pclient->disconnect_after_response = true;  // This is according to XCP standard.
                        }
                        XcpSlavePrintfVerbose("act_seed_res: %02x", responseBuf[1]);
                        XcpSlavePrintfVerboseUint8Array("DAQ seed", pclient->seed, sizeof(pclient->seed));
                        XcpSlavePrintfVerboseUint8Array("DAQ key", accessKey, XCP_SEED_KEY_LENGTH);
                    } else if (IS_RESOURCE_BIT_SET_ONLY_STIM(pclient->act_seed_res)) {
                        // STIM not supported
                        responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                        responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                        pclient->disconnect_after_response = true;  // This is according to XCP standard.

                    } else if (IS_RESOURCE_BIT_SET_ONLY_PGM(pclient->act_seed_res)) {
                        // PGM not supported
                        responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                        responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                        pclient->disconnect_after_response = true;  // This is according to XCP standard.
                    } else if (IS_RESOURCE_BIT_SET_ONLY_DBG(pclient->act_seed_res)) {
                        // DGB not supported
                        responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                        responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                        pclient->disconnect_after_response = true;  // This is according to XCP standard.
                    }
                } else {  // Wrong key
                    responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
                    responseBuf[ERROR_CODE_IDX] = ERR_ACCESS_LOCKED;
                    pclient->disconnect_after_response = true;  // This is according to XCP standard.
                    XcpSlavePrintfVerbose("Wrong key length (not 2 bytes)");
                }
            } break;
            case CC_SET_MTA: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_SET_MTA code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                XcpSlavePrintfVerboseUint8Array("command_buf", command_buf, SET_MTA_MSG_LEN);

                temp_address = READ_UINT32_FROM_UINT8_ARRAY(&command_buf[4]);
                // We don't do range checks here because allowed addresses depends on which service the MTA is used for.
                pclient->mem_trans_addr = temp_address;
                responseSize = 1;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                XcpSlavePrintfVerbose("MTA: %0lx", pclient->mem_trans_addr);
            } break;
            case CC_SHORT_UPLOAD: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_SHORT_UPLOAD code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                XcpSlavePrintfVerboseUint8Array("command_buf", command_buf, (uint32_t)(command_buf[0]));

                if (RESOURCE_IS_UNLOCKED_CAL_PAG(pclient->curr_prot_status) ||
                    RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    pclient->mem_trans_addr = READ_UINT32_FROM_UINT8_ARRAY(&command_buf[4]);
                    ptemp_addr = (uint8_t*)(ProcBaseAddr + pclient->mem_trans_addr);
                    const uint8_t bytes_to_upload = command_buf[1];
                    XcpSlavePrintfVerbose("mem_trans_addr: %lu", pclient->mem_trans_addr);
                    if (IsBlockInBss(ptemp_addr, bytes_to_upload) || IsBlockInData(ptemp_addr, bytes_to_upload) ||
                        (is_calibration_used == 1 && XcpSlaveCoreIsBlockInCalData(ptemp_addr, bytes_to_upload))) {
                        if (bytes_to_upload <= (XCP_MAX_CTO - UPLOAD_RES_HEADER_LEN)) {
                            responseSize = bytes_to_upload + UPLOAD_RES_HEADER_LEN;
                            responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                            for (i = 0U; i < (uint32_t)bytes_to_upload; i++) {
                                responseBuf[UPLOAD_RES_HEADER_LEN + i] = *ptemp_addr;
                                ptemp_addr++;
                                pclient->mem_trans_addr++;
                            }
                        } else {
                            XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                            XcpSlavePrintfVerbose("ERR_OUT_OF_RANGE");
                        }
                    } else {
                        XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose("ERR_ACCESS_DENIED");
                    }
                } else {  // calibration (CAL/PAG) or measurement (DAQ) access has not been unlocked
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            case CC_BUILD_CHECKSUM: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_BUILD_CHECKSUM code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                XcpSlavePrintfVerboseUint8Array("command_buf", command_buf, BUILD_CHECKSUM_MSG_LEN);

                const uint32_t block_length = READ_UINT32_FROM_UINT8_ARRAY(&command_buf[4]);
                XcpSlavePrintfVerbose("Block Length %08x", block_length);
                ptemp_addr = (uint8_t*)(ProcBaseAddr + pclient->mem_trans_addr);

                if (IsBlockInText(ptemp_addr, block_length) ||
                    ((is_calibration_used == 1) && XcpSlaveCoreIsBlockInCalData(ptemp_addr, block_length))) {
                    const uint32_t crc =
                            CalChecksum(ptemp_addr, block_length);  // We calculate a 16-bit checksum but the protocol
                                                                    // specifies that a DWORD shall be returned.
                    responseSize = 8;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    responseBuf[1] = CHKSUM_TYPE_CRC16CITT;
                    responseBuf[2] = RESERVED_BYTE;
                    responseBuf[3] = RESERVED_BYTE;
                    WRITE_UINT32_TO_UINT8_ARRAY(&responseBuf[4], crc);
                    pclient->mem_trans_addr += (uint64_t)block_length;  // Post-increment MTA
                    XcpSlavePrintfVerbose("crc: %08x", crc);
                } else {
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, &responseSize);
                    XcpSlavePrintfVerbose("ERR_ACCESS_DENIED");
                }
            } break;
            case CC_GET_DAQ_PROCESSOR_INFO: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_GET_DAQ_PROCESSOR_INFO code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);

                responseSize = 8;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] = DAQ_PROPERTIES;
                responseBuf[2] = MAX_DAQ_LSB;
                responseBuf[3] = MAX_DAQ_MSB;
                responseBuf[4] = XCP_NR_OF_EVENTS;
                responseBuf[5] = NR_OF_EVENTS_MSB;
                responseBuf[6] = MIN_DAQ;
                responseBuf[7] = DAQ_KEY_BYTE;
            } break;
            case CC_GET_DAQ_RESOLUTION_INFO: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_GET_DAQ_RESOLUTION_INFO code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                responseSize = 8;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] = GRANULARITY_ODT_ENTRY_SIZE_DAQ;
                responseBuf[2] = MAX_ODT_ENTRIES_TOTAL;
                responseBuf[3] = GRANULARITY_ODT_ENTRY_SIZE_STIM;
                responseBuf[4] = MAX_ODT_ENTRY_SIZE_STIM;
                responseBuf[5] = TIMESTAMP_MODE;
                responseBuf[6] = TIMESTAMP_TICKS_LSB;
                responseBuf[7] = TIMESTAMP_TICKS_MSB;
            } break;
            case CC_FREE_DAQ: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_FREE_DAQ code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                XcpSlaveCoreFreeResources(pclient);
                responseSize = 1;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
            } break;
            case CC_ALLOC_DAQ: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_ALLOC_DAQ code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                // The specified number of DAQ lists are allocated if there are enough free DAQ lists available
                // Measure access is granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    alloc_daq_lists = XcpSlaveCoreAllocDaq(pclient, command_buf[2]);
                    XcpSlavePrintfVerbose("Number of DAQ lists to be allocated: %d", command_buf[2]);
                    XcpSlavePrintfVerbose("Number of allocated DAQ lists: %d", alloc_daq_lists);
                    if (command_buf[2] == alloc_daq_lists) {
                        responseSize = 1;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    } else { /*We ran out of free DAQ lists so send negative response with code
                                ERR_MEMORY_OVERFLOW*/
                        XcpSlaveCoreWriteNegativResponse(ERR_MEMORY_OVERFLOW, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose(
                                "We ran out of free DAQ lists, negative "
                                "response sent to the XCP client!");
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            case CC_ALLOC_ODT: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_ALLOC_ODT code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                // The specified number of ODT lists are allocated if there are enough free ODT lists available
                // Meas access is granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    alloc_odt_lists = XcpSlaveCoreAllocOdt(pclient, command_buf[2], command_buf[4]);
                    XcpSlavePrintfVerbose("DAQ list: %d, number of ODT lists: %d, number of allocated ODT lists: %d",
                                          command_buf[2],
                                          command_buf[4],
                                          alloc_odt_lists);
                    if (command_buf[4] == alloc_odt_lists) {
                        responseSize = 1;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    } else {  // We ran out of free ODT lists so send negative response with code ERR_MEMORY_OVERFLOW
                        XcpSlaveCoreWriteNegativResponse(ERR_MEMORY_OVERFLOW, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose(
                                "We ran out of free ODT lists, negative "
                                "response sent to the XCP client!");
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            case CC_ALLOC_ODT_ENTRY: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_ALLOC_ODT_ENTRY code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                // The specified number of ODT list entries are entered in the odt_list_entries array based on the
                // client local DAQ and ODT list number received in this request.
                // Meas access is granted if unlocked via Seed&Key

                // Local variable to avoid compiler error "type-limits" when comparing MAX_ODT_ENTRIES_TOTAL
                // (potentially 0xFF) with command_buf[5], which is uint8_t
                static uint16_t max_odt_entries_total = MAX_ODT_ENTRIES_TOTAL;

                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    if ((command_buf[2] >= XCP_MAX_DAQ_LISTS_PER_CLIENT) ||
                        (command_buf[4] >= XCP_MAX_ODT_LISTS_PER_DAQ_LIST)) {
                        // DAQ or ODT is out of range
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose(
                                "XCP client tried to access DAQ"
                                "or ODT list out of range");
                    } else {
                        // Check if client has requested within the limit of allocatable ODT entries
                        if (command_buf[5] <= max_odt_entries_total) {
                            odt_list_entries[pclient->local_odt_lists[command_buf[2]][command_buf[4]]] = command_buf[5];
                            XcpSlavePrintfVerbose("DAQ list: %d, ODT list: %d, number of ODT list entries: %d",
                                                  command_buf[2],
                                                  command_buf[4],
                                                  command_buf[5]);
                            responseSize = 1;
                            responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                        } else {
                            // XCP_MAX_DTO does not have space to accomodate any signals
                            XcpSlaveCoreWriteNegativResponse(ERR_MEMORY_OVERFLOW, responseBuf, &responseSize);
                            XcpSlavePrintfVerbose(
                                    "XCP does not have space for %d "
                                    "entries in ODT list",
                                    command_buf[5]);
                        }
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            // SET_DAQ_LIST_MODE ties an event to the specified DAQ list
            case CC_SET_DAQ_LIST_MODE: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_SET_DAQ_LIST_MODE code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                // Meas access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    const uint16_t daq_list_number = READ_UINT16_FROM_UINT8_ARRAY(&command_buf[2]);
                    const uint16_t event_channel = READ_UINT16_FROM_UINT8_ARRAY(&command_buf[4]);
                    XcpSlavePrintfVerbose("DAQ list: %d, event number: %d", daq_list_number, event_channel);
                    // Check that requested DAQ list and event are within boundaries
                    if ((daq_list_number < XCP_MAX_DAQ_LISTS_PER_CLIENT) && (event_channel <= XCP_NR_OF_EVENTS)) {
                        // Look in the local DAQ list array to find the corresponding global DAQ list number
                        daq_list_event[pclient->local_daq_lists[command_buf[2]]] = command_buf[4];
                        responseSize = 1;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                        // We don't store mode because none of the bits are used.
                    } else {  // A DAQ list that is not allocated or a non existen event has been selected
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose("ERR_OUT_OF_RANGE");
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            // SET_DAQ_PTR points to a DAQ list, an ODT list and a ODT list entry
            // command_buf[2] contains DAQ_LIST_NUMBER LSB
            // command_buf[3] contains DAQ_LIST_NUMBER MSB (not used as > 255 DAQ-lists not needed due to ethernet)
            // command_buf[4] contains ODT_NUMBER
            // command_buf[5] contains ODT_ENTRY_NUMBER
            case CC_SET_DAQ_PTR: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_SET_DAQ_PTR code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                // Meas access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    // Check if DAQ, ODT and ODT entry pointed to is allocated
                    if ((command_buf[2] < XCP_MAX_DAQ_LISTS_PER_CLIENT) &&
                        (command_buf[4] < XCP_MAX_ODT_LISTS_PER_DAQ_LIST) &&
                        (DAQ_LIST_FREE != pclient->local_daq_lists[command_buf[2]]) &&
                        (ODT_LIST_FREE != pclient->local_odt_lists[command_buf[2]][command_buf[4]]) &&
                        (command_buf[5] < odt_list_entries[pclient->local_odt_lists[command_buf[2]][command_buf[4]]])) {
                        pclient->daq = pclient->local_daq_lists[command_buf[2]];
                        pclient->odt = pclient->local_odt_lists[command_buf[2]][command_buf[4]];
                        pclient->first_odt = (uint8_t)(command_buf[4] == FIRST_ODT_NR);
                        pclient->odt_entry = command_buf[5];
                        XcpSlavePrintfVerbose("DAQ list: %d, ODT list: %d, ODT list entry: %d",
                                              pclient->daq,
                                              pclient->odt,
                                              pclient->odt_entry);
                        responseSize = 1;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    }
                    // XCP client tries to set DAQ, ODT or ODT list entry pointer to position that is not allocaed
                    else {
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose("ERR_OUT_OF_RANGE");
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            // WRITE_DAQ assigns an address and a byte length to the ODT list entry pointed to by SET_DAQ_PTR
            // command_buf[2] contains the size of the selected ODT list entry in bytes
            // command_buf[7-11] contains the address of the selected ODT list entry
            case CC_WRITE_DAQ: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_WRITE_DAQ code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);
                XcpSlavePrintfVerboseUint8Array("command_buf", command_buf, WRITE_DAQ_MSG_LEN);
                // Meas access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    temp_address = READ_UINT32_FROM_UINT8_ARRAY(&command_buf[4]);
                    ptemp_addr = (uint8_t*)(ProcBaseAddr + temp_address);

                    // Calculate total number of bytes in ODT list
                    odt_list_byte_size[pclient->odt] += (uint16_t)command_buf[2];
                    if (IsBlockInBss(ptemp_addr, command_buf[2]) || IsBlockInData(ptemp_addr, command_buf[2])) {
                        // Check DTO packet size limit
                        if (odt_list_byte_size[pclient->odt] <=
                            (XCP_MAX_DTO - ODT_PID_LEN - (pclient->first_odt * TIMESTAMP_LEN))) {
                            // Set number of bytes in pointed to ODT list entry
                            // Which DAQ is pointed to does not matter as ODT lists are tied to DAQ lists in pclient
                            odt_list_entry_byte[pclient->odt][pclient->odt_entry] = command_buf[2];
                            // Set memory address in pointed to ODT list entry
                            // Which DAQ is pointed to does not matter as ODT lists are tied to DAQ lists in pclient
                            odt_list_entry_address[pclient->odt][pclient->odt_entry] = (uint64_t)ptemp_addr;
                            // ODT list entry shall be post incremeted after each WRITE_DAQ req if value is within limit
                            if (pclient->odt_entry < odt_list_entries[pclient->odt]) {
                                pclient->odt_entry++;
                            } else {
                                XcpSlavePrintfVerbose("ODT byte size: %d", odt_list_byte_size[pclient->odt]);
                            }
                            responseSize = 1;
                            responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                        } else {  // Too many bytes allocated in the ODT list (due to DTO size), send negative response
                            XcpSlaveCoreWriteNegativResponse(ERR_MEMORY_OVERFLOW, responseBuf, &responseSize);
                            XcpSlavePrintfVerbose(
                                    "We ran out of DTO packet room, negative "
                                    "response sent to the XCP client!");
                        }
                    } else {
                        XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_DENIED, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose("ERR_ACCESS_DENIED");
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            // START_STOP_DAQ_LIST selects a DAQ list and sets it to one of the modes: STOP, START or SELECT
            // command_buf[1] contains MODE (stop, start or select DAQ-list)
            // command_buf[2] contains selected DAQ_LIST_NUMBER
            case CC_START_STOP_DAQ_LIST: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_START_STOP_DAQ_LIST code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);

                // Meas access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    const uint8_t daqListMode = command_buf[1];
                    const uint8_t daqListNr = command_buf[2];
                    XcpSlavePrintfVerbose("Mode: %d, selected DAQ: %d, first pid: %u",
                                          daqListMode,
                                          daqListNr,
                                          (daqListNr * XCP_MAX_ODT_LISTS_PER_DAQ_LIST));
                    if (command_buf[2] < XCP_MAX_DAQ_LISTS_PER_CLIENT) { /*Bounds check of selected DAQ
                                                                            list*/
                        if (STOP_DAQ_LIST == daqListMode) {
                            pclient->local_daq_list_active[daqListNr] = false;
                        } else if (START_DAQ_LIST == daqListMode) {
                            pclient->local_daq_list_active[daqListNr] = true;
                        } else if (SELECT_DAQ_LIST == daqListMode) {
                            pclient->local_daq_list_sel[daqListNr] = true;
                        }
                        responseSize = 2;
                        responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                        // FIRST_PID needs to be communicated due to that IDENTIFICATION_FIELD_TYPE_ABSOLUTE is used
                        // The first PID in each DAQ list is set to the maximum number of ODT lists per DAQ list
                        // multiplied with the local DAQ list number. The following PID's in the DAQ list is
                        // incremented by one for each new ODT list that is transmitted
                        responseBuf[FIRST_PID_IDX] = daqListNr * XCP_MAX_ODT_LISTS_PER_DAQ_LIST;
                    } else {
                        XcpSlaveCoreWriteNegativResponse(ERR_OUT_OF_RANGE, responseBuf, &responseSize);
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            // START_STOP_SYNC can start transmission of DAQ lists selected by START_STOP_DAQ_LIST and stop
            // transmission of all DAQ lists or the ones selected by START_STOP_DAQ_LIST
            case CC_START_STOP_SYNCH: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose("CC_START_STOP_SYNCH code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                XcpSlavePrintfVerboseUint8Array("command_buf", command_buf, START_STOP_SYNCH_MSG_LEN);

                // Meas access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    responseSize = 1;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    if (STOP_ALL_DAQ_LISTS == command_buf[1]) {
                        for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
                            pclient->local_daq_list_sel[i] = false;
                            pclient->local_daq_list_active[i] = false;
                        }
                    } else if (STOP_SEL_DAQ_LISTS == command_buf[1]) {
                        for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
                            if (true == pclient->local_daq_list_sel[i]) {
                                pclient->local_daq_list_sel[i] = false;
                                pclient->local_daq_list_active[i] = false;
                            }
                        }
                    } else if (START_SEL_DAQ_LISTS == command_buf[1]) {
                        XcpSlavePrintfVerboseUint8Array("daq_list_client", daq_list_client, MAX_NR_OF_DAQ_LISTS);
                        XcpSlavePrintfVerboseUint8Array("daq_list_event", daq_list_event, MAX_NR_OF_DAQ_LISTS);
                        XcpSlavePrintfVerboseUint8Array("odt_list_client", odt_list_client, MAX_NR_OF_ODT_LISTS);
                        XcpSlavePrintfVerboseUint8Array("odt_list_entries", odt_list_entries, MAX_NR_OF_ODT_LISTS);

                        // Print how many bytes long each ODT list entry is
                        for (i = 0U; i < MAX_NR_OF_ODT_LISTS; i++) {
                            XcpSlavePrintfVerboseDebug("odt_list_entry_byte array number %02x : ", i);
                            XcpSlavePrintfVerboseUint8Array(
                                    "odt_list_entry_byte", odt_list_entry_byte[i], MAX_ODT_ENTRIES_TOTAL);
                        }

                        for (i = 0U; i < XCP_MAX_DAQ_LISTS_PER_CLIENT; i++) {
                            if (true == pclient->local_daq_list_sel[i]) {
                                pclient->local_daq_list_sel[i] = false;
                                pclient->local_daq_list_active[i] = true;
                            }
                        }
                    } else {  // command_buf[1] (Mode) has an unsupported value
                        XcpSlaveCoreWriteNegativResponse(ERR_MODE_NOT_VALID, responseBuf, &responseSize);
                    }
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            case CC_GET_DAQ_CLOCK: {
                XcpSlavePrintfVerbose("");
                XcpSlavePrintfVerbose(
                        "CC_GET_DAQ_CLOCK code:%02X, client %d", command_buf[XCP_CMD_NR_BYTE_POS], pclient->client_id);

                // Measure access granted if unlocked via Seed&Key
                if (RESOURCE_IS_UNLOCKED_DAQ(pclient->curr_prot_status)) {
                    const uint32_t time =
                            XcpSlaveCoreGetEpochTime100us();  // Update time for last GET_STATUS command reception
                    responseSize = 8;
                    responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                    responseBuf[1] = RESERVED_BYTE;
                    responseBuf[2] = TRIGGER_INITIATOR;
                    responseBuf[3] = PAYLOAD_FMT;
                    WRITE_UINT32_TO_UINT8_ARRAY(&responseBuf[4], time);
                } else {  // Seed & Key routine not performed or key is wrong so calibration access is not permitted
                    XcpSlaveCoreWriteNegativResponse(ERR_ACCESS_LOCKED, responseBuf, &responseSize);
                }
            } break;
            case CC_CONNECT: {
                XcpSlavePrintfVerbose("CC_CONNECT once again!! code:%02X, client %d",
                                      command_buf[XCP_CMD_NR_BYTE_POS],
                                      pclient->client_id);
                pclient->connected = true;
                responseSize = 8;
                responseBuf[RESPONSE_TYPE_IDX] = POSITIVE_RESPONSE;
                responseBuf[1] =
                        RESOURCE_DAQ_SUPPORTED | (uint8_t)is_calibration_used;  // DAQ always supported, CAL conditional
                responseBuf[2] = (ADDRESS_GRANULARITY + BYTE_ORDER_XCP);
                responseBuf[3] = XCP_MAX_CTO;  // Max payload bytes in CTO pkt
                WRITE_UINT16_TO_UINT8_ARRAY(&responseBuf[4], XCP_MAX_DTO);
                responseBuf[6] = XCP_PROTOCOL_VERSION;
                responseBuf[7] = XCP_PROTOCOL_TRANSPORT_LAYER;
            } break;
            default: {
                if (is_calibration_used == 1) {
                    XcpSlaveCoreProcessCalCommand(command_buf, pclient, responseBuf, &responseSize);
                    if (responseSize == 0) {
                        XcpSlaveCoreWriteNegativResponse(ERR_CMD_UNKNOWN, responseBuf, &responseSize);
                        XcpSlavePrintfVerbose("Unknown command: %02x", command_buf[0]);
                    } else {
                        // Empty by design, calibration module have written a response.
                    }
                } else {
                    XcpSlaveCoreWriteNegativResponse(ERR_CMD_UNKNOWN, responseBuf, &responseSize);
                    XcpSlavePrintfVerbose("Unknown command: %02x", command_buf[0]);
                }
            } break;
        }
    }
    responseSizeExternal = responseSize;
}

void XcpSlaveCoreOnEvent(const uint8_t event, XcpSlaveCoreClient* pclient) {
    // Buffer to hold DTO data to be sent total length of ethernet packet is the sum of:
    // MAX_DTO which consists of 1 byte PID and the rest measurement data)
    // XCP_HEADER_LEN which consists of 2 bytes "total packet length" and 2 bytes "message counter"
    // For ethernet, the message counter field (CTR) is not part of XCP packet. It is easy to be fooled if only the
    // XCP base standard is referenced as the "AS trasport layer" document for XCP over ethernet deviates.
    uint8_t j;                           // loop variable used to index DAQ lists
    uint8_t k;                           // loop variable used to index ODT lists
    uint8_t m;                           // loop variable used to index ODT list entries
    uint8_t n;                           // loop variable used to index single bytes of ODT list entry
    uint8_t* pdata;                      // Used to point out the measuement data to be sent
    uint32_t time;                       // Time in 100 us ticks since Unix Epoch
    static uint8_t dtoBuf[XCP_MAX_DTO];  // Static to avoid reallocating every call.
    uint16_t dtoSize;

    // Check in all clients if there is any DAQ list tied to the current event that has ODT lists to be transmitted
    for (j = 0U; j < XCP_MAX_DAQ_LISTS_PER_CLIENT; j++) {  // Loop through all DAQ lists in client
        // Check if this client is not using any more DAQ lists or if send function returned fail
        if (DAQ_LIST_FREE == pclient->local_daq_lists[j]) {
            break;                                               // One break per loop is okay according to MISRA
        } else if (true == pclient->local_daq_list_active[j]) {  // DAQ list is active
            if (event == daq_list_event[pclient->local_daq_lists[j]]) {  // DAQ list is tied to this event
                for (k = 0U; k < XCP_MAX_ODT_LISTS_PER_DAQ_LIST; k++) {  // Loop through ODT lists in this DAQ
                    // Check if DAQ is not using any more ODT's or if send function returned fail
                    if (ODT_LIST_FREE == pclient->local_odt_lists[j][k]) {
                        break;  // One break per loop is okay according to MISRA
                    } else {
                        dtoSize = 0;
                        // PID is the ODT list identification number. The XCP server uses absolute  numbering
                        // so PID = (DAQ list number * MAX_ODT_LIST_PER_DAQ_LIST + ODT list number
                        dtoBuf[dtoSize] = (j * XCP_MAX_ODT_LISTS_PER_DAQ_LIST) + k;
                        dtoSize++;
                        // WOW, we found a ODT list to send!
                        // Is it place for this ODT list in the current UDP packed buffer?
                        // Fist check that function XcpServer currently is not accessing udp_data and
                        // xcp_offs or is handling UDP packets
                        if (0U == k) {  // First ODT list in the DAQ list shall have the timestamp.
                            time = XcpSlaveCoreGetEpochTime100us();
                            WRITE_UINT32_TO_UINT8_ARRAY(&dtoBuf[1], time);
                            dtoSize += sizeof(time);
                        } else {  // This is not the first ODT list in the DAQ so no timestamp
                            // Intentionally empty.
                        }
                        // Loop through all ODT list entries in the ODT list
                        for (m = 0U; m < odt_list_entries[pclient->local_odt_lists[j][k]]; m++) {
                            pdata = (uint8_t*)(odt_list_entry_address[pclient->local_odt_lists[j][k]][m]);
                            // Loop through the number of bytes in the ODT list entry
                            for (n = 0U; n < odt_list_entry_byte[pclient->local_odt_lists[j][k]][m]; n++) {
                                dtoBuf[dtoSize] = pdata[n];
                                dtoSize++;
                            }
                        }

                        XcpSlaveCoreSendPacket(pclient->client_id, dtoBuf, dtoSize, XcpPacketTxMethod_WHEN_MTU_FULL);
                    }
                }
            }
        }
    }
}

void XcpSlaveCoreInitClient(struct XcpSlaveCoreClient* pclient) {
    uint32_t j;
    uint32_t k;
    pclient->client_id = CLIENT_POS_FREE;
    pclient->connected = false;
    pclient->last_cmd_epoch_time = 0U;
    pclient->mem_trans_addr = 0U;
    pclient->daq = 0U;
    pclient->odt = 0U;
    pclient->first_odt = 0U;
    pclient->odt_entry = 0U;
    pclient->curr_prot_status = XCP_RESOURCE_PROTECTION;
    memset(&pclient->seed, 0, sizeof(pclient->seed));
    pclient->act_seed_res = 0U;
    for (j = 0U; j < XCP_MAX_DAQ_LISTS_PER_CLIENT; j++) {
        pclient->local_daq_lists[j] = DAQ_LIST_FREE;
        pclient->local_daq_list_sel[j] = false;
        pclient->local_daq_list_active[j] = false;
        for (k = 0U; k < XCP_MAX_ODT_LISTS_PER_DAQ_LIST; k++) {
            pclient->local_odt_lists[j][k] = ODT_LIST_FREE;
        }
    }
    pclient->abandon_pending = false;
    pclient->disconnect_after_response = false;
}

void XcpSlaveCoreCloseClient(struct XcpSlaveCoreClient* pclient) {
    XcpSlaveCoreFreeResources(pclient);
    XcpSlaveCoreInitClient(pclient);
}

bool XcpSlaveCoreIsClientFree(struct XcpSlaveCoreClient* pclient) {
    if (pclient->client_id == CLIENT_POS_FREE) {
        return true;
    } else {
        return false;
    }
}

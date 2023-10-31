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
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// set the default visibility
#define XCP_EXPORT __attribute__((visibility("default")))

// Note:
// please go through the official guideline before any integration actions:
// https://c1.confluence.cm.volvocars.biz/display/ARTVDDP/A2L+solution+integration+guideline

// declaration helper
// macro adds linker variables to the caller's project.
// these variables should be added as global variables, so
// should not be added in a function or so
extern unsigned char TextStart;
extern unsigned char TextEnd;
extern unsigned char BssStart;
extern unsigned char BssEnd;
extern unsigned char DataStart;
extern unsigned char DataEnd;
extern unsigned char CalPageStart;
extern unsigned char CalPageEnd;

#ifdef XCP_SLAVE_CALIBRATION_SUPPORT
#define XCP_SLAVE_CALIBRATION_STATUS 1
#else
#define XCP_SLAVE_CALIBRATION_STATUS 0
#endif

// clang-format off
// initialization helper
#define XCP_CREATE_INIT_DATA()                            \
{                                                         \
    /*struct_version*/ 3,                                 \
    /*proc_base_address*/ &TextStart,                     \
    /*server_port*/ XCP_SLAVE_UDP_PORT,                   \
    /*is_calibration_used*/ XCP_SLAVE_CALIBRATION_STATUS, \
    /*log_user_data*/ 0,                                  \
    /*log_level*/ 0,                                      \
    /*text_section_start*/ &TextStart,                    \
    /*text_section_end*/ &TextEnd,                        \
    /*bss_section_start*/ &BssStart,                      \
    /*bss_section_end*/ &BssEnd,                          \
    /*data_section_start*/ &DataStart,                    \
    /*data_section_end*/ &DataEnd,                        \
    /*caldata_section_start*/ &CalPageStart,              \
    /*caldata_section_end*/ &CalPageEnd,                  \
    /*calibration_data_filepath*/ 0,                      \
    /*xcp_init_status*/ 0,                                \
    /*checksum_filepath*/ 0                               \
}
// clang-format on

// initialization parameters. should be created and filled in by the caller.
// it's strongly recommended to use INIT_DATA
// macro to initialize this structure with correct default values. you can
// alternate them later if needed
typedef struct init_data {
    // maintenance
    int struct_version;  // holds the version of the current structure

    // application settings
    void* proc_base_address;  // base address
    int server_port;          // server UDP port
    int is_calibration_used;  // will client use calibration or not?

    // logging
    void* log_user_data;  // user data pointer that will be propagated to UserLog "as is"
    int log_level;        // verbosity level. 0 - no logs (default), 1 - info, 2 - debug

    // linker variables
    // will be filled in by the linker script, please do not update these variables manually.
    // please do not forget to add DEFINE_LINKER_VARIABLE/DEFINE_LINKER_VARIABLE_WITH_CALIBRATION
    // macro to your code to add all needed variables
    void* text_section_start;
    void* text_section_end;
    void* bss_section_start;
    void* bss_section_end;
    void* data_section_start;
    void* data_section_end;
    void* caldata_section_start;
    void* caldata_section_end;

    // This field is valid for struct version >= 2
    char* calibration_data_filepath;  // filepath of calibration data from external file

    // This field is valid for struct version >=3 and for internal testing purposes
    // Value is set to -1 if init fails and to 0 if it is successful
    int xcp_init_status;

    // when calibration data is fed from external file, the checksum file is mandatory
    // this contains Data checksum and Code checksum CRC
    char* checksum_filepath;

} init_data_t;

/**
 * @brief Handles load or creation of reference and working page files and starts XcpServer thread
 *
 * @param app_init_data [in]  pointer to a structure that holds all initialization parameters.
 *                            please use INIT_DATA macro for the proper
 *                            initialization
 */
void XCP_EXPORT XcpInit(const init_data_t* const app_init_data);

/**
 * @brief Send the DTO's allocated to the event stated by argument 'event'(called by app using the XCP server)
 *
 * @param event     [in]  Number stated by EVENT_CHANNEL_NUMBER in the corresponding EVENT block in the a2l-file
 */
void XCP_EXPORT XcpEvent(const unsigned char event);

/**
 * @brief Generates a seed for seed-and-key and writes it to dst
 * Use the IS_RESOURCE_BIT_SET_ONLY to find out for which resource a key shall be calculated.
 *
 * @param dst       [out] Buffer to write key to
 * @param dstSize   [in]  Size of buffer
 */
void XCP_EXPORT XcpSlaveCoreGetSeedCalPag(unsigned char* const dst, const unsigned char dstSize);

/**
 * @brief Generates a seed for seed-and-key and writes it to dst
 * Use the IS_RESOURCE_BIT_SET_ONLY to find out for which resource a key shall be calculated.
 *
 * @param dst       [out] Buffer to write key to
 * @param dstSize   [in]  Size of buffer
 */
void XCP_EXPORT XcpSlaveCoreGetSeedDaq(unsigned char* const dst, const unsigned char dstSize);

/**
 * @brief Calculates the key for a given seed for a certain resoure
 * Use the IS_RESOURCE_BIT_SET_ONLY to find out for which resource a key shall be calculated.
 *
 * @param seed      [in]  seed
 * @param seedSize  [in]  Size of seed
 * @param key       [out] Buffer to write calculated key to
 * @param keySize   [in]  Size of key buffer
 */
void XCP_EXPORT XcpSlaveCoreCalPag(const unsigned char* seed,
                                   const unsigned char seedSize,
                                   unsigned char* key,
                                   const unsigned char keySize);

/**
 * @brief Calculates the key for a given seed for a certain resoure
 * Use the IS_RESOURCE_BIT_SET_ONLY to find out for which resource a key shall be calculated.
 *
 * @param seed      [in]  seed
 * @param seedSize  [in]  Size of seed
 * @param key       [out] Buffer to write calculated key to
 * @param keySize   [in]  Size of key buffer
 */
void XCP_EXPORT XcpSlaveCoreCalcKeyDaq(const unsigned char* seed,
                                       const unsigned char seedSize,
                                       unsigned char* key,
                                       const unsigned char keySize);

/**
 * @brief returns epoch in 100us resolution. should have 4 bytes size
 *
 * @return epoch in 100us resolution. Remember to update #define TIMESTAMP_MODE if you change the resolution!
 */
unsigned int XCP_EXPORT XcpSlaveCoreGetEpochTime100us(void);

/**
 * @brief logging routine that should be implemented on the client's side.
 * could be empty if the logging capabilities are not needed
 *
 * @param log_text      [in]  pointer to a message to log
 * @param log_user_data [in]  pointer to a user data from init_data
 */
void XCP_EXPORT UserLog(const char* const log_text, void* const log_user_data);

#ifdef __cplusplus
}
#endif

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

/********** DAQ list size related defines ****************************************************************************/
/* The defines in this segment will affect the size of the static "memory pool" that is allocated for this instance  */
/* of the XCP server. For the XCP server to work properly, the max value of the "DAQ list size related defines" is   */
/* limited to 255. Additionaly, the product of:                                                                      */
/* (XCP_MAX_NR_OF_CLIENTS x XCP_MAX_DAQ_LISTS_PER_CLIENT x XCP_MAX_ODT_LISTS_PER_DAQ_LIST needs to be less than 256) */

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of clients that can be connected to XCP server at the same time */
#define XCP_MAX_NR_OF_CLIENTS 2U

/* Number of event channels defined in a2l-file. Size: Byte */
#define XCP_NR_OF_EVENTS 3U

/* Maximum number of DAQ lists allowed per client. Size: Byte */
#define XCP_MAX_DAQ_LISTS_PER_CLIENT XCP_NR_OF_EVENTS /* There needs to be at least one DAQ list per event */

/* Maximum number of ODT lists per DAQ list. Size: Byte */
#define XCP_MAX_ODT_LISTS_PER_DAQ_LIST 20U /* Will allow measurement of 6000 Bytes in one DAQ list */
/*********************************************************************************************************************/

/********** Ethernet related defines *********************************************************************************/
/* The defines in this segment will affect the size of the ethernet packets sent to and from the XCP server. The XCP */
/* standard allows packet lengths of up to 255 bytes for CTO and up to 65535 long DTO packets (but in practice ~1500 */
/* as it is the maximum byte length of the type of ethernet packets used in the XCP server. The DTO packet length is */
/* partitioned to a low and a high DTO length byte, LSB and MSB which both needs to be less or equal to 255          */

/* Maximum length of CTO ethernet packets.*/
#define XCP_MAX_CTO 0xFFU /* Maximum value is 0xFF as CTO size is communicated to XCP master in one byte */

/* Maximum length of DTO packets in bytes */
#define XCP_MAX_DTO 300U /* 300 to get a decent fill rate of a UDP packet with 1400 bytes MTU (1400/300 = 4) */

/* Maximum Transmission Unit for UDP packets. Should not be set over 1472 bytes to avoid fragmentation */
#define XCP_MTU 1400U
/*********************************************************************************************************************/

/********** Calibration file related defines *************************************************************************/
/* If true, the contents of existing reference page file will be loaded at init.                                     */
/* If false, the contents of the calibration area (in RAM) will be written to a new or existing reference page file  */
#define XCP_INIT_WITH_EXISTING_CAL_FILES true
/*********************************************************************************************************************/

/********** Resource protection *************************************************************************/
/* See documentation (XcpExampleReadme.md) on how to adjust XCP_RESOURCE_PROTECTION if needed */
#define XCP_RESOURCE_PROTECTION 0x05U /* Seed & key enabled for measurement, calibration and page switch */

/* See documentation (XcpExampleReadme.md) on how seed & key algorithm is handled */
// This implementation assumes that seed and key have the same length.
#define XCP_SEED_KEY_LENGTH 2

#define CLIENT_TIMEOUT 120000U  // Maximum time between GET_STATUS commands. Encoding: 100us per bit

#ifdef __cplusplus
}
#endif

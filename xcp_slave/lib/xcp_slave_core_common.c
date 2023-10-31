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

#include "xcp_slave_core_common.h"

bool XcpSlaveCoreIsAddrInArea(const void* area_start, const void* area_end, const void* addr) {
    return (addr >= area_start) && (addr <= area_end); /* polyspace DEFECT:PTR_TO_DIFF_ARRAY [Not a defect:Unset] "These
                                                          are from the linker => will be correct" */
}

void XcpSlaveCoreWriteNegativResponse(const uint8_t response_code, uint8_t* const responseBuf, uint16_t* responseSize) {
    responseBuf[RESPONSE_TYPE_IDX] = NEGATIVE_RESPONSE;
    responseBuf[ERROR_CODE_IDX] = response_code;
    *responseSize = 2;
}

bool XcpSlaveCoreIsBlockInArea(const void* area_start,
                               const void* area_end,
                               const void* block_start,
                               const uint32_t block_size) {
    return XcpSlaveCoreIsAddrInArea(area_start, area_end, block_start) &&
           XcpSlaveCoreIsAddrInArea(area_start, area_end, (void*)((uint64_t)block_start + block_size - 1));
}

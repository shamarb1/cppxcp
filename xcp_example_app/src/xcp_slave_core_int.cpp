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

#include <stdio.h>   // For NULL
#include <stdlib.h>  // For rand and srand
#include <time.h>
#include <xcp.h>

void XcpSlaveCoreGetSeedCalPag(unsigned char* const dst, const unsigned char dstSize) {
    // Generate a random 2 byte long seed
    srand(time(NULL)); /* polyspace DEFECT:RAND_SEED_PREDICTABLE [Not a defect:Unset] "Return value
                          of time() isn't constant" */
    for (unsigned char i = 0; i < dstSize; i++) {
        dst[i] = (unsigned char)
                rand(); /* polyspace DEFECT:VULNERABLE_PRNG [Not a defect:Unset] "Good enough for this purpose" */
    }
}

void XcpSlaveCoreGetSeedDaq(unsigned char* const dst, const unsigned char dstSize) {
    // Generate a random 2 byte long seed
    srand(time(NULL)); /* polyspace DEFECT:RAND_SEED_PREDICTABLE [Not a defect:Unset] "Return value
                          of time() isn't constant" */
    for (unsigned char i = 0; i < dstSize; i++) {
        dst[i] = (unsigned char)
                rand(); /* polyspace DEFECT:VULNERABLE_PRNG [Not a defect:Unset] "Good enough for this purpose" */
    }
}

void XcpSlaveCoreCalPag(const unsigned char* const seed,
                        const unsigned char seedSize,
                        unsigned char* const key,
                        const unsigned char keySize) {
    // key == seed in our example
    for (unsigned char i = 0; i < keySize && i < seedSize; i++) {
        key[i] = seed[i];
    }
}

void XcpSlaveCoreCalcKeyDaq(const unsigned char* seed,
                            const unsigned char seedSize,
                            unsigned char* key,
                            const unsigned char keySize) {
    // key == seed in our example
    for (unsigned char i = 0; i < keySize && i < seedSize; i++) {
        key[i] = seed[i];
    }
}

// Timestamp related defines
#define SCALE_S_TO_100US 10000        // Factor to convert seconds to 100 microseconds
#define SCALE_NS_TO_100US_INV 100000  // The inverse of the factor to convert nanoseconds to 100 microseconds

unsigned int XcpSlaveCoreGetEpochTime100us(void) {
    struct timespec now;
    (void)clock_gettime(CLOCK_MONOTONIC, &now); /* Casting to void to avoid polyspace warning.
                                                   clock_gettime won't fail*/
    return (unsigned int)(((long long)now.tv_sec * SCALE_S_TO_100US) +
                          ((long long)now.tv_nsec / SCALE_NS_TO_100US_INV));
}

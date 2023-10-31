/**
 * Copyright (C) 2023 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical concepts) contained herein is, and remains the
 * property of or licensed to Volvo Car Corporation. This information is proprietary to Volvo Car Corporation and may be
 * covered by patents or patent applications. This information is protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Volvo Car Corporation.
 */

#ifndef XCP_XCP_TYPES
#define XCP_XCP_TYPES

#include "common_types.hpp"

// TODO: reimplement according to the Protocol specification.
class Packet {
  public:
    byte* serialize(byte* bytes) const { return bytes; }
    const byte* deserialize(const byte* bytes) { return bytes; }
    size_t getNetworkSize() const { return 0; }

  private:
    int dummy = 0;
};

#endif

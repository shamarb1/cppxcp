/*
 * Copyright (C) 2019 - Volvo Car Corporation
 *
 * All Rights Reserved
 *
 * LEGAL NOTICE:  All information (including intellectual and technical
 * concepts) contained herein is, and remains the property of or licensed to
 * Volvo Car Corporation. This information is proprietary to Volvo Car
 * Corporation and may be covered by patents or patent applications. This
 * information is protected by trade secret or copyright law. Dissemination of
 * this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Volvo Car Corporation.
 */

#ifndef XCP_EXAMPLE_APP_HPP_
#define XCP_EXAMPLE_APP_HPP_

#include "application.hpp"

/**
 * \brief Class inheriting Application. Prints "Xcp Example App".
 */
class XcpExampleApp : public csp::afw::core::Executable {
  public:
    XcpExampleApp();
    void OnWillStart() override;

  private:
    csp::afw::core::TimerHandle timer_;
};

#endif

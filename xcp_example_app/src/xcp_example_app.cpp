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

/**********************************************************************************************************************/

#include "xcp_example_app.hpp"

#include "business_logic.hpp"

// Constructor
XcpExampleApp::XcpExampleApp() : Executable("xcpExampleApp") {}

// Lifecycle callback called when the executable is about to start.
void XcpExampleApp::OnWillStart() {
    /* Register a timer that will execute every 100ms. The timer will run
       after OnWillStart has finished so we must not capture any local
       variables. The timer will not trigger after ExecutionContext::Exit
       has been invoked. The timer-callback will run on the main thread, so
       no extra thread synchronization is needed with other timers. Its
       also possible to use std::bind to call a function on the executable
       instead of using a lambda expression which is preferred when the
       callback function has many lines.
       The timer handle must be saved because the timer is canceled when
       the timer handle is destructed. */
    timer_ = context()->dispatch_queue()->ScheduleWithInterval(std::chrono::milliseconds(10), [] { business_logic(); });
}

/*
void UserLog(const char* const stText, void* const user_data) {
    (static_cast<csp::log::LogContext*>(user_data))->LogInfo() << "xcp-slave: " << stText;
}*/

void UserLog(const char* const stText, void* const user_data) {
    (void)user_data;
    std::cout << "xcp-slave: " << stText << "\n";
}

int main(int argc, char** argv) {
    csp::log::LogContext app_logger("app", "xcp test app");
    csp::log::LogApplication::instance().Register("app", "xcp test app");
    csp::log::LogApplication::instance().SetLogLevel(csp::log::LogLevel::kDebug);

    init_data_t app_init_data = XCP_CREATE_INIT_DATA();

    // uncomment for logs
    app_init_data.log_level = 2;
    app_init_data.log_user_data = nullptr;

    if (argc > 2) {
        app_init_data.calibration_data_filepath = argv[1];  // The absolute path of the CalData File
        app_init_data.checksum_filepath = argv[2];          // The absolute path of the Checksum File
        app_init_data.is_calibration_used = 1;
    }

    app_logger.LogInfo() << "call for XcpInit\n";

    // Initiaize XCP and start server
    // Note that the A2l file must be updated to match this port
    // The port needs to be unique for each XCP server.
    // Register yourself at https://c1.confluence.cm.volvocars.biz/display/ARTVDDP/Apps+using+XCP-slave
    XcpInit(&app_init_data);

    return csp::afw::core::Application::instance().Execute(std::make_shared<XcpExampleApp>());
}

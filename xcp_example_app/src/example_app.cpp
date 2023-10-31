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

#include <time.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "business_logic.hpp"

extern uint8_t printTS;

void UserLog(const char* const stText, void* const user_data) {
    (void)user_data;

    if (printTS) {
        tm localtime_struct;
        timespec ts;
        int retcode = clock_gettime(CLOCK_REALTIME, &ts);
        (void)retcode;
        tm* retptr = localtime_r(&ts.tv_sec, &localtime_struct);
        (void)retptr;
        const char* time_format_str = "%Y-%b-%d %H:%M:%S";
        char time_str[100];  // eg : [2022-Oct-14 09:34:39.160331]
        size_t bytes_written = strftime(time_str, sizeof(time_str), time_format_str, &localtime_struct);
        (void)bytes_written;
        printf("xcp-slave: [%s.%06ld] %s \n", time_str, (ts.tv_nsec / 1000), stText);
    } else {
        printf("xcp-slave:                               %s \n", stText);
    }
}

int main(int argc, char** argv) {
    using namespace std;
    using namespace std::chrono_literals;

    init_data_t app_init_data = XCP_CREATE_INIT_DATA();

    // uncomment for logs
    app_init_data.log_level = 1;
    app_init_data.log_user_data = nullptr;

    if (argc > 2) {
        app_init_data.calibration_data_filepath = argv[1];  // The absolute path of the CalData File
        app_init_data.checksum_filepath = argv[2];          // The absolute path of the Checksum File
    }
    // Note that the A2l file must be updated to match this port
    // The port needs to be unique for each XCP server.
    // Register yourself at https://c1.confluence.cm.volvocars.biz/display/ARTVDDP/Apps+using+XCP-slave
    XcpInit(&app_init_data);

    std::cout << "Press Ctrl+C to exit\n" << std::flush;
    while (true) {
        business_logic();
        std::this_thread::sleep_for(10ms);
    }
}

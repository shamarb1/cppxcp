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

#include "xcp_slave_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "xcp_globals.h"

#define MAX_LOG_MSG_SIZE 256

extern void UserLog(const char* const stText, void* const user_data);
unsigned char printTS = 0;

void XcpSlavePrint(const char* format, ...) {
    char debug_text_buffer[MAX_LOG_MSG_SIZE];

    va_list args;
    va_start(args, format);
    vsnprintf(debug_text_buffer, MAX_LOG_MSG_SIZE, format, args);
    UserLog(debug_text_buffer, user_data);
    va_end(args);
}

void XcpSlavePrintfVerbose(const char* format, ...) {
    if (log_level > 1) {
        char debug_text_buffer[MAX_LOG_MSG_SIZE];

        va_list args;
        va_start(args, format);
        vsnprintf(debug_text_buffer, MAX_LOG_MSG_SIZE, format, args);
        UserLog(debug_text_buffer, user_data);
        va_end(args);
    }
}

void XcpSlavePrintfVerboseDebug(const char* format, ...) {
    if (log_level > 2) {
        char debug_text_buffer[MAX_LOG_MSG_SIZE];

        va_list args;
        va_start(args, format);
        vsnprintf(debug_text_buffer, MAX_LOG_MSG_SIZE, format, args);
        UserLog(debug_text_buffer, user_data);
        va_end(args);
    }
}

void XcpSlavePrintfVerbosePayload(int direction, const unsigned char* p_data, const unsigned length) {
    if (log_level > 0) {
        char p_log_hex[128] = {0};
        unsigned int p_log_hex_len = 0;
        unsigned int line_counter = 0;
        unsigned int byte_counter = 0;

        while (byte_counter < length) {
            sprintf(&p_log_hex[p_log_hex_len], "%02X ", *(p_data++));
            p_log_hex_len += 3;

            byte_counter++;
            if (byte_counter > 1 && (byte_counter % 25 == 0)) {
                // ensure that our strings null-terminated
                p_log_hex[--p_log_hex_len] = 0;
                if (line_counter == 0) {
                    printTS = 1;
                    XcpSlavePrint("%s [%4dd] [%s", direction ? "-->" : "<--", length, p_log_hex);
                } else {
                    printTS = 0;
                    XcpSlavePrint("%s", p_log_hex);
                }
                memset(p_log_hex, 0, 128);
                memset(p_log_hex, ' ', 13);
                line_counter++;
                p_log_hex_len = 13;
            }
        }

        if (p_log_hex_len > 0) {
            // ensure that our strings null-terminated
            p_log_hex[--p_log_hex_len] = 0;
            if (line_counter == 0) {
                printTS = 1;
                XcpSlavePrint("%s [%4dd] [%s]", direction ? "-->" : "<--", length, p_log_hex);
            } else {
                printTS = 0;
                XcpSlavePrint("%s]", p_log_hex);
            }
            memset(p_log_hex, 0, 128);
            line_counter++;
        }
    }
}

void XcpSlavePrintfVerboseUint8Array(const char* p_header, const unsigned char* p_data, const unsigned length) {
    if (log_level > 1) {
        // pattern
        char p_hex_arr[] = "0123456789abcdef";

        // hex representation string
        char p_log_hex[128] = {0};
        unsigned int p_log_hex_len = 0;

        // ascii representation string
        char p_log_ascii[128] = {0};
        unsigned int p_log_ascii_len = 0;

        // write the header
        XcpSlavePrintfVerbose("array: %s, length: %d", p_header, length);

        // process all bytes
        for (unsigned int i = 0; i < length; ++i) {
            // do we have a line to show?
            if (p_log_hex_len >= 30) {
                // ensure the null is in the end
                p_log_hex[p_log_hex_len] = 0;
                p_log_hex_len = 0;

                p_log_ascii[p_log_ascii_len] = 0;
                p_log_ascii_len = 0;

                // write the line
                XcpSlavePrintfVerbose("%s || %s", p_log_hex, p_log_ascii);
            }

            // process byte and make a hex from it
            p_log_hex[p_log_hex_len++] = p_hex_arr[(p_data[i] & 0xf0) >> 4];
            p_log_hex[p_log_hex_len++] = p_hex_arr[p_data[i] & 0x0f];
            p_log_hex[p_log_hex_len++] = ' ';

            // show printable characters, otherwise show a '.'
            p_log_ascii[p_log_ascii_len++] = p_data[i] >= 0x20 ? p_data[i] : '.';
        }

        // write last piece of data
        if (p_log_hex_len > 0) {
            // shift the end of the string to allign the second part
            memset(&p_log_hex[p_log_hex_len], ' ', 30 - p_log_hex_len);
            p_log_hex_len = 30;

            // ensure that our strings null-terminated
            p_log_hex[p_log_hex_len] = 0;
            p_log_ascii[p_log_ascii_len] = 0;

            XcpSlavePrintfVerbose("%s || %s ", p_log_hex, p_log_ascii);
        }
    }
}

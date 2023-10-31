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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // make sure dladdr is declared
#endif
#include "xcp.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>

#include "xcp_globals.h"
#include "xcp_slave_cal_page_file_io.h"
#include "xcp_slave_core.h"
#include "xcp_slave_log.h"

//********* Local Defines******************************************************************************************

// XCP packet stuffing related defines
#define PAYLOAD_LSB_IDX 0U  // XCP header length, least significant byte
#define PAYLOAD_MSB_IDX 1U  // XCP header length, most significant byte
#define PKT_CTR_LSB_IDX 2U  // XCP packet counter least significant byte
#define PKT_CTR_MSB_IDX 3U  // XCP packet counter most significant byte
#define PKT_PID_NR_IDX 4U   // XCP packet PID number index
#define TIMESTAMP_0 5U      // XCP packet (ODT) sample time least significant byte

// Ethernet related defines
#define LISTEN_BACKLOG 5   // Max length to which the queue of pending connections for sockfd may grow
#define SEND_FLAG 0        // 4:th argument in send() function call. No flags set
#define RECV_FLAG 0        // 4:th argument in recv() function call. No flags set
#define SOCKET_PROTOCOL 0  // Socket protocol used in socket() function calls
#define SOCKET_FAILED -1   // The return value of socket() function will indicate an error with '-1'
#define BIND_FAILED -1     // The return value of bind() function will indicate an error with '-1'
#define LISTEN_FAILED -1   // The return value of listen() function will indicate an error with '-1'
#define RECV_FAILED -1     // The return value of recv() function will indicate an error with '-1'
#define SEND_FAILED -1     // The return value of send() function will indicate an error with '-1'
#define FCNTL_FAILED -1    // The return value of fcntl() function will indicate an error with '-1'
#define SEND_SUCCESS 0     // The send was a success or send has not been called for current client
#define SA_DATA_PORT_LSB 1
#define SA_DATA_PORT_MSB 0

// Generic error code defines
#define CODE_OK_GENERIC 0      // Generic code to indicate success
#define CODE_ERROR_GENERIC -1  // Generic code to indicate error/failure

// 2 Sec timeout for the SendTo buffer
#define SENDTO_TIMEOUT_SEC 2

int32_t SockFileDescr;  // socket file descriptor

extern uint8_t responseBuf[XCP_MAX_CTO];
extern uint16_t responseSizeExternal;

uint32_t prev_sendTo_time = 0;
uint8_t is_first_sample = 1;

struct node {
    int32_t sock_file_descr;       // Socket file descriptor, return value from socket()
    struct sockaddr from;          // Source information for XCP client
    uint32_t last_cmd_epoch_time;  // EPOCH time at the time for the last command reception
    uint16_t xcp_pkt_ctr;          // keeps track of how many packes we send, used to identify dropped packages
    uint16_t xcp_offs;             // Index for ethernet UDP data buffer "udp_data"
    uint8_t udp_data[XCP_MTU];     // Buffer for data to be sent in ethernet UDP packet

    struct XcpSlaveCoreClient core_client;
};

static struct node client[XCP_MAX_NR_OF_CLIENTS];

/**
 * @brief Initializes a client struct.
 *
 * @param pclient Client to initialize.
 */
static void InitClient(struct node* pclient) {
    pclient->sock_file_descr = -1;
    memset(&pclient->from, 0, sizeof(pclient->from));
    pclient->last_cmd_epoch_time = 0U;

    pclient->xcp_pkt_ctr = 0U;
    pclient->xcp_offs = 0U;

    XcpSlaveCoreInitClient(&pclient->core_client);
}

/**
 * @brief Close an initialized client making it ready for reuse.
 *
 * @param pclient Client to initialize.
 */
static void CloseClient(struct node* pclient) {
    XcpSlaveCoreCloseClient(&pclient->core_client);
    InitClient(pclient);
}

int XcpSlaveCoreSendPacket(const int8_t client_id,
                           const uint8_t* data,
                           const uint16_t dataLen,
                           const XcpPacketTxMethod txMethod) {
    ssize_t send_stat = 0;  // Return value from sendto() function
    int data_sent = CODE_ERROR_GENERIC;
    struct node* const pclient = &client[client_id];

    pclient->xcp_pkt_ctr++;

    if (is_first_sample) {
        is_first_sample = 0;
        prev_sendTo_time = XcpSlaveCoreGetEpochTime100us();
    }
    uint8_t time_diff = (XcpSlaveCoreGetEpochTime100us() - prev_sendTo_time) / 10000;

    // Send buf if adding new data would exceed buffer size.
    // 2 + 2 for payload + packet conter.
    if (dataLen + 2U + 2U + pclient->xcp_offs >= XCP_MTU ||
        (pclient->xcp_offs > 0 && time_diff >= SENDTO_TIMEOUT_SEC)) {
        XcpSlavePrintfVerbosePayload(0, (const unsigned char*)&pclient->udp_data, pclient->xcp_offs);
        send_stat = sendto(pclient->sock_file_descr,
                           pclient->udp_data,
                           (size_t)pclient->xcp_offs,
                           SEND_FLAG,
                           (struct sockaddr*)&pclient->from,
                           sizeof(pclient->from));
        if (SEND_FAILED == send_stat) {
            XcpSlavePrintfVerbose("Send failed, closing client: %d", pclient->core_client.client_id);
            CloseClient(pclient);
        } else {
            data_sent = CODE_OK_GENERIC;
            pclient->xcp_offs = 0U;
            prev_sendTo_time = XcpSlaveCoreGetEpochTime100us();
        }
    }
    if (SEND_FAILED != send_stat) {
        // Add current packet to buffer.
        WRITE_UINT16_TO_UINT8_ARRAY(&pclient->udp_data[pclient->xcp_offs], dataLen);
        WRITE_UINT16_TO_UINT8_ARRAY(&pclient->udp_data[pclient->xcp_offs + 2], pclient->xcp_pkt_ctr);

        memcpy(&pclient->udp_data[pclient->xcp_offs + 4], data, dataLen);
        pclient->xcp_offs += 2 + 2 + dataLen;

        if (txMethod == XcpPacketTxMethod_IMMEDIATE) {
            XcpSlavePrintfVerbosePayload(0, (const unsigned char*)&pclient->udp_data, pclient->xcp_offs);
            send_stat = sendto(pclient->sock_file_descr,
                               pclient->udp_data,
                               (size_t)pclient->xcp_offs,
                               SEND_FLAG,
                               (struct sockaddr*)&pclient->from,
                               sizeof(pclient->from));
            if (SEND_FAILED == send_stat) {
                XcpSlavePrintfVerbose("Send failed, closing client: %d", pclient->core_client.client_id);
                CloseClient(pclient);
            } else {
                data_sent = CODE_OK_GENERIC;
                pclient->xcp_offs = 0U;
            }
        } else {
            // Intentionally empty
        }
    } else {
        // First send fail, no need to try more stuff.
    }
    return data_sent;
}

/**
 * @brief MasterSocket(socket that listens for connections)
 */
int XcpServerSocketSetup(void) {
    int32_t bind_result;
    struct sockaddr_in server;
    int socket_flags;
    const uint32_t ip_tos_value = 0U;  // Set network-level priority to 0 (lowest)
    const ssize_t set_priority_fail = -1;
    ssize_t sock_priority_status;

    int xcp_server_satus = CODE_OK_GENERIC;

    XcpSlavePrintfVerbose("Starting upp UDP server");
    XcpSlavePrintfVerbose("xcp_slave: server_port = %d", server_port);
    // Clear in socket struct
    memset((uint8_t*)&server, 0U, sizeof(server));
    // Fill in socket struct
    server.sin_family = AF_INET;                                   // IPv4
    server.sin_addr.s_addr = INADDR_ANY;                           // Listen to any IP address
    server.sin_port = htons(server_port);                          // which telnet port number to listen to
    SockFileDescr = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);  // create a socket
    if (SOCKET_FAILED == SockFileDescr) {
        XcpSlavePrintfVerbose("Socket errno: %s", strerror(errno));
        xcp_server_satus = CODE_ERROR_GENERIC;
    } else {
        XcpSlavePrintfVerbose("Socket File Descriptor: %d", SockFileDescr);
    }
    socket_flags = fcntl(SockFileDescr, F_GETFL, 0);  // get socket flags
    if (FCNTL_FAILED == socket_flags) {
        XcpSlavePrintfVerbose("Could not get socket flags via fcntl() call \n");
        xcp_server_satus = CODE_ERROR_GENERIC;
    }
    bind_result = bind(SockFileDescr, (struct sockaddr*)&server, sizeof(server));  // bind socket to a port
    if (BIND_FAILED == bind_result) {
        XcpSlavePrintfVerbose("Bind socket nok: %s", strerror(errno));
        xcp_server_satus = CODE_ERROR_GENERIC;
    } else {
        XcpSlavePrintfVerbose("Bind socket ok, %d", bind_result);
    }
    sock_priority_status = setsockopt(SockFileDescr, IPPROTO_IP, IP_TOS, &ip_tos_value, sizeof(ip_tos_value));
    if (sock_priority_status == set_priority_fail) {
        XcpSlavePrintfVerbose("Failed to set low socket priority");
        xcp_server_satus = CODE_ERROR_GENERIC;
    } else {
        XcpSlavePrintfVerbose("Network priority : Lowest");
    }

    return xcp_server_satus;
}

/**
 * @brief Receive XCP command packets from socket and take actions
 */
void* XcpReceiveCommandsInThread(void* thread_input) {
    uint8_t i;                                            // For use in loops
    ssize_t send_stat;                                    // Return value from sendto() function
    uint8_t cto_recv_data[XCP_MAX_CTO + XCP_HEADER_LEN];  // Buffer for CTO recived data
    ssize_t num_bytes_received;
    uint32_t time;
    struct sockaddr from;
    socklen_t from_len = sizeof(from);
    struct node* pclient;
    struct timespec request;
    struct timespec remaining;
    request.tv_sec = 1;
    request.tv_nsec = 0;

    while (true) {
        pclient = NULL;
        num_bytes_received = recvfrom(
                SockFileDescr, cto_recv_data, sizeof(cto_recv_data), RECV_FLAG, (struct sockaddr*)&from, &from_len);
        if (SOCKET_FAILED == num_bytes_received) {
            XcpSlavePrintfVerbose("Socket errno: %d", errno);
            // Try recvfrom again after 1 second
            nanosleep(&request, &remaining);
            continue;
        }
        time = XcpSlaveCoreGetEpochTime100us();  // Get current EPOCH time
        for (i = 0; i < XCP_MAX_NR_OF_CLIENTS; i++) {
            if ((client[i].from.sa_data[SA_DATA_PORT_MSB] == from.sa_data[SA_DATA_PORT_MSB]) &&
                (client[i].from.sa_data[SA_DATA_PORT_LSB] == from.sa_data[SA_DATA_PORT_LSB])) {
                pclient = &client[i];
                pclient->last_cmd_epoch_time = time;  // Heartbeat timestamp for CTO command
                break;                                // Break if we find that client is already connected
            }
        }
        if (NULL == pclient) {
            // This is a new client so see if there is available client positions in the client list
            for (i = 0; i < XCP_MAX_NR_OF_CLIENTS; i++) {  // Loop through the available client structs
                if (XcpSlaveCoreIsClientFree(
                            &client[i].core_client)) {  // Pick a free client struct if one is available
                    pclient = &client[i];
                    pclient->last_cmd_epoch_time = time;  // Heartbeat timestamp for CTO command
                    pclient->core_client.client_id = i;   // ID is set to the struct array index
                    pclient->sock_file_descr = SockFileDescr;
                    pclient->from = from;  // Source information of XCP client
                    uint16_t port_nr = (uint16_t)((uint8_t)pclient->from.sa_data[SA_DATA_PORT_MSB]) << 8;
                    port_nr = port_nr + (uint16_t)(pclient->from.sa_data[SA_DATA_PORT_LSB] & 0x00FF);
                    XcpSlavePrintfVerbose("This is client number %d, port %d", pclient->core_client.client_id, port_nr);
                    break;  // One break per loop is OK according to MISRA
                }
            }
        }
        if (NULL == pclient) {
            // Client list is full but see if there are any abandoned clients that can be thrown out
            uint32_t client_idle_dur_max = 0U;  // Longest idle time so far in the client abandoned search
            uint8_t aban_client = 0xFF;         // Init to 0xFF to detect if no active clients are abandoned
            for (i = 0; i < XCP_MAX_NR_OF_CLIENTS; i++) {
                //     Find abandon_pending clients          handle possible underflow
                if ((true == client[i].core_client.abandon_pending) && (time >= CLIENT_TIMEOUT)) {
                    // Set last_cmd_epoch_time to the lowest value valid for client throw-out so that
                    // abandon_pending gets lowest priority if there are other abandoned clients
                    client[i].last_cmd_epoch_time = time - CLIENT_TIMEOUT;
                }
                // Find client that has been on hold for the longest time
                if ((time - client[i].last_cmd_epoch_time) > client_idle_dur_max) {
                    client_idle_dur_max = time - client[i].last_cmd_epoch_time;
                    aban_client = i;
                }
            }
            if (CLIENT_TIMEOUT <= client_idle_dur_max) {
                // Client is thrown out if the longest idling client has been passive for longer than the timeout or
                // if (with lower priority) a client has the abandon_pending flag set
                {
                    uint16_t port_nr = (uint16_t)((uint8_t)client[aban_client].from.sa_data[SA_DATA_PORT_MSB]) << 8;
                    port_nr = port_nr + (uint16_t)(client[aban_client].from.sa_data[SA_DATA_PORT_LSB] & 0x00FF);
                    XcpSlavePrintfVerbose(
                            "Client nr %d, port %d  disconnected due to inactivity", aban_client, port_nr);
                }
                //..and replaced by the new client
                pclient = &client[aban_client];
                CloseClient(pclient);
                pclient->core_client.client_id = aban_client;
                pclient->sock_file_descr = SockFileDescr;
                pclient->last_cmd_epoch_time = time;  // Heartbeat timestamp for CTO command
                pclient->from = from;                 // Source information of XCP client
                {
                    uint16_t port_nr = (uint16_t)((uint8_t)pclient->from.sa_data[SA_DATA_PORT_MSB]) << 8;
                    port_nr = port_nr + (uint16_t)(pclient->from.sa_data[SA_DATA_PORT_LSB] & 0x00FF);
                    XcpSlavePrintfVerbose("and replaced with port number %d", port_nr);
                }
            }
        }
        if (NULL == pclient) {
            XcpSlavePrintfVerbose("Oops, no free client structs!");
            // No available client slots, fire away a polite goodbye
            uint8_t buf[] = {0, 0, 0, 0, NEGATIVE_RESPONSE, ERR_RESOURCE_TEMPORARY_NOT_ACCESSIBLE};
            WRITE_UINT16_TO_UINT8_ARRAY(&buf[PAYLOAD_LSB_IDX], 2);  // Payload size.
            WRITE_UINT16_TO_UINT8_ARRAY(&buf[PKT_CTR_LSB_IDX], 0);  // Packet counter.
            XcpSlavePrintfVerbosePayload(1, (const unsigned char*)buf, (*(const uint16_t*)(buf)) + 4);
            send_stat = sendto(SockFileDescr, buf, sizeof(buf), SEND_FLAG, (struct sockaddr*)&from, sizeof(from));
            if (SEND_FAILED == send_stat) {
                XcpSlavePrintfVerbose("Sending negative response due to no free client slots failed");
            }
        } else if (num_bytes_received > 0) {
            XcpSlavePrintfVerbosePayload(
                    1, (const unsigned char*)cto_recv_data, (*(const uint16_t*)(cto_recv_data)) + 4);
            XcpSlaveCoreProcessCmd(
                    cto_recv_data + XCP_HEADER_LEN, *(const uint16_t*)cto_recv_data, &pclient->core_client);

            uint8_t sendBuf[XCP_MTU];
            memset(sendBuf, 0, XCP_MTU);
            WRITE_UINT16_TO_UINT8_ARRAY(&sendBuf[PAYLOAD_LSB_IDX], responseSizeExternal);  // Payload size.
            sendBuf[PKT_CTR_MSB_IDX] = cto_recv_data[PKT_CTR_MSB_IDX];                     // Msg Counter LSB
            sendBuf[PKT_CTR_LSB_IDX] = cto_recv_data[PKT_CTR_LSB_IDX];                     // Msg Counter MSB
            memcpy(&sendBuf[4], responseBuf, responseSizeExternal);

            if (responseSizeExternal > 0) {
                XcpSlavePrintfVerbosePayload(0, (const unsigned char*)sendBuf, 4 + responseSizeExternal);
                send_stat = sendto(SockFileDescr,
                                   sendBuf,
                                   responseSizeExternal + 4,
                                   SEND_FLAG,
                                   (struct sockaddr*)&from,
                                   sizeof(from));
            }
            if (pclient->core_client.disconnect_after_response == true) {
                // No need to flush tx queue because all responses from process_cmd always send immediately.
                CloseClient(pclient);
            }
        }
    }
    return thread_input;
}

/**
 * @brief Send the DTO's allocated to the event stated by argument 'event'(called by app using the XCP server)
 *
 * @param event number stated by EVENT_CHANNEL_NUMBER in the corresponding EVENT block in the a2l-file
 */
void XcpEvent(const unsigned char event) {
    uint32_t i;
    for (i = 0U; i < XCP_MAX_NR_OF_CLIENTS; i++) {
        XcpSlaveCoreOnEvent(event, &client[i].core_client);
    }
}

/**
 * @brief Handles load or creation of reference and working page files and initialize XCP server
 *
 */
void XcpInit(const init_data_t* const app_init_data) {
    const char* xcp_disabled_environment_variable = "XCP_DISABLED";
    char* is_xcp_disabled = getenv(xcp_disabled_environment_variable);
    if (is_xcp_disabled != NULL) {
        // XCP_DISABLED environment variable is defined, go ahead and check value
        if (strcmp(is_xcp_disabled, "ON") == 0) {
            // XCP_DISABLED is set to "ON", do not initialize xcp server
            return;
        }
    }
    Dl_info info;
    uint32_t i;  // For use in loops
    for (i = 0U; i < XCP_MAX_NR_OF_CLIENTS; i++) {
        InitClient(&client[i]);
    }
    server_port = app_init_data->server_port;
    text_section_start = app_init_data->text_section_start;
    text_section_end = app_init_data->text_section_end;
    bss_section_start = app_init_data->bss_section_start;
    bss_section_end = app_init_data->bss_section_end;
    data_section_start = app_init_data->data_section_start;
    data_section_end = app_init_data->data_section_end;

    is_calibration_used = app_init_data->is_calibration_used;

    // set default value for fields that were added in version 2
    is_calibration_data_from_file = 0;
    memset(calibration_data_filepath, 0, sizeof(calibration_data_filepath));
    memset(checksum_filepath, 0, sizeof(checksum_filepath));

    // check, do we have version 2 or never
    if (app_init_data->struct_version > 1) {
        is_calibration_data_from_file =
                (app_init_data->calibration_data_filepath != NULL) && (app_init_data->checksum_filepath != NULL);

        // save file path if needed
        if (is_calibration_data_from_file == 1) {
            strncpy(calibration_data_filepath,
                    app_init_data->calibration_data_filepath,
                    CALIBRATION_FILE_PATH_SIZE - 1);
            strncpy(checksum_filepath, app_init_data->checksum_filepath, CALIBRATION_FILE_PATH_SIZE - 1);
        }
    }

    caldata_section_start = app_init_data->caldata_section_start;
    caldata_section_end = app_init_data->caldata_section_end;

    user_data = app_init_data->log_user_data;
    log_level = app_init_data->log_level;

    dladdr(app_init_data->proc_base_address, &info);

    XcpSlaveCoreInit((uint64_t)info.dli_fbase);

    XcpSlavePrintfVerbose("xcp_slave: Initialize mutex free, non blocking socket version of XCP server");
    XcpSlavePrintfVerbose("xcp_slave: Base address = 0x%lX", (&info)->dli_fbase);
    XcpSlavePrintfVerbose("xcp_slave: Text address = 0x%lX", text_section_start);
    XcpSlavePrintfVerbose("xcp_slave: Data address = 0x%lX", data_section_start);
    XcpSlavePrintfVerbose("xcp_slave: Bss  address = 0x%lX", bss_section_start);

    if (is_calibration_used) {
        XcpSlavePrintfVerbose("xcp_slave: Cal  address = 0x%lX", caldata_section_start);

        if (is_calibration_data_from_file) {
            XcpSlavePrintfVerbose("xcp_slave: Calibration Data loaded from file @ %s", calibration_data_filepath);
            XcpSlavePrintfVerbose("xcp_slave: Checksum are referred from file @ %s", checksum_filepath);
        }
    }

    // Set variable for internal testing
    if (app_init_data->struct_version > 2) {
        (*(init_data_t**)&(app_init_data))->xcp_init_status = XcpServerSocketSetup();
    } else {
        // Struct version <= 2, set up the socket without recording the status of the setup
        XcpServerSocketSetup();
    }

    // Launch thread to receive commands
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, XcpReceiveCommandsInThread, NULL);
    const char* recv_thread_name = "xcp_recv_thread";
    pthread_setname_np(thread_id, recv_thread_name);
    pthread_detach(thread_id);
}

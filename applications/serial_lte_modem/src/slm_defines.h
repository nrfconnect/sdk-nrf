/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_ip.h>
#include <nrf_socket.h>

#ifndef SLM_AT_DEFINES_
#define SLM_AT_DEFINES_

#define INVALID_SOCKET       -1
#define INVALID_SEC_TAG      -1
#define INVALID_ROLE         -1

/** The maximum allowed length of an AT command/response passed through the SLM */
#define SLM_AT_MAX_CMD_LEN   4096
#define SLM_AT_MAX_RSP_LEN   2100

/** The maximum allowed length of data send/receive through the SLM */
#define SLM_MAX_PAYLOAD_SIZE 1024 /** max size of payload sent in command mode */
#define SLM_MAX_MESSAGE_SIZE NRF_SOCKET_TLS_MAX_MESSAGE_SIZE

#define SLM_MAX_SOCKET_COUNT 8    /** re-define NRF_MODEM_MAX_SOCKET_COUNT */

#define SLM_MAX_URL          128  /** max size of URL string */
#define SLM_MAX_FILEPATH     128  /** max size of filepath string */
#define SLM_MAX_USERNAME     32   /** max size of username in login */
#define SLM_MAX_PASSWORD     32   /** max size of password in login */

#define SLM_NRF52_BLK_SIZE   4096 /** nRF52 flash block size for write operation */
#define SLM_NRF52_BLK_TIME   2000 /** nRF52 flash block write time in millisecond (1.x second) */

#endif /* SLM_AT_DEFINES_ */

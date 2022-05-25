/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_ip.h>

#ifndef SLM_AT_DEFINES_
#define SLM_AT_DEFINES_

#define INVALID_SOCKET       -1
#define INVALID_SEC_TAG      -1
#define INVALID_ROLE         -1

#define SLM_AT_CMD_RESPONSE_MAX_LEN 2100 /** re-define CONFIG_AT_CMD_RESPONSE_MAX_LEN */
#define SLM_MAX_SOCKET_COUNT 8    /** re-define NRF_MODEM_MAX_SOCKET_COUNT */

#define SLM_MAX_URL          128  /** max size of URL string */
#define SLM_MAX_USERNAME     32   /** max size of username in login */
#define SLM_MAX_PASSWORD     32   /** max size of password in login */

#define MODEM_MTU            1280
#define TCP_MAX_PAYLOAD_IPV4 (MODEM_MTU - NET_IPV4TCPH_LEN)
#define UDP_MAX_PAYLOAD_IPV4 (MODEM_MTU - NET_IPV4UDPH_LEN)
#define TCP_MAX_PAYLOAD_IPV6 (MODEM_MTU - NET_IPV6TCPH_LEN)
#define UDP_MAX_PAYLOAD_IPV6 (MODEM_MTU - NET_IPV6UDPH_LEN)
#define SLM_MAX_PAYLOAD      (MODEM_MTU - NET_IPV4UDPH_LEN)
#define SLM_MAX_PAYLOAD6     (MODEM_MTU - NET_IPV6UDPH_LEN)

#define SLM_NRF52_BLK_SIZE   4096 /** nRF52 flash block size for write operation */
#define SLM_NRF52_BLK_TIME   2000 /** nRF52 flash block write time in millisecond (1.x second) */

#endif /* SLM_AT_DEFINES_ */

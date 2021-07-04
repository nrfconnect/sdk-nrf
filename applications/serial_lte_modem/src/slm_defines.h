/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/net_ip.h>

#ifndef SLM_AT_DEFINES_
#define SLM_AT_DEFINES_

#define INVALID_SOCKET       -1
#define INVALID_SEC_TAG      -1
#define INVALID_ROLE         -1

#define SLM_MAX_URL          128  /** max size of URL string */

#define MODEM_MSS            708
#define MODEM_MTU            1280
#define TCP_MAX_PAYLOAD_IPV4 (MODEM_MSS - NET_IPV4TCPH_LEN)
#define UDP_MAX_PAYLOAD_IPV4 (MODEM_MTU - NET_IPV4UDPH_LEN)
#define TCP_MAX_PAYLOAD_IPV6 (MODEM_MSS - NET_IPV6TCPH_LEN)
#define UDP_MAX_PAYLOAD_IPV6 (MODEM_MTU - NET_IPV6TCPH_LEN)
#define SLM_MAX_PAYLOAD      UDP_MAX_PAYLOAD_IPV4
#define SLM_MAX_PAYLOAD6     UDP_MAX_PAYLOAD_IPV6

#endif /* SLM_AT_DEFINES_ */

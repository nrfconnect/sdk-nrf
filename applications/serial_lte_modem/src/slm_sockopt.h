/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_SOCKOPT_
#define SLM_SOCKOPT_

/** @file slm_sockopt.h
 *
 * @brief SLM socket options for AT-commands.
 * @{
 */

/**
 * @brief Non-secure socket options for XSOCKETOPT.
 */
enum at_sockopt {
	AT_SO_REUSEADDR = 2,
	AT_SO_RCVTIMEO = 20,
	AT_SO_SNDTIMEO = 21,
	AT_SO_SILENCE_ALL = 30,
	AT_SO_IP_ECHO_REPLY = 31,
	AT_SO_IPV6_ECHO_REPLY = 32,
	AT_SO_BINDTOPDN = 40,
	AT_SO_RAI_NO_DATA = 50,
	AT_SO_RAI_LAST = 51,
	AT_SO_RAI_ONE_RESP = 52,
	AT_SO_RAI_ONGOING = 53,
	AT_SO_RAI_WAIT_MORE = 54,
	AT_SO_TCP_SRV_SESSTIMEO = 55,
	AT_SO_RAI = 61,
};

/**
 * @brief Secure socket options for XSSOCKETOPT.
 */
enum at_sec_sockopt {
	AT_TLS_HOSTNAME = 2,
	AT_TLS_CIPHERSUITE_USED = 4,
	AT_TLS_PEER_VERIFY = 5,
	AT_TLS_SESSION_CACHE = 12,
	AT_TLS_SESSION_CACHE_PURGE = 13,
	AT_TLS_DTLS_CID = 14,
	AT_TLS_DTLS_CID_STATUS = 15,
	AT_TLS_DTLS_HANDSHAKE_TIMEO = 18
};

/** @} */
#endif

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TLS_FUNCTIONS_H__
#define TLS_FUNCTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif


#define SERVER_PORT 4243		/* The port number the TLS server will use. */
#define RECV_BUFFER_SIZE 2400		/* Receive-buffer size in bytes. */
#define MAX_CLIENT_QUEUE 1		/* Max number of connections in listen queue. */
#define TLS_PEER_HOSTNAME "localhost"	/* Peer TLS hostname when functioning as client. */


/** @brief Function for starting a TLS or a DTLS thread.
 *
 */
void process_psa_tls(void);

/** @brief Function for starting a TLS or a DTLS thread.
 *
 */
int psa_tls_send_buffer(int sock, const uint8_t *buf, size_t len);


#ifdef __cplusplus
}
#endif

#endif /* TLS_FUNCTIONS_H__ */

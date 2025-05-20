/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_CLIENT_TRANSPORT_H__
#define SSF_CLIENT_TRANSPORT_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief       Notification handler prototype.
 */
typedef void (*ssf_client_transport_notif_handler)(const uint8_t *packet, size_t len);

/**
 * @brief       Initialize underlying transport.
 *
 * @param[in]   notif_handler  Function to be invoked when receiving notifications.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_client_transport_init(ssf_client_transport_notif_handler notif_handler);

/**
 * @brief       Allocate send buffer in underlying transport.
 *
 * @param[out]  buf  A pointer to the allocated tx buffer.
 * @param[in]   len  Size of tx buffer to allocate.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_client_transport_alloc_tx_buf(uint8_t **buf, size_t len);

/**
 * @brief       Free a send buffer previously allocated with ssf_client_transport_alloc_tx_buf.
 *
 * @param[in]  buf  A pointer to the tx buffer to release.
 */
void ssf_client_transport_free_tx_buf(uint8_t *buf);

/**
 * @brief       Free a receive buffer with data from underlying transport.
 *
 * @param[in]   packet  A pointer to the rx buffer to release.
 */
void ssf_client_transport_decoding_done(const uint8_t *packet);

/**
 * @brief       Send a request packet and wait for response to be received. Return response packet.
 *
 * @note        Request packet must be allocated with ssf_client_transport_alloc_tx_buf.
 *              Response packet must be released with ssf_client_transport_decoding_done
 *              after packet have been processed.
 *
 * @param[in]   pkt  A pointer to the request packet to send.
 * @param[in]   pkt_len  Length of the request packet.
 * @param[out]  rsp_pkt  A pointer to the response packet to be receive.
 * @param[out]  rsp_pkt_len  Length of the received response packet.
 *
 * @return      0 on success, otherwise a negative errno.
 */
int ssf_client_transport_request_send(uint8_t *pkt, size_t pkt_len, const uint8_t **rsp_pkt,
				      size_t *rsp_pkt_len);

#endif /* SSF_CLIENT_TRANSPORT_H__ */

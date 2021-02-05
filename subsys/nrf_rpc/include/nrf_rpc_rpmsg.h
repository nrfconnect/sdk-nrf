/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_TR_RPMSG_H_
#define NRF_RPC_TR_RPMSG_H_

#include <stdint.h>
#include <stddef.h>

#include "rp_ll.h"

/**
 * @defgroup nrf_rpc_tr_rpmsg nRF PRC transport using RPMsg
 * @{
 * @brief nRF PRC transport implementation using RPMsg
 *
 * API is compatible with nrf_rpc_tr API. For API documentation
 * @see nrf_rpc_tr_tmpl.h
 */

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_RPC_TR_MAX_HEADER_SIZE 0
#define NRF_RPC_TR_AUTO_FREE_RX_BUF 1

typedef void (*nrf_rpc_tr_receive_handler_t)(const uint8_t *packet, size_t len);

int nrf_rpc_tr_init(nrf_rpc_tr_receive_handler_t callback);

static inline void nrf_rpc_tr_free_rx_buf(const uint8_t *buf)
{
}

#define nrf_rpc_tr_alloc_tx_buf(buf, len)				       \
	uint32_t _nrf_rpc_tr_buf_vla[(sizeof(uint32_t) - 1 + (len)) /	       \
				     sizeof(uint32_t)];			       \
	*(buf) = (uint8_t *)(&_nrf_rpc_tr_buf_vla)

#define nrf_rpc_tr_free_tx_buf(buf)

int nrf_rpc_tr_send(uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NRF_RPC_TR_RPMSG_H_ */

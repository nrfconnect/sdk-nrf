/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define NRF_RPC_LOG_MODULE NRF_RPC_TR
#include <nrf_rpc_log.h>

#include <zephyr.h>
#include <errno.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>
#include <openamp/open_amp.h>

#include "rp_ll.h"
#include "nrf_rpc.h"
#include "nrf_rpc_rpmsg.h"

/* Utility macro for dumping content of the packets with limit of 32 bytes
 * to prevent overflowing the logs.
 */
#define DUMP_LIMITED_DBG(memory, len, text) do {			       \
	if ((len) > 32) {						       \
		NRF_RPC_DUMP_DBG(memory, 32, text " (truncated)");	       \
	} else {							       \
		NRF_RPC_DUMP_DBG(memory, (len), text);			       \
	}								       \
} while (0)

/* Semaphore used to do initial handshake */
K_SEM_DEFINE(handshake_sem, 0, 1);

/* Upper level callbacks */
static nrf_rpc_tr_receive_handler_t receive_callback;

/* Lower level endpoint instance */
static struct rp_ll_endpoint ll_endpoint;

/* Translates RPMsg error code to nRF RPC error code. */
static int translate_error(int rpmsg_err)
{
	switch (rpmsg_err) {
	case RPMSG_ERR_BUFF_SIZE:
	case RPMSG_ERR_NO_MEM:
	case RPMSG_ERR_NO_BUFF:
		return -NRF_ENOMEM;
	case RPMSG_ERR_PARAM:
		return -NRF_EINVAL;
	case RPMSG_ERR_DEV_STATE:
	case RPMSG_ERR_INIT:
	case RPMSG_ERR_ADDR:
		return -NRF_EIO;
	default:
		if (rpmsg_err < 0) {
			return -NRF_EIO;
		}
		break;
	}
	return 0;
}

/* Event callback from lower level. */
static void ll_event_handler(struct rp_ll_endpoint *endpoint,
			    enum rp_ll_event_type event, const uint8_t *buf,
			    size_t length)
{
	if (event == RP_LL_EVENT_CONNECTED) {

		NRF_RPC_DBG("nRF RPC Connected");
		k_sem_give(&handshake_sem);
		return;

	} else if (event != RP_LL_EVENT_DATA) {

		return;

	}

	NRF_RPC_ASSERT(buf != NULL);

	DUMP_LIMITED_DBG(buf, length, "Received data");

	receive_callback(buf, length);
}

int nrf_rpc_tr_init(nrf_rpc_tr_receive_handler_t callback)
{
	int err = 0;

	NRF_RPC_ASSERT(callback != NULL);

	receive_callback = callback;

	err = rp_ll_init();
	if (err != 0) {
		goto error_exit;
	}

	err = rp_ll_endpoint_init(&ll_endpoint, 1, ll_event_handler, NULL);
	if (err != 0) {
		goto error_exit;
	}

	k_sem_take(&handshake_sem, K_FOREVER);

	NRF_RPC_DBG("nRF RPC Initialized");

	return 0;

error_exit:
	NRF_RPC_DBG("nRF RPC Initialization error %d", err);

	return translate_error(err);
}

int nrf_rpc_tr_send(uint8_t *buf, size_t len)
{
	int err;

	NRF_RPC_ASSERT(buf != NULL);

	DUMP_LIMITED_DBG(buf, len, "Send data");

	err = rp_ll_send(&ll_endpoint, buf, len);

	return translate_error(err);
}

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define NRF_RPC_LOG_MODULE NRF_RPC_TR
#include <nrf_rpc_log.h>

#include <device.h>
#include <ipc/ipc_service.h>
#include <openamp/open_amp.h>

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


/* Semaphore used to synchronize endpoint binding.*/
K_SEM_DEFINE(ipc_bound_sem, 0, 1);

/* Upper level callbacks */
static nrf_rpc_tr_receive_handler_t receive_callback;

/* IPC service endpoint instance */
struct ipc_ept nrf_rpc_ept;

static void nrf_rpc_ept_bound(void *priv)
{
	NRF_RPC_DBG("nRF RPC Connected");

	k_sem_give(&ipc_bound_sem);
}

static void nrf_rpc_ept_recv(const void *data, size_t len, void *priv)
{
	NRF_RPC_ASSERT(data != NULL);

	DUMP_LIMITED_DBG(data, len, "Received data");

	receive_callback(data, len);
}

static struct ipc_ept_cfg nrf_rpc_ept_cfg = {
	.name = "nrf_rpc_ept",
	.cb = {
		.bound    = nrf_rpc_ept_bound,
		.received = nrf_rpc_ept_recv,
	},
};

/* Translates error code from the lower layer to nRF RPC error code. */
static int translate_error(int ll_err)
{
	switch (ll_err) {
	case -EINVAL:
		return -NRF_EINVAL;
	case -EIO:
		return -NRF_EIO;
	case -EALREADY:
		return -NRF_EALREADY;
	case -EBADMSG:
		return -NRF_EBADMSG;
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
		if (ll_err < 0) {
			return -NRF_EIO;
		}
		break;
	}
	return 0;
}

int nrf_rpc_tr_init(nrf_rpc_tr_receive_handler_t callback)
{
	int err;
	const struct device *nrf_rpc_ipc_instance;

	NRF_RPC_ASSERT(callback != NULL);
	receive_callback = callback;

	nrf_rpc_ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	err = ipc_service_open_instance(nrf_rpc_ipc_instance);
	if (err < 0 && err != -EALREADY) {
		NRF_RPC_ERR("IPC service instance initialization failed: %d\n", err);
		return translate_error(err);
	}

	err = ipc_service_register_endpoint(nrf_rpc_ipc_instance, &nrf_rpc_ept, &nrf_rpc_ept_cfg);
	if (err < 0) {
		NRF_RPC_ERR("Registering endpoint failed with %d", err);
		return translate_error(err);
	}

	k_sem_take(&ipc_bound_sem, K_FOREVER);

	return 0;
}

int nrf_rpc_tr_send(uint8_t *buf, size_t len)
{
	int err;

	NRF_RPC_ASSERT(buf != NULL);
	NRF_RPC_DBG("Send %u bytes.", len);
	DUMP_LIMITED_DBG(buf, len, "Data:");

	err = ipc_service_send(&nrf_rpc_ept, buf, len);
	if (err > 0) {
		err = 0;
	}

	return translate_error(err);
}

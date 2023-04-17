/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <nrf_rpc_cbor.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include "dm_rpc_host.h"
#include "../common/dm_rpc_common.h"
#include "../common/serialize.h"
#include "dm.h"

LOG_MODULE_DECLARE(nrf_dm, CONFIG_DM_MODULE_LOG_LEVEL);
NRF_RPC_GROUP_DECLARE(dm_rpc_grp);

static struct ipc_ept ep;
static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(process_sem, 0, 1);

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &dm_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static void dm_init_rpc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	int result;
	struct dm_init_param dm_param;

	if (!ser_decoding_done_and_check(group, ctx)) {
		report_decoding_error(DM_INIT_RPC_CMD, handler_data);
		return;
	}

	dm_param.cb = NULL;
	result = dm_init(&dm_param);
	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(dm_rpc_grp, dm_init, DM_INIT_RPC_CMD, dm_init_rpc_handler, NULL);

static void dm_request_add_rpc_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct dm_request req;
	int result;

	/* Decode the request structure */
	req.role = ser_decode_uint(ctx);
	ser_decode_buffer(ctx, &req.bt_addr, sizeof(bt_addr_le_t));
	req.rng_seed = ser_decode_uint(ctx);
	req.ranging_mode = ser_decode_uint(ctx);
	req.start_delay_us = ser_decode_uint(ctx);
	req.extra_window_time_us = ser_decode_uint(ctx);

	if (!ser_decoding_done_and_check(group, ctx)) {
		report_decoding_error(DM_REQUEST_ADD_RPC_CMD, handler_data);
		return;
	}

	result = dm_request_add(&req);
	ser_rsp_send_int(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(dm_rpc_grp, dm_request_add, DM_REQUEST_ADD_RPC_CMD,
			 dm_request_add_rpc_handler, NULL);


void *dm_rpc_get_buffer(size_t buffer_len)
{
	int ret;
	void *data = NULL;

	ret = ipc_service_get_tx_buffer(&ep, &data, &buffer_len, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("ipc_service_get_tx_buffer failed with err: %d\n", ret);
		__ASSERT_NO_MSG(false);
	}

	return data;
}

void dm_rpc_calc_and_process(const void *data, size_t len)
{
	int ret;

	ret = ipc_service_send_nocopy(&ep, data, len);
	if (ret < 0) {
		LOG_ERR("ipc_service_send_nocpy returned err: %d", ret);
		return;
	}

	k_sem_take(&process_sem, K_FOREVER);
}

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	enum dm_rpc_cmd cmd;

	cmd = *((enum dm_rpc_cmd *)data);
	if (cmd == DM_END_PROCESS_RPC_CMD) {
		k_sem_give(&process_sem);
	}
}

static struct ipc_ept_cfg ep_cfg = {
	.name = "dm_ept",
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

static int ipc_init(void)
{

	int err;
	const struct device *ipc_instance;

	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc1));

	err = ipc_service_open_instance(ipc_instance);
	if ((err < 0) && (err != -EALREADY)) {
		LOG_ERR("IPC service instance initialization failed with err: %d", err);
		return err;
	}

	err = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
	if (err < 0) {
		LOG_ERR("Registering endpoint failed with err: %d", err);
		return err;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	return 0;
}

SYS_INIT(ipc_init, POST_KERNEL, CONFIG_DM_MODULE_RPC_IPC_SERVICE_INIT_PRIORITY);

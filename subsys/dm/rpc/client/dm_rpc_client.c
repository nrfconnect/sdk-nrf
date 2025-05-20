/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nrf_rpc_cbor.h>
#include <nrf_dm.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

#include <dm.h>

#include "../common/dm_rpc_common.h"
#include "../common/serialize.h"

LOG_MODULE_REGISTER(nrf_dm, CONFIG_DM_MODULE_LOG_LEVEL);
NRF_RPC_GROUP_DECLARE(dm_rpc_grp);

static void data_handler(struct k_work *work);

static struct dm_cb *init_param_cb;
static struct ipc_ept ep;
static K_SEM_DEFINE(bound_sem, 0, 1);
static void *recv_data;
static K_WORK_DEFINE(dm_data_work, data_handler);

int dm_request_add(struct dm_request *req)
{
	int res;
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 34; /* This value is due to the serialization method. */

	NRF_RPC_CBOR_ALLOC(&dm_rpc_grp, ctx, buffer_size_max);

	/* Encode the request structure */
	ser_encode_uint(&ctx, req->role);
	ser_encode_buffer(&ctx, &req->bt_addr, sizeof(bt_addr_le_t));
	ser_encode_uint(&ctx, req->rng_seed);
	ser_encode_uint(&ctx, req->ranging_mode);
	ser_encode_uint(&ctx, req->start_delay_us);
	ser_encode_uint(&ctx, req->extra_window_time_us);

	nrf_rpc_cbor_cmd_no_err(&dm_rpc_grp, DM_REQUEST_ADD_RPC_CMD, &ctx,
				ser_rsp_decode_i32, &res);

	return res;
}

int dm_init(struct dm_init_param *init_param)
{
	int result;
	struct nrf_rpc_cbor_ctx ctx;
	size_t buffer_size_max = 0;

	if (!init_param) {
		return -EINVAL;
	}

	init_param_cb = init_param->cb;

	NRF_RPC_CBOR_ALLOC(&dm_rpc_grp, ctx, buffer_size_max);

	nrf_rpc_cbor_cmd_no_err(&dm_rpc_grp, DM_INIT_RPC_CMD,
			       &ctx, ser_rsp_decode_i32, &result);

	return result;
}

static void process_data(const struct dm_rpc_process_data *data, struct dm_result *result,
			 float high_precision_estimate)
{
	if (!data || !result) {
		return;
	}

	result->status = data->report.status == NRF_DM_STATUS_SUCCESS;
	bt_addr_le_copy(&result->bt_addr, &data->bt_addr);

	result->quality = DM_QUALITY_NONE;
	if (data->report.quality == NRF_DM_QUALITY_OK) {
		result->quality = DM_QUALITY_OK;
	} else if (data->report.quality == NRF_DM_QUALITY_POOR) {
		result->quality = DM_QUALITY_POOR;
	} else if (data->report.quality == NRF_DM_QUALITY_DO_NOT_USE) {
		result->quality = DM_QUALITY_DO_NOT_USE;
	} else if (data->report.quality == NRF_DM_QUALITY_CRC_FAIL) {
		result->quality = DM_QUALITY_CRC_FAIL;
	}

	result->ranging_mode = data->report.ranging_mode;
	if (result->ranging_mode == DM_RANGING_MODE_RTT) {
		result->dist_estimates.rtt.rtt = data->report.distance_estimates.rtt.rtt;
	} else {
		result->dist_estimates.mcpd.ifft = data->report.distance_estimates.mcpd.ifft;
		result->dist_estimates.mcpd.phase_slope =
						data->report.distance_estimates.mcpd.phase_slope;
		result->dist_estimates.mcpd.best = data->report.distance_estimates.mcpd.best;
		result->dist_estimates.mcpd.rssi_openspace =
						data->report.distance_estimates.mcpd.rssi_openspace;
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
		result->dist_estimates.mcpd.high_precision = high_precision_estimate;
#endif
	}
}

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	int ret;

	ret = ipc_service_hold_rx_buffer(&ep, (void *)data);
	if (ret < 0) {
		LOG_ERR("ipc_service_hold_rx_buffer failed with err: %d", ret);
		__ASSERT_NO_MSG(false);
		return;
	}

	recv_data = (void *)data;
	k_work_submit(&dm_data_work);
}
static struct ipc_ept_cfg ep_cfg = {
	.name = "dm_ept",
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

static void data_handler(struct k_work *work)
{
	int ret;
	enum dm_rpc_cmd cmd;
	struct dm_result result = {0};
	float high_precision_estimate = 0;

	struct dm_rpc_process_data *dm_data = (struct dm_rpc_process_data *)recv_data;

	nrf_dm_calc(&dm_data->report);
#ifdef CONFIG_DM_HIGH_PRECISION_CALC
	if (dm_data->report.ranging_mode == NRF_DM_RANGING_MODE_MCPD) {
		high_precision_estimate = nrf_dm_high_precision_calc(&dm_data->report);
	}
#endif
	process_data(dm_data, &result, high_precision_estimate);
	if (init_param_cb->data_ready) {
		init_param_cb->data_ready(&result);
	}

	ret = ipc_service_release_rx_buffer(&ep, recv_data);
	if (ret < 0) {
		LOG_ERR("ipc_service_release_rx_buffer failed with err: %d", ret);
		__ASSERT_NO_MSG(false);
		return;
	}

	cmd = DM_END_PROCESS_RPC_CMD;
	ipc_service_send(&ep, &cmd, sizeof(cmd));
}

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

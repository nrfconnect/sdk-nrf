/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing OS specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <common/mem_mgmt.h>
#include "ipc_if.h"
#include <zephyr/sys/math_extras.h>

#include "shim.h"
#include "osal_ops.h"
#include "common/hal_structs_common.h"

LOG_MODULE_REGISTER(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct zep_shim_intr_priv *intr_priv;

static unsigned long zep_shim_time_get_curr_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

static unsigned int zep_shim_time_elapsed_us(unsigned long start_time_us)
{
	unsigned long curr_time_us = 0;

	curr_time_us = zep_shim_time_get_curr_us();

	return curr_time_us - start_time_us;
}

static unsigned long zep_shim_time_get_curr_ms(void)
{
	return k_uptime_get();
}

static unsigned int zep_shim_time_elapsed_ms(unsigned long start_time_ms)
{
	unsigned long curr_time_ms = 0;

	curr_time_ms = zep_shim_time_get_curr_ms();

	return curr_time_ms - start_time_ms;
}

static enum nrf_wifi_status zep_shim_bus_qspi_dev_init(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	return NRF_WIFI_STATUS_SUCCESS;
}

static void zep_shim_bus_qspi_dev_deinit(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	volatile struct rpu_dev *dev = qspi_priv->qspi_dev;

	dev->deinit();
}

static int ipc_send_msg(unsigned int msg_type, void *msg, unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct rpu_dev *dev = rpu_dev();
	int ret;
	ipc_ctx_t ctx;

	switch (msg_type) {
	case NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL:
		ctx.inst = IPC_INSTANCE_CMD_CTRL;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX:
		ctx.inst = IPC_INSTANCE_CMD_TX;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX:
		ctx.inst = IPC_INSTANCE_RX;
		ctx.ept = IPC_EPT_LMAC;
		break;
	default:
		LOG_ERR("%s: Invalid msg_type (%d)", __func__, msg_type);
		goto out;
	};

	ret = dev->send(ctx, msg, len);
	if (ret < 0) {
		LOG_ERR("%s: Sending message to RPU failed\n", __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

static void *zep_shim_bus_qspi_dev_add(void *os_qspi_priv, void *osal_qspi_dev_ctx)
{
	struct zep_shim_bus_qspi_priv *zep_qspi_priv = os_qspi_priv;
	struct rpu_dev *dev = rpu_dev();

	dev->init();
	zep_qspi_priv->qspi_dev = dev;
	zep_qspi_priv->dev_added = true;

	return zep_qspi_priv;
}

static void zep_shim_bus_qspi_dev_rem(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev = qspi_priv->qspi_dev;

	ARG_UNUSED(dev);

}

static void *zep_shim_bus_qspi_init(void)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*qspi_priv));

	if (!qspi_priv) {
		LOG_ERR("%s: Unable to allocate memory for qspi_priv", __func__);
		goto out;
	}
out:
	return qspi_priv;
}

static void zep_shim_bus_qspi_deinit(void *os_qspi_priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = os_qspi_priv;

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, qspi_priv);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
/* nRF71 flat driver: QSPI RPU power save not supported; no-ops. */
static int zep_shim_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}

static int zep_shim_bus_qspi_ps_wake(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}

static int zep_shim_bus_qspi_ps_status(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

static void zep_shim_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					       struct nrf_wifi_osal_host_map *host_map)
{
	if (!os_qspi_dev_ctx || !host_map) {
		LOG_ERR("%s: Invalid parameters", __func__);
		return;
	}

	host_map->addr = 0;
}


static enum nrf_wifi_status zep_shim_bus_qspi_intr_reg(void *os_dev_ctx, void *callbk_data,
						       int (*callbk_fn)(void *callbk_data))
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int ret = -1;

	ARG_UNUSED(os_dev_ctx);

	ret = ipc_register_rx_cb(callbk_fn, callbk_data);
	if (ret) {
		LOG_ERR("%s: ipc_register_rx_cb failed\n", __func__);
		goto out;
	}
	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

static void zep_shim_bus_qspi_intr_unreg(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	/* Detach the event consumer before the HAL context is torn down. The IPC
	 * endpoint itself stays bound: it is the control plane and its lifetime
	 * follows the Wi-Fi core, not the interface.
	 */
	ipc_unregister_rx_cb();
}

static unsigned int zep_shim_strlen(const void *str)
{
	return strlen(str);
}

const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops = {
	.time_get_curr_us = zep_shim_time_get_curr_us,
	.time_elapsed_us = zep_shim_time_elapsed_us,
	.time_get_curr_ms = zep_shim_time_get_curr_ms,
	.time_elapsed_ms = zep_shim_time_elapsed_ms,

	.bus_qspi_init = zep_shim_bus_qspi_init,
	.bus_qspi_deinit = zep_shim_bus_qspi_deinit,
	.bus_qspi_dev_add = zep_shim_bus_qspi_dev_add,
	.bus_qspi_dev_rem = zep_shim_bus_qspi_dev_rem,
	.bus_qspi_dev_init = zep_shim_bus_qspi_dev_init,
	.bus_qspi_dev_deinit = zep_shim_bus_qspi_dev_deinit,
	.bus_qspi_dev_intr_reg = zep_shim_bus_qspi_intr_reg,
	.bus_qspi_dev_intr_unreg = zep_shim_bus_qspi_intr_unreg,
	.bus_qspi_dev_host_map_get = zep_shim_bus_qspi_dev_host_map_get,

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.bus_qspi_ps_sleep = zep_shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = zep_shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = zep_shim_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	.strlen = zep_shim_strlen,
	.ipc_send_msg = ipc_send_msg,
};

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief IPC bus layer function definitions for the nRF71 Wi-Fi driver.
 */

#include <common/mem_mgmt.h>

#include <stdint.h>
#include <string.h>

#include "bal_structs.h"
#include "ipc_bus.h"
#include "ipc_if.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static int nrf_wifi_bus_ipc_irq_handler(void *data)
{
	struct nrf_wifi_bus_ipc_dev_ctx *dev_ctx = data;
	struct nrf_wifi_bus_ipc_priv *ipc_priv = dev_ctx->ipc_priv;

	return ipc_priv->intr_callbk_fn(dev_ctx->bal_dev_ctx);
}

static void *nrf_wifi_bus_ipc_dev_add(void *bus_priv, void *bal_dev_ctx)
{
	struct nrf_wifi_bus_ipc_priv *ipc_priv = bus_priv;
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx;
	int ret;

	ipc_dev_ctx = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*ipc_dev_ctx));
	if (!ipc_dev_ctx) {
		LOG_ERR("%s: Unable to allocate ipc_dev_ctx", __func__);
		return NULL;
	}

	ipc_dev_ctx->ipc_priv = ipc_priv;
	ipc_dev_ctx->bal_dev_ctx = bal_dev_ctx;

	ret = ipc_init();
	if (ret) {
		LOG_ERR("%s: ipc_init failed", __func__);
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, ipc_dev_ctx);
		return NULL;
	}

	ipc_dev_ctx->host_addr_base = 0;
	ipc_dev_ctx->addr_pktram_base = ipc_dev_ctx->host_addr_base +
					ipc_priv->cfg_params.addr_pktram_base;

	return ipc_dev_ctx;
}

static void nrf_wifi_bus_ipc_dev_rem(void *bus_dev_ctx)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = bus_dev_ctx;

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, ipc_dev_ctx);
}

static enum nrf_wifi_status nrf_wifi_bus_ipc_dev_init(void *bus_dev_ctx)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = bus_dev_ctx;
	int ret;

	ret = ipc_register_rx_cb(&nrf_wifi_bus_ipc_irq_handler, ipc_dev_ctx);
	if (ret) {
		LOG_ERR("%s: ipc_register_rx_cb failed", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

static void nrf_wifi_bus_ipc_dev_deinit(void *bus_dev_ctx)
{
	ARG_UNUSED(bus_dev_ctx);

	/* Detach the event consumer before the HAL context is torn down. The IPC
	 * endpoint itself stays bound: it is the control plane and its lifetime
	 * follows the Wi-Fi core, not the interface.
	 */
	ipc_unregister_rx_cb();
	ipc_deinit();
}

static void *nrf_wifi_bus_ipc_init(void *params,
				    enum nrf_wifi_status (*intr_callbk_fn)(void *bal_dev_ctx))
{
	struct nrf_wifi_bus_ipc_priv *ipc_priv;

	ipc_priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*ipc_priv));
	if (!ipc_priv) {
		LOG_ERR("%s: Unable to allocate memory for ipc_priv", __func__);
		return NULL;
	}

	nrf_wifi_mem_cpy(&ipc_priv->cfg_params, params, sizeof(ipc_priv->cfg_params));
	ipc_priv->intr_callbk_fn = intr_callbk_fn;

	return ipc_priv;
}

static void nrf_wifi_bus_ipc_deinit(void *bus_priv)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, bus_priv);
}

static unsigned int nrf_wifi_bus_ipc_read_word(void *dev_ctx, unsigned long addr_offset)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;
	uintptr_t addr = ipc_dev_ctx->host_addr_base + addr_offset;

	return *(const volatile uint32_t *)addr;
}

static void nrf_wifi_bus_ipc_write_word(void *dev_ctx, unsigned long addr_offset,
					unsigned int val)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;
	uintptr_t addr = ipc_dev_ctx->host_addr_base + addr_offset;

	*(volatile uint32_t *)addr = val;
}

static void nrf_wifi_bus_ipc_read_block(void *dev_ctx, void *dest_addr,
					unsigned long src_addr_offset, size_t len)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;
	uintptr_t addr = ipc_dev_ctx->host_addr_base + src_addr_offset;

	memcpy(dest_addr, (const void *)addr, len);
}

static void nrf_wifi_bus_ipc_write_block(void *dev_ctx, unsigned long dest_addr_offset,
					 const void *src_addr, size_t len)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;
	uintptr_t addr = ipc_dev_ctx->host_addr_base + dest_addr_offset;

	memcpy((void *)addr, src_addr, len);
}

static unsigned long nrf_wifi_bus_ipc_dma_map(void *dev_ctx, unsigned long virt_addr,
					      size_t len,
					      enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;

	ARG_UNUSED(len);
	ARG_UNUSED(dma_dir);

	return ipc_dev_ctx->host_addr_base + (virt_addr - ipc_dev_ctx->addr_pktram_base);
}

static unsigned long nrf_wifi_bus_ipc_dma_unmap(void *dev_ctx, unsigned long phy_addr,
						size_t len,
						enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bus_ipc_dev_ctx *ipc_dev_ctx = dev_ctx;

	ARG_UNUSED(len);
	ARG_UNUSED(dma_dir);

	return ipc_dev_ctx->addr_pktram_base + (phy_addr - ipc_dev_ctx->host_addr_base);
}

static enum nrf_wifi_status nrf_wifi_bus_ipc_send_msg(void *dev_ctx, unsigned int msg_type,
						      void *msg, unsigned int len)
{
	ipc_ctx_t ctx;
	int ret;

	ARG_UNUSED(dev_ctx);

	switch (msg_type) {
	case NRF_WIFI_IPC_MSG_CMD_CTRL:
		ctx.inst = IPC_INSTANCE_CMD_CTRL;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_IPC_MSG_CMD_DATA_TX:
		ctx.inst = IPC_INSTANCE_CMD_TX;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_IPC_MSG_CMD_DATA_RX:
		ctx.inst = IPC_INSTANCE_RX;
		ctx.ept = IPC_EPT_LMAC;
		break;
	default:
		LOG_ERR("%s: Invalid msg_type (%d)", __func__, msg_type);
		return NRF_WIFI_STATUS_FAIL;
	}

	ret = ipc_send(ctx, msg, len);
	if (ret < 0) {
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

#ifdef NRF_WIFI_LOW_POWER
static void nrf_wifi_bus_ipc_ps_sleep(void *dev_ctx)
{
	ARG_UNUSED(dev_ctx);
}

static void nrf_wifi_bus_ipc_ps_wake(void *dev_ctx)
{
	ARG_UNUSED(dev_ctx);
}

static int nrf_wifi_bus_ipc_ps_status(void *dev_ctx)
{
	ARG_UNUSED(dev_ctx);

	return 0;
}
#endif /* NRF_WIFI_LOW_POWER */

static struct nrf_wifi_bal_ops nrf_wifi_bus_ipc_ops = {
	.init = &nrf_wifi_bus_ipc_init,
	.deinit = &nrf_wifi_bus_ipc_deinit,
	.dev_add = &nrf_wifi_bus_ipc_dev_add,
	.dev_rem = &nrf_wifi_bus_ipc_dev_rem,
	.dev_init = &nrf_wifi_bus_ipc_dev_init,
	.dev_deinit = &nrf_wifi_bus_ipc_dev_deinit,
	.read_word = &nrf_wifi_bus_ipc_read_word,
	.write_word = &nrf_wifi_bus_ipc_write_word,
	.read_block = &nrf_wifi_bus_ipc_read_block,
	.write_block = &nrf_wifi_bus_ipc_write_block,
	.dma_map = &nrf_wifi_bus_ipc_dma_map,
	.dma_unmap = &nrf_wifi_bus_ipc_dma_unmap,
	.ipc_send_msg = &nrf_wifi_bus_ipc_send_msg,
#ifdef NRF_WIFI_LOW_POWER
	.rpu_ps_sleep = &nrf_wifi_bus_ipc_ps_sleep,
	.rpu_ps_wake = &nrf_wifi_bus_ipc_ps_wake,
	.rpu_ps_status = &nrf_wifi_bus_ipc_ps_status,
#endif /* NRF_WIFI_LOW_POWER */
};

struct nrf_wifi_bal_ops *get_bus_ops(void)
{
	return &nrf_wifi_bus_ipc_ops;
}

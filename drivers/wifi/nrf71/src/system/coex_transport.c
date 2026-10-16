/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief nRF71 Wi-Fi coexistence transport.
 *
 * Thin, stable wrapper that lets the host-side Coexistence Driver forward
 * CD2CM command buffers to the Coexistence Manager (running in RPU firmware)
 * over the Wi-Fi FMAC control path, without depending on the Wi-Fi driver's
 * private FMAC context layout.
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>

#include <system/core.h>
#include <common/fmac_api_common.h>
#include <common/fmac_structs_common.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static nrf71_wifi_coex_event_cb_t coex_event_cb;
static void *coex_event_cb_ctx;

static struct nrf_wifi_ctx_zep *coex_rpu_ctx(void)
{
	return &rpu_drv_priv_zep.rpu_ctx_zep;
}

/**
 * Read or write a COEXC register in the global domain.
 *
 * On nRF71 COEXC sits in the always-on global domain and is memory-mapped
 * on the application processor.
 */
static int coex_hal_reg_access(uint32_t reg_addr, uint32_t *value, bool write)
{
	uintptr_t mmio_addr;

	if (value == NULL) {
		return -EINVAL;
	}

	mmio_addr = (uintptr_t)reg_addr;

	if (write) {
		sys_write32(*value, mmio_addr);
	} else {
		*value = sys_read32(mmio_addr);
	}

	return 0;
}

bool nrf71_wifi_coex_is_ready(void)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();

	return (rpu_ctx != NULL) && (rpu_ctx->rpu_ctx != NULL);
}

int nrf71_wifi_coex_cmd_send(const void *cmd, size_t len)
{
	struct nrf_wifi_ctx_zep *rpu_ctx = coex_rpu_ctx();
	enum nrf_wifi_status status;

	if ((cmd == NULL) || (len == 0)) {
		return -EINVAL;
	}

	if (!nrf71_wifi_coex_is_ready()) {
		LOG_DBG("Coex transport not ready, dropping command");
		return -ENODEV;
	}

	/* nrf_wifi_fmac_conf_srcoex() takes a non-const buffer; the FMAC layer
	 * copies it into the command message and does not modify the caller's
	 * data.
	 */
	status = nrf_wifi_fmac_conf_srcoex(rpu_ctx->rpu_ctx, (void *)cmd, (unsigned int)len);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to send coex command to RPU");
		return -EIO;
	}

	LOG_DBG("CD2CM command posted to RPU (%u bytes)", (unsigned int)len);

	return 0;
}

int nrf71_wifi_coex_register_event_cb(nrf71_wifi_coex_event_cb_t cb, void *ctx)
{
	coex_event_cb = cb;
	coex_event_cb_ctx = ctx;

	return 0;
}

void nrf71_wifi_coex_on_event(const void *event, size_t len)
{
	if (coex_event_cb != NULL) {
		LOG_DBG("Forwarding CM2CD event to coex driver (%zu bytes)", len);
		coex_event_cb(coex_event_cb_ctx, event, len);
		LOG_DBG("Coex driver handler returned");
	} else {
		LOG_WRN("Coex event dropped (%zu bytes): no handler registered", len);
	}
}

int nrf71_wifi_coex_reg_write(uint32_t reg_addr, uint32_t value)
{
	return coex_hal_reg_access(reg_addr, &value, true);
}

int nrf71_wifi_coex_reg_read(uint32_t reg_addr, uint32_t *value)
{
	if (value == NULL) {
		return -EINVAL;
	}

	return coex_hal_reg_access(reg_addr, value, false);
}

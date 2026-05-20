/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrf_gswdt

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/watchdog.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(NRF_GSWDT, CONFIG_NRF_GSWDT_LOG_LEVEL);

/* Driver for Nordic Global Software Watchdog Timer (GSWDT)
 *
 * The GSWDT is a software-based watchdog that can be used in conjunction with
 * the MBOX peripheral to provide a watchdog functionality. The driver sends
 * messages to the system controller through the MBOX interface to feed the
 * watchdog. Maximum timeout is determined by the system controller configuration
 * and is set to 6s. Minimum feed interval should be 5s to avoid watchdog
 * expiration. Too short feeding intervals may cause increased power consumption.
 *
 * During nrf_gswdt_setup the driver sends a message to the system controller to start the watchdog.
 * After that, the driver expects periodic calls to nrf_gswdt_feed to keep the watchdog from
 * expiring. If the watchdog expires, the system controller will reset the SoC.
 * Disable functionality is not supported, as the GSWDT cannot be disabled once started.
 */

 #define NRF_GSWDT_MAX_TIMEOUT_MS 5000

struct nrf_gswdt_config {
	struct mbox_dt_spec gswdt_mbox;
};

static int nrf_gswdt_setup(const struct device *dev, uint8_t options)
{
	if (options != 0) {
		LOG_WRN("Unsupported options: 0x%02x", options);
	}

	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_send_dt(gswdt_mbox, NULL);
}

static int nrf_gswdt_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int nrf_gswdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	LOG_DBG("timeout: %u ms, flags: 0x%02x", cfg->window.max, cfg->flags);
	ARG_UNUSED(dev);
	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_ERR("Unsupported flags: 0x%02x", cfg->flags);
		return -ENOTSUP;
	}

	if (cfg->window.max > NRF_GSWDT_MAX_TIMEOUT_MS) {
		LOG_ERR("Unsupported timeout: %u ms", cfg->window.max);
		return -ENOTSUP;
	}
	return 0;
}

static int nrf_gswdt_feed(const struct device *dev, int channel_id)
{
	LOG_DBG("FEED channel_id: %d", channel_id);
	ARG_UNUSED(channel_id);

	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_send_dt(gswdt_mbox, NULL);
}

static DEVICE_API(wdt, nrf_gswdt_driver_api) = {
	.setup = nrf_gswdt_setup,
	.disable = nrf_gswdt_disable,
	.install_timeout = nrf_gswdt_install_timeout,
	.feed = nrf_gswdt_feed,
};

static int nrf_gswdt_init(const struct device *dev)
{
	LOG_DBG("Initializing GSWDT device");
	const struct mbox_dt_spec *gswdt_mbox =
		&((const struct nrf_gswdt_config *)dev->config)->gswdt_mbox;

	return mbox_is_ready_dt(gswdt_mbox) ? 0 : -ENODEV;
}

#define GSWDT_INIT(inst)									\
	static struct nrf_gswdt_config nrf_gswdt_config_##inst = {				\
		.gswdt_mbox = MBOX_DT_SPEC_GET(DT_PHANDLE(DT_INST(inst, nordic_nrf_gswdt),	\
		mbox),tx),									\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
	nrf_gswdt_init,										\
	NULL,											\
	NULL,											\
	&nrf_gswdt_config_##inst,								\
	POST_KERNEL,										\
	UTIL_INC(CONFIG_MBOX_INIT_PRIORITY),							\
	&nrf_gswdt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GSWDT_INIT)

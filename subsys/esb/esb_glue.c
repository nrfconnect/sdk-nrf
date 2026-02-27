/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file esb_glue.c
 * @brief ESB platform glue layer
 *
 * This module provides the platform-specific integration between the ESB
 * protocol and the Zephyr RTOS. It handles HF clock initialization and
 * applies platform-specific errata workarounds that are required for correct
 * ESB radio operation.
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>
#include "esb_glue.h"
#include "esb_workarounds.h"

#if defined(CONFIG_CLOCK_CONTROL_NRF2)
#include <hal/nrf_lrcconf.h>
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF2) */

LOG_MODULE_REGISTER(esb_glue, CONFIG_ESB_LOG_LEVEL);

#if defined(CONFIG_ESB_CLOCK_INIT)

#if defined(CONFIG_CLOCK_CONTROL_NRF)

int esb_clocks_start(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENODEV;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
		if (err == -EAGAIN) {
			k_yield();
		}
	} while (err == -EAGAIN);

	__ASSERT(err == 0, "Unexpected return code from sys_notify_fetch_result: %d", err);
	if (err) {
		LOG_ERR("Unexpected return code from sys_notify_fetch_result: %d", err);
		return err;
	}

	esb_apply_nrf54l_20();
	esb_apply_nrf54l_39();

	LOG_DBG("HF clock started");
	return 0;
}

int esb_clocks_stop(void)
{
	int err;
	struct onoff_manager *clk_mgr;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENODEV;
	}

	err = onoff_release(clk_mgr);
	if (err < 0) {
		LOG_ERR("Clock release failed: %d", err);
		return err;
	}

	esb_revert_nrf54l_39();
	esb_revert_nrf54l_20();

	LOG_DBG("HF clock stopped");
	return 0;
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

int esb_clocks_start(void)
{
	int err;
	int res;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
	struct onoff_client radio_cli;

	if (!radio_clk_dev || !device_is_ready(radio_clk_dev)) {
		LOG_ERR("Radio clock device not found or not ready");
		return -ENODEV;
	}

	sys_notify_init_spinwait(&radio_cli.notify);

	err = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&radio_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
		if (err == -EAGAIN) {
			k_yield();
		}
	} while (err == -EAGAIN);

	__ASSERT(err == 0, "Unexpected return code from sys_notify_fetch_result: %d", err);
	if (err) {
		LOG_ERR("Unexpected return code from sys_notify_fetch_result: %d", err);
		return err;
	}

	/* Keep radio domain powered all the time to reduce latency. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, true);

	esb_apply_nrf54h_84();

	LOG_DBG("HF clock started");
	return 0;
}

int esb_clocks_stop(void)
{
	int err;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));

	if (!radio_clk_dev || !device_is_ready(radio_clk_dev)) {
		LOG_ERR("Radio clock device not found or not ready");
		return -ENODEV;
	}

	err = nrf_clock_control_release(radio_clk_dev, NULL);
	if (err < 0) {
		LOG_ERR("Clock release failed: %d", err);
		return err;
	}

	esb_revert_nrf54h_84();

	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, false);

	LOG_DBG("HF clock stopped");
	return 0;
}

#else
BUILD_ASSERT(false, "No Clock Control driver");
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF) */

#endif /* defined(CONFIG_ESB_CLOCK_INIT) */

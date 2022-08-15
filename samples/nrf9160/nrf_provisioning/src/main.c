/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/nrf_provisioning.h>
#include "nrf_provisioning_at.h"

LOG_MODULE_REGISTER(nrf_provisioning_sample, CONFIG_NRF_PROVISIONING_SAMPLE_LOG_LEVEL);

static struct nrf_provisioning_dm_change dmode;
static struct nrf_provisioning_mm_change mmode;

static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;
	char time_buf[64];
	int ret;

	(void)user_data;

	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		ret = -EFAULT;
		return ret;
	}

	if (fmode == new_mode) {
		ret = fmode;
	} else if (new_mode == LTE_LC_FUNC_MODE_NORMAL) {
		/* Use the blocking call, because in next step
		 * the service will create a socket and call connect()
		 */
		ret = lte_lc_connect();

		if (ret) {
			LOG_ERR("lte_lc_connect() failed %d", ret);
		}
		LOG_INF("Modem connection restored");

		LOG_INF("Waiting for modem to acquire network time...");

		do {
			k_sleep(K_SECONDS(3));
			ret = nrf_provisioning_at_time_get(time_buf, sizeof(time_buf));
		} while (ret != 0);

		LOG_INF("Network time obtained");
		ret = fmode;
	} else {
		ret = lte_lc_func_mode_set(new_mode);
		if (ret == 0) {
			LOG_DBG("Modem set to requested state %d", new_mode);
			ret = fmode;
		}
	}

	return ret;
}

static void device_mode_cb(void *user_data)
{
	(void)user_data;

	/* Disconnect from network gracefully */
	int ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);

	if (ret != 0) {
		LOG_ERR("Unable to set modem offline, error %d", ret);
	}

	LOG_INF("Provisioning done, rebooting...");
	while (log_process()) {
		;
	}

	sys_reboot(SYS_REBOOT_WARM);
}

int main(void)
{
	int ret;

	mmode.cb = modem_mode_cb;
	mmode.user_data = NULL;
	dmode.cb = device_mode_cb;
	dmode.user_data = NULL;

	LOG_INF("nRF Device Provisioning Sample");

	if (!IS_ENABLED(CONFIG_NRF_PROVISIONING_SYS_INIT)) {
		ret = nrf_modem_lib_init();
		if (ret < 0) {
			LOG_ERR("Unable to init modem library (%d)", ret);
			return 0;
		}

		LOG_INF("Establishing LTE link ...");
		ret = lte_lc_init_and_connect();
		if (ret) {
			LOG_ERR("LTE link could not be established (%d)", ret);
			return 0;
		}
	}

	ret = nrf_provisioning_init(&mmode, &dmode);
	if (ret) {
		LOG_ERR("Failed to initialize provisioning client");
	}

	return 0;
}

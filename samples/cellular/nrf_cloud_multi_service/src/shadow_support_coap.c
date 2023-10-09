/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <modem/location.h>
#include <date_time.h>
#include <stdio.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>

#include "cloud_connection.h"
#include "shadow_support_coap.h"

LOG_MODULE_REGISTER(shadow_support_coap, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define COAP_SHADOW_MAX_SIZE 512

static int check_shadow(void)
{
	int err;
	char buf[COAP_SHADOW_MAX_SIZE];
	struct nrf_cloud_data in_data = {
		.ptr = buf
	};

	/* Wait until we are able to communicate. */
	LOG_DBG("Waiting for valid connection before checking shadow");
	(void)await_cloud_ready(K_FOREVER);

	buf[0] = '\0';
	LOG_INF("Checking for shadow delta...");
	err = nrf_cloud_coap_shadow_get(buf, sizeof(buf), true);
	if (err == -EACCES) {
		LOG_DBG("Not connected yet.");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request shadow delta: %d", err);
		return err;
	}

	in_data.len = strlen(buf);
	if (!in_data.len) {
		LOG_INF("Checking again in %d seconds",
			CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS);
		return -EAGAIN;
	}
	LOG_DBG("Shadow delta: len:%zd, %s", in_data.len, buf);
	return nrf_cloud_coap_shadow_delta_process(&in_data);
}

#define SHADOW_THREAD_DELAY_S 10

int coap_shadow_thread_fn(void)
{
	int err;

	while (1) {
		err = check_shadow();
		if (err == -EAGAIN) {
			k_sleep(K_SECONDS(CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS));
		} else {
			k_sleep(K_SECONDS(SHADOW_THREAD_DELAY_S));
		}
	}
	return 0;
}

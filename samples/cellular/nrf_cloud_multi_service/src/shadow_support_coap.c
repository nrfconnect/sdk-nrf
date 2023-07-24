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

	/* Wait until we are able to communicate. */
	LOG_DBG("Waiting for valid connection before checking shadow");
	(void)await_cloud_ready(K_FOREVER);

	buf[0] = '\0';
	LOG_INF("Checking for shadow delta...");
	err = nrf_cloud_coap_shadow_get(buf, sizeof(buf), true);
	if (err == -EACCES) {
		LOG_DBG("Not connected yet.");
	} else if (err) {
		LOG_ERR("Failed to request shadow delta: %d", err);
	} else {
		size_t len = strlen(buf);

		if (len) {
			LOG_INF("Shadow delta: len:%zd, %s", len, buf);
			/* Do something with the shadow delta's JSON data, such
			 * as parse it and use the decoded information to change a
			 * behavior.
			 */

			/* Acknowledge it so we do not receive it again. */
			err = nrf_cloud_coap_shadow_state_update(buf);
			if (err) {
				LOG_ERR("Failed to acknowledge delta: %d", err);
			} else {
				LOG_DBG("Delta acknowledged");
			}
		} else {
			LOG_INF("Checking again in %d seconds",
				CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS);
			err = -EAGAIN;
		}
	}
	return err;
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

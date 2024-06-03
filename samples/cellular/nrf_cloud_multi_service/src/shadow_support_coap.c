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
#include "shadow_config.h"

LOG_MODULE_REGISTER(shadow_support_coap, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define COAP_SHADOW_MAX_SIZE 512

static int process_delta(struct nrf_cloud_data *const delta)
{
	if (delta->len == 0) {
		return -ENODATA;
	}

	int err;
	bool update_desired = false;
	struct nrf_cloud_obj delta_obj = {0};

	LOG_DBG("Shadow delta: len:%zd, %s", delta->len, (const char *)delta->ptr);

	err = nrf_cloud_coap_shadow_delta_process(delta, &delta_obj);
	if (err < 0) {
		LOG_ERR("Failed to process shadow delta, err: %d", err);
		return -EIO;
	} else if (err == 0) {
		/* No application specific delta data */
		return -ENODATA;
	}

	/* There is an application specific shadow delta to process.
	 * This application only needs to deal with the configuration section.
	 */
	err = shadow_config_delta_process(&delta_obj);
	if (err == -EBADF) {
		LOG_INF("Rejecting shadow delta");
		update_desired = true;
	} else if (err == -ENOMEM) {
		LOG_ERR("Error handling shadow delta");
		return err;
	}

	/* Reject the delta by updating desired.
	 * Accept the delta by updating reported.
	 */
	err = shadow_support_coap_obj_send(&delta_obj, update_desired);

	if (err) {
		LOG_ERR("Failed to send shadow delta update, err: %d", err);
		err = -EFAULT;
	}

	return err;
}

static int check_shadow(void)
{
	/* Ensure the config is sent once */
	static bool config_sent;

	int err;
	char buf[COAP_SHADOW_MAX_SIZE] = {0};
	size_t length = sizeof(buf);
	struct nrf_cloud_data in_data = {
		.ptr = buf
	};

	LOG_INF("Checking for shadow delta...");

	err = nrf_cloud_coap_shadow_get(buf, &length, true, COAP_CONTENT_FORMAT_APP_JSON);
	if (err == -EACCES) {
		LOG_DBG("Not connected yet.");
		return err;
	} else if (err) {
		LOG_ERR("Failed to request shadow delta: %d", err);
		return err;
	}

	in_data.len = length;

	err = process_delta(&in_data);
	if (err == -ENODATA) {
		/* There was no application specific delta data to process.
		 * Return -EAGAIN so the thread sleeps for a longer duration.
		 */
		err = -EAGAIN;
	}

	if (!config_sent) {
		/* If the config needs to be sent, return so that the thread sleeps
		 * for a shorter duration.
		 */
		if (shadow_config_reported_send() == 0) {
			config_sent = true;
			return 0;
		} else {
			return -EIO;
		}
	}

	return err;
}

int shadow_support_coap_obj_send(struct nrf_cloud_obj *const shadow_obj, const bool desired)
{
	/* Encode the data for the cloud */
	int err = nrf_cloud_obj_cloud_encode(shadow_obj);

	/* Free the object */
	(void)nrf_cloud_obj_free(shadow_obj);

	if (!err) {
		/* Send the encoded data */
		if (desired) {
			err = nrf_cloud_coap_shadow_desired_update(shadow_obj->encoded_data.ptr);
		} else {
			err = nrf_cloud_coap_shadow_state_update(shadow_obj->encoded_data.ptr);
		}
	} else {
		LOG_ERR("Failed to encode cloud data, err: %d", err);
		return err;
	}

	/* Free the encoded data */
	(void)nrf_cloud_obj_cloud_encoded_free(shadow_obj);

	return err;
}

#define SHADOW_THREAD_DELAY_S 10

int coap_shadow_thread_fn(void)
{
	int err;

	while (1) {
		/* Wait until we are able to communicate. */
		if (!await_cloud_ready(K_NO_WAIT)) {
			LOG_DBG("Waiting for valid connection before checking shadow");
			(void)await_cloud_ready(K_FOREVER);
		}

		err = check_shadow();
		if (err == -EAGAIN) {
			LOG_INF("Checking shadow again in %d seconds",
				CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS);
			k_sleep(K_SECONDS(CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS));
			continue;
		}
		if (err == -ETIMEDOUT) {
			cloud_transport_error_detected();
		}
		LOG_DBG("check_shadow() returned %d", err);
		k_sleep(K_SECONDS(SHADOW_THREAD_DELAY_S));
	}
	return 0;
}

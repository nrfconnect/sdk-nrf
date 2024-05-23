/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sdfw/sdfw_services/echo_service.h>
#include <sdfw/sdfw_services/reset_evt_service.h>
#include <sdfw/sdfw_services/sdfw_update_service.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssf_client_sample, CONFIG_SSF_CLIENT_SAMPLE_LOG_LEVEL);

#if CONFIG_IMPRIMATUR_ENABLED
#define IMPRIMATUR_UPDATE_IMAGE_OFFSET CONFIG_IMPRIMATUR_UPDATE_IMAGE_OFFSET
#else
#define IMPRIMATUR_UPDATE_IMAGE_OFFSET (0xe155000)
#endif

static void echo_request(void)
{
	int err;
	char req_str[] = "Hello " CONFIG_BOARD;
	char rsp_str[sizeof(req_str) + 1];

	LOG_INF("Calling ssf_echo, str: \"%s\"", req_str);

	err = ssf_echo(req_str, rsp_str, sizeof(rsp_str));
	if (err != 0) {
		LOG_ERR("ssf_echo failed");
	} else {
		LOG_INF("ssf_echo response: %s", rsp_str);
	}
}

static int reset_evt_callback(uint32_t domains, uint32_t delay_ms, void *user_data)
{
	LOG_INF("reset_evt: domains 0x%x will reset in %d ms", domains, delay_ms);

	return 0;
}

static void reset_evt_subscribe(void)
{
	int err;

	err = ssf_reset_evt_subscribe(reset_evt_callback, NULL);
	if (err != 0) {
		LOG_ERR("Unable to subscribe to reset_evt, err: %d", err);
		return;
	}
}

static void sdfw_update(void)
{
	int err;

	uintptr_t *image_data = (uintptr_t *)IMPRIMATUR_UPDATE_IMAGE_OFFSET;

	if (IS_ENABLED(CONFIG_UPDATE_REQUEST_CHECK_DATA) && *image_data == 0xFFFFFFFF) {
		LOG_WRN("No update download data.");
		return;
	}

	err = ssf_sdfw_update(IMPRIMATUR_UPDATE_IMAGE_OFFSET);
	if (err != 0) {
		LOG_ERR("Unable to perform sdfw update, err: %d", err);
		return;
	}
}

int main(void)
{
	LOG_INF("ssf client sample (%s)", CONFIG_BOARD);

	if (IS_ENABLED(CONFIG_ENABLE_ECHO_REQUEST)) {
		echo_request();
	}

	if (IS_ENABLED(CONFIG_ENABLE_RESET_EVT_SUBSCRIBE_REQUEST)) {
		reset_evt_subscribe();
	}

	if (IS_ENABLED(CONFIG_ENABLE_SDFW_UPDATE_REQUEST)) {
		sdfw_update();
	}

	return 0;
}

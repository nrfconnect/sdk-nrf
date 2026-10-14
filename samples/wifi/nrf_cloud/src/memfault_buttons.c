/* Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <memfault/metrics/metrics.h>
#include <memfault/ports/zephyr/http.h>
#if defined(CONFIG_SAMPLE_MEMFAULT_FOTA)
#include <memfault/ports/zephyr/fota.h>
#endif

#include "memfault_buttons.h"
#include "cloud_connection.h"

LOG_MODULE_REGISTER(memfault_buttons, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

/* DK_BTN1/DK_BTN2 map to the sw0/sw1 aliases, whose printed label differs between DKs */
#define POST_CHUNKS_BTN_MSK	DK_BTN1_MSK
#define POST_CHUNKS_BTN_LABEL	DT_PROP(DT_ALIAS(sw0), label)
#define FOTA_CHECK_BTN_MSK	DK_BTN2_MSK
#define FOTA_CHECK_BTN_LABEL	DT_PROP(DT_ALIAS(sw1), label)

#define POST_CHUNKS_EVT		BIT(0)
#define FOTA_CHECK_EVT		BIT(1)

static K_EVENT_DEFINE(button_events);

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t pressed = button_state & has_changed;

	/* Runs in the DK library's context: defer the (blocking) work to our own thread */
	if (pressed & POST_CHUNKS_BTN_MSK) {
		k_event_post(&button_events, POST_CHUNKS_EVT);
	}
	if (pressed & FOTA_CHECK_BTN_MSK) {
		k_event_post(&button_events, FOTA_CHECK_EVT);
	}
}

static void fota_check(void)
{
#if defined(CONFIG_SAMPLE_MEMFAULT_FOTA)
	int rv;

	LOG_INF("Checking for Memfault FOTA update");

	/* Check for any FOTA update and start its download if available */
	rv = memfault_zephyr_fota_start();
	if (rv < 0) {
		LOG_ERR("FOTA check failed, error: %d", rv);
	} else if (rv == 0) {
		LOG_INF("No FOTA update available");
	} else {
		LOG_INF("FOTA update download started");
	}
#else
	LOG_WRN("FOTA not enabled, build with coap-fota.conf");
#endif
}

static void post_chunks(void)
{
	ssize_t rv;

	/* Capture the metrics collected so far into a heartbeat now and post it */
	memfault_metrics_heartbeat_debug_trigger();

	LOG_INF("Posting Memfault chunks");

	rv = memfault_zephyr_port_post_data_return_size();
	if (rv < 0) {
		LOG_ERR("Posting Memfault chunks failed, error: %d", (int)rv);
	} else if (rv == 0) {
		LOG_INF("No Memfault data to post");
	} else {
		LOG_INF("Posted %d bytes of Memfault data", (int)rv);
	}
}

void memfault_buttons_thread_fn(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize buttons, error: %d", err);
		return;
	}

	LOG_INF("Press \"%s\" to post Memfault chunks, \"%s\" to check for a FOTA update",
		POST_CHUNKS_BTN_LABEL, FOTA_CHECK_BTN_LABEL);

	while (true) {
		uint32_t events = k_event_wait_safe(&button_events,
						    POST_CHUNKS_EVT | FOTA_CHECK_EVT,
						    false, K_FOREVER);

		if (!await_cloud_ready(K_NO_WAIT)) {
			LOG_WRN("Cloud not ready, ignoring button press");
			continue;
		}

		if (events & POST_CHUNKS_EVT) {
			post_chunks();
		}
		if (events & FOTA_CHECK_EVT) {
			fota_check();
		}
	}
}

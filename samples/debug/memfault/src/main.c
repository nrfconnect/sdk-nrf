/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr.h>
#include <memfault/metrics/metrics.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(memfault_sample, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK	      (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

static K_SEM_DEFINE(nw_connected_sem, 0, 1);

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

/* Recursive Fibonacci calculation used to trigger stack overflow. */
static int fib(int n)
{
	if (n <= 1) {
		return n;
	}

	return fib(n - 1) + fib(n - 2);
}

/* Handle button presses and trigger faults that can be captured and sent to
 * the Memfault cloud for inspection after rebooting:
 * Only button 1 is available on Thingy:91, the rest are available on nRF9160 DK.
 *	Button 1: Trigger stack overflow.
 *	Button 2: Trigger NULL-pointer dereference.
 *	Switch 1: Increment Switch1ToggleCount metric by one.
 *	Switch 2: Trace Switch2Toggled event, along with switch state.
 */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	uint32_t buttons_pressed = has_changed & button_states;

	if (buttons_pressed & DK_BTN1_MSK) {
		LOG_WRN("Stack overflow will now be triggered");
		fib(10000);
	} else if (buttons_pressed & DK_BTN2_MSK) {
		volatile uint32_t i;

		LOG_WRN("Division by zero will now be triggered");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
		i = 1 / 0;
#pragma GCC diagnostic pop
		ARG_UNUSED(i);
	} else if (has_changed & DK_BTN3_MSK) {
		/* DK_BTN3_MSK is Switch 1 on nRF9160 DK. */
		int err =
			memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(Switch1ToggleCount), 1);
		if (err) {
			LOG_ERR("Failed to increment Switch1ToggleCount");
		} else {
			LOG_INF("Switch1ToggleCount incremented");
		}
	} else if (has_changed & DK_BTN4_MSK) {
		/* DK_BTN4_MSK is Switch 2 on nRF9160 DK. */
		MEMFAULT_TRACE_EVENT_WITH_LOG(Switch2Toggled, "Switch state: %d",
					      buttons_pressed & DK_BTN4_MSK ? 1 : 0);
		LOG_INF("Switch2Toggled event has been traced, button state: %d",
			buttons_pressed & DK_BTN4_MSK ? 1 : 0);
	}
}

static void on_connect(void)
{
#if IS_ENABLED(MEMFAULT_NCS_LTE_METRICS)
	uint32_t time_to_lte_connection;

	/* Retrieve the LTE time to connect metric. */
	memfault_metrics_heartbeat_timer_read(MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect),
					      &time_to_lte_connection);

	LOG_INF("Time to connect: %d ms", time_to_lte_connection);
#endif /* IS_ENABLED(MEMFAULT_NCS_LTE_METRICS) */

	LOG_INF("Sending already captured data to Memfault");

	/* Trigger collection of heartbeat data. */
	memfault_metrics_heartbeat_debug_trigger();

	/* Check if there is any data available to be sent. */
	if (!memfault_packetizer_data_available()) {
		LOG_DBG("There was no data to be sent");
		return;
	}

	LOG_DBG("Sending stored data...");

	/* Send the data that has been captured to the memfault cloud.
	 * This will also happen periodically, with an interval that can be configured using
	 * CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS.
	 */
	memfault_zephyr_port_post_data();
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		k_sem_give(&nw_connected_sem);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		break;
	default:
		LOG_DBG("Unknown event: 0x%08X", event);
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		__ASSERT(false, "Failed to connect to a network");
		return;
	}
}

int main(void)
{
	int err;

	LOG_INF("Memfault sample has started");

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		__ASSERT(false, "conn_mgr_all_if_up, error: %d", err);
		return err;
	}

	/* Performing in an infinite loop to be resilient against
	 * re-connect bursts directly after boot, e.g. when connected
	 * to a roaming network or via weak signal. Note that
	 * Memfault data will be uploaded periodically every
	 * CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS.
	 * We post data here so as soon as a connection is available
	 * the latest data will be pushed to Memfault.
	 */

	while (1) {
		k_sem_take(&nw_connected_sem, K_FOREVER);
		LOG_INF("Connected to network");
		on_connect();
	}
}

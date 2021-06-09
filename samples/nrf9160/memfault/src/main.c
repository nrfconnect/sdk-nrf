/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <net/socket.h>
#include <dk_buttons_and_leds.h>

#include <memfault/metrics/metrics.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(memfault_sample, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

static K_SEM_DEFINE(lte_connected, 0, 1);

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");

		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("Active LTE mode changed: %s",
			evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			"Unknown");
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		k_sem_give(&lte_connected);
	} else {
		int err;

		err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d", err);
			return;
		}

		/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
		 * lte_handler() to determine when the LTE link is up.
		 */
	}
#endif
}

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
		uint32_t *ptr = NULL;
		volatile uint32_t i = *ptr;

		LOG_WRN("NULL pointer de-reference will now be triggered");

		(void)i;
	} else if (has_changed & DK_BTN3_MSK) {
		/* DK_BTN3_MSK is Switch 1 on nRF9160 DK. */
		int err = memfault_metrics_heartbeat_add(
			MEMFAULT_METRICS_KEY(Switch1ToggleCount), 1);

		if (err) {
			LOG_ERR("Failed to increment Switch1ToggleCount");
		} else {
			LOG_INF("Switch1ToggleCount incremented");
		}
	} else if (has_changed & DK_BTN4_MSK) {
		/* DK_BTN4_MSK is Switch 2 on nRF9160 DK. */
		MEMFAULT_TRACE_EVENT_WITH_LOG(Switch2Toggled,
					      "Switch state: %d",
					      buttons_pressed & DK_BTN4_MSK ? 1 : 0);
	}
}

void main(void)
{
	int err;
	uint32_t time_to_lte_connection;

	LOG_INF("Memfault sample has started");

	modem_configure();

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}

	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);

	/* Retrieve the LTE time to connect metric. */
	memfault_metrics_heartbeat_timer_read(
		MEMFAULT_METRICS_KEY(Ncs_LteTimeToConnect),
		&time_to_lte_connection);

	LOG_INF("Connected to LTE network. Time to connect: %d ms", time_to_lte_connection);
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

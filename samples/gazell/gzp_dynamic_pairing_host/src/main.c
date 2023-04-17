/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>
#include <nrf_gzll.h>
#include <gzll_glue.h>
#include <gzp.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

/* Pipes 0 and 1 are reserved for GZP pairing and data. See gzp.h. */
#define UNENCRYPTED_DATA_PIPE     2

/* RXPERIOD/2 on LU1 = timeslot period on nRF5x. */
#define NRF_GZLLDE_RXPERIOD_DIV_2 504

static void led_output(uint8_t value)
{
	int err;

	err = dk_set_leds(value & DK_ALL_LEDS_MSK);
	if (err) {
		LOG_ERR("Cannot output LEDs (err: %d)", err);
	}
}

int main(void)
{
	/* Debug helper variables */
	uint32_t length;

	/* Data and acknowledgment payloads */
	uint8_t payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];

	int err;
	bool result_value;

	settings_subsys_init();

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot initialize LEDs (err: %d)", err);
		return 0;
	}

	/* Initialize Gazell Link Layer glue */
	result_value = gzll_glue_init();
	if (!result_value) {
		LOG_ERR("Cannot initialize GZLL glue code");
		return 0;
	}

	/* Initialize the Gazell Link Layer */
	result_value = nrf_gzll_init(NRF_GZLL_MODE_HOST);
	if (!result_value) {
		LOG_ERR("Cannot initialize GZLL");
		return 0;
	}

	result_value = nrf_gzll_set_timeslot_period(NRF_GZLLDE_RXPERIOD_DIV_2);
	if (!result_value) {
		LOG_ERR("Cannot set GZLL timeslot period");
		return 0;
	}

	/* Initialize the Gazell Pairing Library */
	gzp_init();

	result_value = nrf_gzll_set_rx_pipes_enabled(nrf_gzll_get_rx_pipes_enabled() |
						     (1 << UNENCRYPTED_DATA_PIPE));
	if (!result_value) {
		LOG_ERR("Cannot set GZLL RX pipes");
		return 0;
	}

	gzp_pairing_enable(true);

	result_value = nrf_gzll_enable();
	if (!result_value) {
		LOG_ERR("Cannot enable GZLL");
		return 0;
	}

	LOG_INF("Gazell dynamic pairing example started. Host mode.");

	for (;;) {
		gzp_host_execute();

		/* If a Host ID request received */
		if (gzp_id_req_received()) {
			/* Always grant a request */
			gzp_id_req_grant();
		}

		length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

		if (nrf_gzll_get_rx_fifo_packet_count(UNENCRYPTED_DATA_PIPE)) {
			if (nrf_gzll_fetch_packet_from_rx_fifo(UNENCRYPTED_DATA_PIPE,
							       payload,
							       &length)) {
				led_output(payload[0]);
			}
		} else if (gzp_crypt_user_data_received()) {
			if (gzp_crypt_user_data_read(payload, (uint8_t *)&length)) {
				led_output(payload[0]);
			}
		}
	}
}

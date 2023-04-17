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

/* Ensure that we try all channels before giving up */
#define MAX_TX_ATTEMPTS (NRF_GZLL_DEFAULT_TIMESLOTS_PER_CHANNEL_WHEN_DEVICE_OUT_OF_SYNC * \
			 NRF_GZLL_DEFAULT_CHANNEL_TABLE_SIZE)

static K_SEM_DEFINE(tx_complete_sem, 0, 1);

static bool gzp_tx_is_success;

static void gzp_tx_result_checker(bool success,
				  uint32_t pipe,
				  const nrf_gzll_device_tx_info_t *tx_info)
{
	if (pipe == UNENCRYPTED_DATA_PIPE) {
		gzp_tx_is_success = success;
		k_sem_give(&tx_complete_sem);
	}
}

int main(void)
{
	int err;
	bool result_value;
	bool tx_success = false;
	bool send_crypt_data = false;
	enum gzp_id_req_res id_req_status = GZP_ID_RESP_NO_REQUEST;

	/* Data and acknowledgment payloads */
	uint8_t payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];

	err = dk_buttons_init(NULL);
	if (err) {
		LOG_ERR("Cannot initialize buttons (err: %d)", err);
		return 0;
	}

	settings_subsys_init();

	/* Initialize Gazell Link Layer glue */
	result_value = gzll_glue_init();
	if (!result_value) {
		LOG_ERR("Cannot initialize GZLL glue code");
		return 0;
	}

	/* Initialize the Gazell Link Layer */
	result_value = nrf_gzll_init(NRF_GZLL_MODE_DEVICE);
	if (!result_value) {
		LOG_ERR("Cannot initialize GZLL");
		return 0;
	}

	nrf_gzll_set_max_tx_attempts(MAX_TX_ATTEMPTS);

	result_value = nrf_gzll_set_timeslot_period(NRF_GZLLDE_RXPERIOD_DIV_2);
	if (!result_value) {
		LOG_ERR("Cannot set GZLL timeslot period");
		return 0;
	}

	/* Erase pairing data. This sample is intended to demonstrate pairing after every reset. */
	gzp_erase_pairing_data();

	/* Initialize the Gazell Pairing Library */
	gzp_init();

	gzp_tx_result_callback_register(gzp_tx_result_checker);

	result_value = nrf_gzll_enable();
	if (!result_value) {
		LOG_ERR("Cannot enable GZLL");
		return 0;
	}

	LOG_INF("Gazell dynamic pairing example started. Device mode.");

	for (;;) {
		payload[0] = ~dk_get_buttons();

		/* Send every other packet as encrypted data. */
		if (send_crypt_data) {
			/* Send encrypted packet using the Gazell pairing library. */
			tx_success = gzp_crypt_data_send(payload,
							 GZP_ENCRYPTED_USER_DATA_MAX_LENGTH);
		} else {
			k_sem_reset(&tx_complete_sem);
			gzp_tx_is_success = false;

			/* Send packet as plain text. */
			if (nrf_gzll_add_packet_to_tx_fifo(UNENCRYPTED_DATA_PIPE,
							   payload,
							   GZP_MAX_FW_PAYLOAD_LENGTH)) {
				k_sem_take(&tx_complete_sem, K_FOREVER);
				tx_success = gzp_tx_is_success;
			} else {
				LOG_ERR("TX fifo error");
			}
		}
		send_crypt_data = !send_crypt_data;

		/* Check if data transfer failed. */
		if (!tx_success) {
			LOG_ERR("Gazelle: transmission failed");

			/* Send "system address request". Needed for sending any
			 * user data to the host.
			 */
			if (gzp_address_req_send()) {
				/* Send "Host ID request". Needed for sending
				 * encrypted user data to the host.
				 */
				id_req_status = gzp_id_req_send();
			} else {
				/* System address request failed. */
			}
		}

		/* If waiting for the host to grant or reject an ID request. */
		if (id_req_status == GZP_ID_RESP_PENDING) {
			/* Send a new ID request for fetching response. */
			id_req_status = gzp_id_req_send();
		}
	}
}

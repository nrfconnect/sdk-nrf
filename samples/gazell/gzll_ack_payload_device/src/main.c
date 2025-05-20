/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_gzll.h>
#include <gzll_glue.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

/* Pipe 0 is used in this example. */
#define PIPE_NUMBER 0

/* 1-byte payload length is used when transmitting. */
#define TX_PAYLOAD_LENGTH 1

/* Maximum number of transmission attempts */
#define MAX_TX_ATTEMPTS 100

/* Gazell Link Layer TX result structure */
struct gzll_tx_result {
	bool success;
	uint32_t pipe;
	nrf_gzll_device_tx_info_t info;
};

/* Payload to send to Host. */
static uint8_t data_payload[TX_PAYLOAD_LENGTH];

/* Placeholder for received ACK payloads from Host. */
static uint8_t ack_payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];

#ifdef CONFIG_GZLL_TX_STATISTICS
/* Struct containing transmission statistics. */
static nrf_gzll_tx_statistics_t statistics;
static nrf_gzll_tx_statistics_t statistics_2;
#endif

/* Gazell Link Layer TX result message queue */
K_MSGQ_DEFINE(gzll_msgq,
	      sizeof(struct gzll_tx_result),
	      1,
	      sizeof(uint32_t));

/* Main loop semaphore */
static K_SEM_DEFINE(main_sem, 0, 1);

static struct k_work gzll_work;

static void gzll_tx_result_handler(struct gzll_tx_result *tx_result);
static void gzll_work_handler(struct k_work *work);

int main(void)
{
	int err;
	bool result_value;

	k_work_init(&gzll_work, gzll_work_handler);

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot initialize LEDs (err: %d)", err);
		return 0;
	}

	err = dk_buttons_init(NULL);
	if (err) {
		LOG_ERR("Cannot initialize buttons (err: %d)", err);
		return 0;
	}

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

#ifdef CONFIG_GZLL_TX_STATISTICS
	/* Turn on transmission statistics gathering. */
	result_value = nrf_gzll_tx_statistics_enable(&statistics);
	if (!result_value) {
		LOG_ERR("Cannot enable GZLL TX statistics");
		return 0;
	}
#endif

	data_payload[0] = ~dk_get_buttons();

	result_value = nrf_gzll_add_packet_to_tx_fifo(PIPE_NUMBER, data_payload, TX_PAYLOAD_LENGTH);
	if (!result_value) {
		LOG_ERR("Cannot add packet to GZLL TX FIFO");
		return 0;
	}

	result_value = nrf_gzll_enable();
	if (!result_value) {
		LOG_ERR("Cannot enable GZLL");
		return 0;
	}

	LOG_INF("Gzll ack payload device example started.");

	while (true) {
		if (k_sem_take(&main_sem, K_FOREVER)) {
			continue;
		}

#ifdef CONFIG_GZLL_TX_STATISTICS
		if (statistics.packets_num >= 1000) {
			uint32_t key = irq_lock();

			statistics_2 = statistics;

			/* Reset statistics buffers. */
			nrf_gzll_reset_tx_statistics();

			irq_unlock(key);

			/* Print all transmission statistics. */
			LOG_INF("");
			LOG_INF("Total transmitted packets:   %4u",  statistics_2.packets_num);
			LOG_INF("Total transmission time-outs: %03u", statistics_2.timeouts_num);
			LOG_INF("");

			for (uint8_t i = 0; i < nrf_gzll_get_channel_table_size(); i++) {
				LOG_INF(
				"Channel %u: %03u packets transmitted, %03u transmissions failed.",
				i,
				statistics_2.channel_packets[i],
				statistics_2.channel_timeouts[i]);
			}
		}
#endif
	}
}

static void gzll_device_report_tx(bool success,
				  uint32_t pipe,
				  nrf_gzll_device_tx_info_t *tx_info)
{
	int err;
	struct gzll_tx_result tx_result;

	tx_result.success = success;
	tx_result.pipe = pipe;
	tx_result.info = *tx_info;
	err = k_msgq_put(&gzll_msgq, &tx_result, K_NO_WAIT);
	if (!err) {
		/* Get work handler to run */
		k_work_submit(&gzll_work);
	} else {
		LOG_ERR("Cannot put TX result to message queue");
	}
}

void nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
	gzll_device_report_tx(true, pipe, &tx_info);
}

void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
	gzll_device_report_tx(false, pipe, &tx_info);
}

void nrf_gzll_disabled(void)
{
}

void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{
}

static void gzll_work_handler(struct k_work *work)
{
	struct gzll_tx_result tx_result;

	/* Process message queue */
	while (!k_msgq_get(&gzll_msgq, &tx_result, K_NO_WAIT)) {
		gzll_tx_result_handler(&tx_result);
	}

	/* Get main loop to run */
	k_sem_give(&main_sem);
}

static void gzll_tx_result_handler(struct gzll_tx_result *tx_result)
{
	int err;
	bool result_value;
	uint32_t ack_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

	if (tx_result->success) {
		if (tx_result->info.payload_received_in_ack) {
			/* Pop packet and write first byte of the payload to the GPIO port. */
			result_value = nrf_gzll_fetch_packet_from_rx_fifo(tx_result->pipe,
									  ack_payload,
									  &ack_payload_length);
			if (!result_value) {
				LOG_ERR("RX fifo error");
			} else if (ack_payload_length > 0) {
				err = dk_set_leds(ack_payload[0] & DK_ALL_LEDS_MSK);
				if (err) {
					LOG_ERR("Cannot output LEDs (err: %d)", err);
				}
			}
		}
	} else {
		LOG_ERR("Gazell transmission failed");
	}

	/* Load data payload into the TX queue. */
	data_payload[0] = ~dk_get_buttons();
	result_value = nrf_gzll_add_packet_to_tx_fifo(tx_result->pipe,
						      data_payload,
						      TX_PAYLOAD_LENGTH);
	if (!result_value) {
		LOG_ERR("TX fifo error");
	}
}

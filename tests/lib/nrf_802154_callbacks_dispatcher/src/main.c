/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <net/nrf_802154_callbacks_dispatcher.h>
#include <nrf_802154_callouts.h>
#include <nrf_802154_types.h>
#include <zephyr/ztest.h>

#include "nrf_802154_stubs.h"

struct client_test_stats {
	int init_count;
	int deinit_count;
	int received_count;
	int receive_failed_count;
	int tx_ack_started_count;
	int transmitted_count;
	int transmit_failed_count;
	int energy_detected_count;
	int energy_detection_failed_count;
};

static struct client_test_stats client_a_stats;
static struct client_test_stats client_b_stats;

static struct nrf_802154_radio_client_config client_a_config = {
	.pan_id = { 0x34, 0x12 },
	.short_address = { 0x56, 0x78 },
	.mac = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 },
};

static void client_a_init(void)
{
	client_a_stats.init_count++;
}

static void client_a_deinit(void)
{
	client_a_stats.deinit_count++;
}

static void client_a_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	ARG_UNUSED(data);
	ARG_UNUSED(power);
	ARG_UNUSED(lqi);
	ARG_UNUSED(time);
	client_a_stats.received_count++;
}

static void client_a_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
	ARG_UNUSED(error);
	ARG_UNUSED(id);
	client_a_stats.receive_failed_count++;
}

static void client_a_tx_ack_started(const uint8_t *data)
{
	ARG_UNUSED(data);
	client_a_stats.tx_ack_started_count++;
}

static void client_a_transmitted_raw(uint8_t *frame,
				     const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(metadata);
	client_a_stats.transmitted_count++;
}

static void client_a_transmit_failed(uint8_t *frame, nrf_802154_tx_error_t error,
				     const nrf_802154_transmit_done_metadata_t *metadata)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(error);
	ARG_UNUSED(metadata);
	client_a_stats.transmit_failed_count++;
}

static void client_a_energy_detected(const nrf_802154_energy_detected_t *result)
{
	ARG_UNUSED(result);
	client_a_stats.energy_detected_count++;
}

static void client_a_energy_detection_failed(nrf_802154_ed_error_t error)
{
	ARG_UNUSED(error);
	client_a_stats.energy_detection_failed_count++;
}

static struct nrf_802154_radio_client_config *client_a_get_config(void)
{
	return &client_a_config;
}

static void client_b_init(void)
{
	client_b_stats.init_count++;
}

static void client_b_deinit(void)
{
	client_b_stats.deinit_count++;
}

static void client_b_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	ARG_UNUSED(data);
	ARG_UNUSED(power);
	ARG_UNUSED(lqi);
	ARG_UNUSED(time);
	client_b_stats.received_count++;
}

static struct nrf_802154_callbacks client_a_cbs = {
	.init = client_a_init,
	.deinit = client_a_deinit,
	.received_timestamp_raw = client_a_received_timestamp_raw,
	.receive_failed = client_a_receive_failed,
	.tx_ack_started = client_a_tx_ack_started,
	.transmitted_raw = client_a_transmitted_raw,
	.transmit_failed = client_a_transmit_failed,
	.energy_detected = client_a_energy_detected,
	.energy_detection_failed = client_a_energy_detection_failed,
	.get_config = client_a_get_config,
};

static struct nrf_802154_callbacks client_b_cbs = {
	.init = client_b_init,
	.deinit = client_b_deinit,
	.received_timestamp_raw = client_b_received_timestamp_raw,
};

NRF_802154_CALLBACKS_DISPATCHER_REGISTER(client_a, client_a_cbs);
NRF_802154_CALLBACKS_DISPATCHER_REGISTER(client_b, client_b_cbs);

static void reset_client_stats(void)
{
	memset(&client_a_stats, 0, sizeof(client_a_stats));
	memset(&client_b_stats, 0, sizeof(client_b_stats));
}

static void *suite_setup(void)
{
	nrf_802154_stub_reset();
	reset_client_stats();

	return NULL;
}

static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)nrf_802154_callbacks_dispatcher_switch(NULL);
	nrf_802154_stub_reset();
	reset_client_stats();
}

ZTEST_SUITE(nrf_802154_callbacks_dispatcher, NULL, suite_setup, test_setup, NULL, NULL);

ZTEST(nrf_802154_callbacks_dispatcher, test_switch_unknown_client)
{
	zassert_equal(nrf_802154_callbacks_dispatcher_switch("unknown"), -EINVAL);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_switch_to_client_initializes_and_applies_config)
{
	uint32_t reinit_before = nrf_802154_stub_stats.reinit_count;

	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));

	zassert_equal(client_a_stats.init_count, 1);
	zassert_equal(client_a_stats.deinit_count, 0);
	zassert_equal(nrf_802154_stub_stats.reinit_count, reinit_before + 1);
	zassert_equal(nrf_802154_stub_stats.pan_id_set_count, 1);
	zassert_equal(nrf_802154_stub_stats.short_address_set_count, 1);
	zassert_equal(nrf_802154_stub_stats.extended_address_set_count, 1);
	zassert_mem_equal(nrf_802154_stub_stats.last_pan_id, client_a_config.pan_id, PAN_ID_SIZE);
	zassert_mem_equal(nrf_802154_stub_stats.last_short_address, client_a_config.short_address,
			  SHORT_ADDRESS_SIZE);
	zassert_mem_equal(nrf_802154_stub_stats.last_extended_address, client_a_config.mac,
			  EXTENDED_ADDRESS_SIZE);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_switch_same_client_is_noop)
{
	uint32_t reinit_before;

	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));
	reinit_before = nrf_802154_stub_stats.reinit_count;

	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));

	zassert_equal(client_a_stats.init_count, 1);
	zassert_equal(client_a_stats.deinit_count, 0);
	zassert_equal(nrf_802154_stub_stats.reinit_count, reinit_before);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_switch_between_clients)
{
	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));
	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_b"));

	zassert_equal(client_a_stats.init_count, 1);
	zassert_equal(client_a_stats.deinit_count, 1);
	zassert_equal(client_b_stats.init_count, 1);
	zassert_equal(client_b_stats.deinit_count, 0);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_switch_to_null_deactivates_client)
{
	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));
	zassert_ok(nrf_802154_callbacks_dispatcher_switch(NULL));

	zassert_equal(client_a_stats.deinit_count, 1);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_callbacks_forward_to_active_client)
{
	uint8_t frame[16];
	nrf_802154_transmit_done_metadata_t metadata = { 0 };
	nrf_802154_energy_detected_t ed_result = { 0 };

	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));

	nrf_802154_received_timestamp_raw(frame, -50, 200, 1234);
	nrf_802154_receive_failed(NRF_802154_RX_ERROR_INVALID_FRAME, 42);
	nrf_802154_tx_ack_started(frame);
	nrf_802154_transmitted_raw(frame, &metadata);
	nrf_802154_transmit_failed(frame, NRF_802154_TX_ERROR_NO_ACK, &metadata);
	nrf_802154_energy_detected(&ed_result);
	nrf_802154_energy_detection_failed(NRF_802154_ED_ERROR_ABORTED);

	zassert_equal(client_a_stats.received_count, 1);
	zassert_equal(client_a_stats.receive_failed_count, 1);
	zassert_equal(client_a_stats.tx_ack_started_count, 1);
	zassert_equal(client_a_stats.transmitted_count, 1);
	zassert_equal(client_a_stats.transmit_failed_count, 1);
	zassert_equal(client_a_stats.energy_detected_count, 1);
	zassert_equal(client_a_stats.energy_detection_failed_count, 1);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_callbacks_without_active_client_are_noop)
{
	uint8_t frame[16];
	nrf_802154_transmit_done_metadata_t metadata = { 0 };
	nrf_802154_energy_detected_t ed_result = { 0 };

	nrf_802154_received_timestamp_raw(frame, -50, 200, 1234);
	nrf_802154_receive_failed(NRF_802154_RX_ERROR_INVALID_FRAME, 42);
	nrf_802154_tx_ack_started(frame);
	nrf_802154_transmitted_raw(frame, &metadata);
	nrf_802154_transmit_failed(frame, NRF_802154_TX_ERROR_NO_ACK, &metadata);
	nrf_802154_energy_detected(&ed_result);
	nrf_802154_energy_detection_failed(NRF_802154_ED_ERROR_ABORTED);

	zassert_equal(client_a_stats.received_count, 0);
	zassert_equal(client_a_stats.receive_failed_count, 0);
	zassert_equal(client_a_stats.tx_ack_started_count, 0);
	zassert_equal(client_a_stats.transmitted_count, 0);
	zassert_equal(client_a_stats.transmit_failed_count, 0);
	zassert_equal(client_a_stats.energy_detected_count, 0);
	zassert_equal(client_a_stats.energy_detection_failed_count, 0);
	zassert_equal(client_b_stats.received_count, 0);
	zassert_equal(client_b_stats.receive_failed_count, 0);
	zassert_equal(client_b_stats.tx_ack_started_count, 0);
	zassert_equal(client_b_stats.transmitted_count, 0);
	zassert_equal(client_b_stats.transmit_failed_count, 0);
	zassert_equal(client_b_stats.energy_detected_count, 0);
	zassert_equal(client_b_stats.energy_detection_failed_count, 0);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_callbacks_follow_client_switch)
{
	uint8_t frame[16];

	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));
	nrf_802154_received_timestamp_raw(frame, 0, 0, 0);
	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_b"));
	nrf_802154_received_timestamp_raw(frame, 0, 0, 0);

	zassert_equal(client_a_stats.received_count, 1);
	zassert_equal(client_b_stats.received_count, 1);
}

ZTEST(nrf_802154_callbacks_dispatcher, test_reinit_resets_radio_state)
{
	zassert_ok(nrf_802154_callbacks_dispatcher_switch("client_a"));

	zassert_true(nrf_802154_stub_stats.transmit_at_cancel_count > 0);
	zassert_true(nrf_802154_stub_stats.promiscuous_set_count > 0);
	zassert_true(nrf_802154_stub_stats.pan_coord_set_count > 0);
	zassert_true(nrf_802154_stub_stats.auto_ack_set_count > 0);
	zassert_true(nrf_802154_stub_stats.auto_pending_bit_set_count > 0);
	zassert_equal(nrf_802154_stub_stats.pending_bit_for_addr_reset_count, 2);
	zassert_true(nrf_802154_stub_stats.tx_power_set_count > 0);
	zassert_equal(nrf_802154_stub_stats.ack_data_remove_all_count, 2);
}

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements the callbacks dispatcher for the nRF IEEE802.15.4 radio driver.
 */

#include "nrf_802154_callbacks_dispatcher.h"
#include <errno.h>
#include <nrf_802154.h>
#include <nrf_802154_types.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(nrf_802154_callbacks_dispatcher, LOG_LEVEL_INF);

static struct nrf_802154_cb_dispatch_entry *active_entry;
static struct k_spinlock dispatcher_lock;
static K_MUTEX_DEFINE(switch_mutex);

static struct nrf_802154_cb_dispatch_entry *entry_lookup(const char *name)
{
	STRUCT_SECTION_FOREACH(nrf_802154_cb_dispatch_entry, entry)
	{
		if (entry->name != NULL && strcmp(entry->name, name) == 0) {
			return entry;
		}
	}

	return NULL;
}

/**
 * @brief Get the active client entry.
 *
 * @return The active client entry.
 */
static struct nrf_802154_cb_dispatch_entry *active_entry_get(void)
{
	struct nrf_802154_cb_dispatch_entry *entry;
	k_spinlock_key_t key = k_spin_lock(&dispatcher_lock);

	entry = active_entry;

	k_spin_unlock(&dispatcher_lock, key);

	return entry;
}

/**
 * @brief Set the active client.
 *
 * @param entry The client to set as active.
 *
 * This function sets the active client.
 */
static void active_entry_set(struct nrf_802154_cb_dispatch_entry *entry)
{
	k_spinlock_key_t key = k_spin_lock(&dispatcher_lock);

	active_entry = entry;

	k_spin_unlock(&dispatcher_lock, key);
}

/**
 * @brief Get the active client callbacks.
 *
 * @return The active client callbacks.
 */
static const struct nrf_802154_callbacks *active_client_callbacks(void)
{
	struct nrf_802154_cb_dispatch_entry *entry = active_entry_get();

	return entry != NULL ? entry->callbacks : NULL;
}

static void reinit_radio_driver(void)
{
	(void)nrf_802154_transmit_at_cancel();
	nrf_802154_promiscuous_set(false);
	nrf_802154_pan_coord_set(false);
	nrf_802154_auto_ack_set(false);
	nrf_802154_auto_pending_bit_set(true);
	nrf_802154_pending_bit_for_addr_reset(false);
	nrf_802154_pending_bit_for_addr_reset(true);
	nrf_802154_tx_power_set(0);

	nrf_802154_ack_data_remove_all(false, NRF_802154_ACK_DATA_IE);
	nrf_802154_ack_data_remove_all(true, NRF_802154_ACK_DATA_IE);

	nrf_802154_reinit();
}

int nrf_802154_callbacks_dispatcher_switch(const char *name)
{
	struct nrf_802154_cb_dispatch_entry *prev_entry;
	struct nrf_802154_cb_dispatch_entry *next_entry = NULL;
	const struct nrf_802154_callbacks *prev_client;
	const struct nrf_802154_callbacks *next_client;

	/* Check whether the new client is valid and exists */
	if (name != NULL && name[0] != '\0') {
		next_entry = entry_lookup(name);
		if (next_entry == NULL) {
			LOG_ERR("No client found with name: %s", name ? name : "null");
			return -EINVAL;
		}
	}

	/* Lock the switch mutex to prevent concurrent switches */
	k_mutex_lock(&switch_mutex, K_FOREVER);

	/* Get the current active client and its callbacks */
	prev_entry = active_entry_get();
	prev_client = prev_entry != NULL ? prev_entry->callbacks : NULL;
	next_client = next_entry != NULL ? next_entry->callbacks : NULL;

	/* If the new client is the same as the current active client, no switch is needed */
	if (prev_entry == next_entry) {
		LOG_INF("No switch needed for %s", next_entry != NULL ? next_entry->name : "null");
		k_mutex_unlock(&switch_mutex);
		return 0;
	}

	/* Deinitialize the previous client, and save its configuration to be restored later if
	 * needed
	 */
	if (prev_client != NULL && prev_client->deinit != NULL) {
		prev_client->deinit();
	}

	/* Reinitialize the radio driver */
	reinit_radio_driver();

	/* Initialize the new client */
	if (next_client != NULL && next_client->init != NULL) {
		next_client->init();
		if (next_client->get_config != NULL) {
			struct nrf_802154_radio_client_config *config = next_client->get_config();

			if (config != NULL) {
				nrf_802154_pan_id_set(config->pan_id);
				nrf_802154_short_address_set(config->short_address);
				nrf_802154_extended_address_set(config->mac);
			}
		}
	}

	active_entry_set(next_entry);
	k_mutex_unlock(&switch_mutex);

	LOG_INF("Switched client: %s", next_entry->name);

	return 0;
}

void nrf_802154_received_timestamp_raw(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->received_timestamp_raw != NULL) {
		cb->received_timestamp_raw(data, power, lqi, time);
	}
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->receive_failed != NULL) {
		cb->receive_failed(error, id);
	}
}

void nrf_802154_tx_ack_started(const uint8_t *data)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->tx_ack_started != NULL) {
		cb->tx_ack_started(data);
	}
}

void nrf_802154_transmitted_raw(uint8_t *frame, const nrf_802154_transmit_done_metadata_t *metadata)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->transmitted_raw != NULL) {
		cb->transmitted_raw(frame, metadata);
	}
}

void nrf_802154_transmit_failed(uint8_t *frame, nrf_802154_tx_error_t error,
				const nrf_802154_transmit_done_metadata_t *metadata)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->transmit_failed != NULL) {
		cb->transmit_failed(frame, error, metadata);
	}
}

void nrf_802154_energy_detected(const nrf_802154_energy_detected_t *result)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->energy_detected != NULL) {
		cb->energy_detected(result);
	}
}

void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->energy_detection_failed != NULL) {
		cb->energy_detection_failed(error);
	}
}

#if defined(CONFIG_NRF_802154_SER_HOST)
void nrf_802154_serialization_error(const nrf_802154_ser_err_data_t *err)
{
	const struct nrf_802154_callbacks *cb = active_client_callbacks();

	if (cb != NULL && cb->serialization_error != NULL) {
		cb->serialization_error(err);
	}
}
#endif

static int nrf_802154_callbacks_dispatcher_init(void)
{
	/* Initialize the radio driver since it is needed for MPSL to work.
	 * At this point no client is active, so a user must call
	 * the switch function to activate a client before using the radio.
	 */
	nrf_802154_init();

	return 0;
}

SYS_INIT(nrf_802154_callbacks_dispatcher_init, POST_KERNEL,
	 CONFIG_NRF_802154_CALLBACKS_DISPATCHER_INIT_PRIORITY);

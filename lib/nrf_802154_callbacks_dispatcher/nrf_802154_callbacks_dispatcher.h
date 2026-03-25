/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_802154_CALLBACKS_DISPATCHER_H
#define NRF_802154_CALLBACKS_DISPATCHER_H

#include <nrf_802154_const.h>
#include <nrf_802154_types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/sys/iterable_sections.h>

/** Invalid client index (no active client). */
#define NRF_802154_CALLBACKS_DISPATCHER_INDEX_NONE UINT32_MAX

struct nrf_802154_radio_client_config {
	uint8_t pan_id[PAN_ID_SIZE];
	uint8_t short_address[SHORT_ADDRESS_SIZE];
	uint8_t mac[EXTENDED_ADDRESS_SIZE];
};

/**
 * @brief Callbacks for the nRF IEEE 802.15.4 radio driver.
 *
 * All callbacks are optional and may be set to NULL if not used.
 */
struct nrf_802154_callbacks {
	void (*init)(void);
	void (*deinit)(void);
	void (*received_timestamp_raw)(uint8_t *data, int8_t power, uint8_t lqi, uint64_t time);
	void (*receive_failed)(nrf_802154_rx_error_t error, uint32_t id);
	void (*tx_ack_started)(const uint8_t *data);
	void (*transmitted_raw)(uint8_t *frame,
				const nrf_802154_transmit_done_metadata_t *metadata);
	void (*transmit_failed)(uint8_t *frame, nrf_802154_tx_error_t error,
				const nrf_802154_transmit_done_metadata_t *metadata);
	void (*energy_detected)(const nrf_802154_energy_detected_t *result);
	void (*energy_detection_failed)(nrf_802154_ed_error_t error);
	struct nrf_802154_radio_client_config *(*get_config)(void);
#if defined(CONFIG_NRF_802154_SER_HOST)
	void (*serialization_error)(const nrf_802154_ser_err_data_t *err);
#endif
};

/**
 * @brief Entry for static registration in the iterable section.
 */
struct nrf_802154_cb_dispatch_entry {
	/* Unique name for this registration (e.g. openthread). */
	const char *name;
	/* Variable of type struct nrf_802154_callbacks. */
	const struct nrf_802154_callbacks *callbacks;
	/* Whether the configuration has been saved. */
	bool saved_configuration;
	/* Extended address in format of
	 * 0x00:0x01:0x02:0x03:0x04:0x05:0x06:0x07.
	 */
	uint8_t ext_address[EXTENDED_ADDRESS_SIZE];
	/* Short address in format of 0x0000. */
	uint16_t short_address;
	/* PAN ID in little-endian byte format expected by nrf_802154_pan_id_* APIs. */
	uint8_t pan_id[PAN_ID_SIZE];
};

/** Helper to stringify after argument expansion */
#define NRF_802154_CALLBACKS_DISPATCHER_NAME_STR(x) #x
#define NRF_802154_CALLBACKS_DISPATCHER_NAME_STR_EXPAND(x)                                         \
	NRF_802154_CALLBACKS_DISPATCHER_NAME_STR(x)

/**
 * @brief Statically register a client.
 *
 * Place this macro in file scope (e.g. in your protocol's radio driver implementation file).
 *
 * @param _entry_name Unique name for this registration (e.g. openthread).
 * Pass the same string to nrf_802154_callbacks_dispatcher_switch() function to
 * switch to this client's callbacks.
 * @param _callbacks_var Variable of type struct nrf_802154_callbacks.
 */
#define NRF_802154_CALLBACKS_DISPATCHER_REGISTER(_entry_name, _callbacks_var)                      \
	static STRUCT_SECTION_ITERABLE(nrf_802154_cb_dispatch_entry, _entry_name) = {              \
		.name = NRF_802154_CALLBACKS_DISPATCHER_NAME_STR_EXPAND(_entry_name),              \
		.callbacks = &(_callbacks_var),                                                    \
		.saved_configuration = false,                                                      \
		.ext_address = {0},                                                                \
		.short_address = 0,                                                                \
		.pan_id = {0},                                                                     \
	}

/**
 * @brief Switch callbacks to the new client.
 *
 * @param name New client name (same name used in REGISTER).
 * @retval 0 Success.
 * @retval -EINVAL Name not found.
 */
int nrf_802154_callbacks_dispatcher_switch(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_CALLBACKS_DISPATCHER_H */

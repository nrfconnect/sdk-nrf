/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Declarations for nRF 802.15.4 driver APIs used by the callbacks dispatcher but
 * omitted from nrf_802154.h when NRF_802154_SERIALIZATION_HOST is set.
 */

#ifndef NRF_802154_TEST_API_H_
#define NRF_802154_TEST_API_H_

#include <nrf_802154_types.h>
#include <stdbool.h>
#include <stdint.h>

bool nrf_802154_transmit_at_cancel(void);
void nrf_802154_promiscuous_set(bool enabled);
void nrf_802154_pan_coord_set(bool enabled);
void nrf_802154_auto_ack_set(bool enabled);
void nrf_802154_auto_pending_bit_set(bool enabled);
void nrf_802154_pending_bit_for_addr_reset(bool extended);
void nrf_802154_tx_power_set(int8_t power);
void nrf_802154_ack_data_remove_all(bool extended, nrf_802154_ack_data_t data_type);
bool nrf_802154_reinit(void);
void nrf_802154_pan_id_set(const uint8_t *p_pan_id);
void nrf_802154_short_address_set(const uint8_t *p_short_address);
void nrf_802154_extended_address_set(const uint8_t *p_extended_address);
void nrf_802154_init(void);

#endif /* NRF_802154_TEST_API_H_ */

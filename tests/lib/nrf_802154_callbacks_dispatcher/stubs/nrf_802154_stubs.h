/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_802154_STUBS_H_
#define NRF_802154_STUBS_H_

#include <nrf_802154_const.h>
#include <stdint.h>

struct nrf_802154_stub_stats {
	uint32_t init_count;
	uint32_t reinit_count;
	uint32_t transmit_at_cancel_count;
	uint32_t promiscuous_set_count;
	uint32_t pan_coord_set_count;
	uint32_t auto_ack_set_count;
	uint32_t auto_pending_bit_set_count;
	uint32_t pending_bit_for_addr_reset_count;
	uint32_t tx_power_set_count;
	uint32_t ack_data_remove_all_count;
	uint32_t pan_id_set_count;
	uint32_t short_address_set_count;
	uint32_t extended_address_set_count;
	uint8_t last_pan_id[PAN_ID_SIZE];
	uint8_t last_short_address[SHORT_ADDRESS_SIZE];
	uint8_t last_extended_address[EXTENDED_ADDRESS_SIZE];
};

extern struct nrf_802154_stub_stats nrf_802154_stub_stats;

void nrf_802154_stub_reset(void);

#endif /* NRF_802154_STUBS_H_ */

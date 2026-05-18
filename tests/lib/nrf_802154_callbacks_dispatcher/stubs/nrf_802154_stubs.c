/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_802154_stubs.h"

#include <nrf_802154.h>
#include <string.h>
#include <zephyr/sys/util.h>

struct nrf_802154_stub_stats nrf_802154_stub_stats;

void nrf_802154_stub_reset(void)
{
	memset(&nrf_802154_stub_stats, 0, sizeof(nrf_802154_stub_stats));
}

void nrf_802154_init(void)
{
	nrf_802154_stub_stats.init_count++;
}

bool nrf_802154_reinit(void)
{
	nrf_802154_stub_stats.reinit_count++;

	return true;
}

bool nrf_802154_transmit_at_cancel(void)
{
	nrf_802154_stub_stats.transmit_at_cancel_count++;

	return true;
}

void nrf_802154_promiscuous_set(bool enabled)
{
	ARG_UNUSED(enabled);
	nrf_802154_stub_stats.promiscuous_set_count++;
}

void nrf_802154_pan_coord_set(bool enabled)
{
	ARG_UNUSED(enabled);
	nrf_802154_stub_stats.pan_coord_set_count++;
}

void nrf_802154_auto_ack_set(bool enabled)
{
	ARG_UNUSED(enabled);
	nrf_802154_stub_stats.auto_ack_set_count++;
}

void nrf_802154_auto_pending_bit_set(bool enabled)
{
	ARG_UNUSED(enabled);
	nrf_802154_stub_stats.auto_pending_bit_set_count++;
}

void nrf_802154_pending_bit_for_addr_reset(bool extended)
{
	ARG_UNUSED(extended);
	nrf_802154_stub_stats.pending_bit_for_addr_reset_count++;
}

void nrf_802154_tx_power_set(int8_t power)
{
	ARG_UNUSED(power);
	nrf_802154_stub_stats.tx_power_set_count++;
}

void nrf_802154_ack_data_remove_all(bool extended, nrf_802154_ack_data_t data_type)
{
	ARG_UNUSED(extended);
	ARG_UNUSED(data_type);
	nrf_802154_stub_stats.ack_data_remove_all_count++;
}

void nrf_802154_pan_id_set(const uint8_t *p_pan_id)
{
	memcpy(nrf_802154_stub_stats.last_pan_id, p_pan_id, PAN_ID_SIZE);
	nrf_802154_stub_stats.pan_id_set_count++;
}

void nrf_802154_short_address_set(const uint8_t *p_short_address)
{
	memcpy(nrf_802154_stub_stats.last_short_address, p_short_address, SHORT_ADDRESS_SIZE);
	nrf_802154_stub_stats.short_address_set_count++;
}

void nrf_802154_extended_address_set(const uint8_t *p_extended_address)
{
	memcpy(nrf_802154_stub_stats.last_extended_address, p_extended_address,
	       EXTENDED_ADDRESS_SIZE);
	nrf_802154_stub_stats.extended_address_set_count++;
}

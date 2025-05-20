/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __NRF_802154_RADIO_WRAPPER_H__
#define __NRF_802154_RADIO_WRAPPER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets current automatic acknowledgments mode state.
 *
 * @return                true when auto ACK is set, false otherwise
 */
bool nrf_802154_radio_wrapper_auto_ack_get(void);

/**
 * Enables or disables the automatic acknowledgments mode (auto ACK).
 *
 * @param[in]  enabled        Value setting (true) or clearing (false)
 *                              auto ACK radio mode.
 */
void nrf_802154_radio_wrapper_auto_ack_set(bool enabled);

/**
 * Gets the device hardware capabilities
 *
 * @return                0 on failure; nonzero hw capabilities value
 *                          on success (@ref ieee802154_hw_caps).
 */
uint16_t nrf_802154_radio_wrapper_hw_capabilities_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __NRF_802154_RADIO_WRAPPER_H__ */

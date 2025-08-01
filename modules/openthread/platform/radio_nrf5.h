/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_PLATFORM_RADIO_NRF5_H
#define OT_PLATFORM_RADIO_NRF5_H

#include <nrf_802154_const.h>

#include <stdint.h>

void openthread_platform_radio_set_eui64(uint8_t eui64[EXTENDED_ADDRESS_SIZE]);

#endif /* OT_PLATFORM_RADIO_NRF5_Hz */
